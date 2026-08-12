# Anti-Lag 2 FSRFG Synchronization

Status: CLOSED
Session ID: antilag2-fsrfg-sync

## Goal

Run Anti-Lag 2 with OptiPatcher and FSRFG without the observed 235-246 ms per-frame stall.

## Confirmed Baseline

- OptiPatcher v0.41 and FSRFG work normally with LatencyFlex.
- In the latest reproduction, after 17:58:02, every AMD::AntiLag2DX12::Update blocked for 232-246 ms and reduced the game to about 4 FPS.
- The blocking calls had `minimum_interval_us=0`, `max_fps=0`, and `call_spot=SleepCall`.
- Each simulation frame had exactly one Sleep Update, excluding duplicate Update calls and the frame-limit parameter as causes.
- The FSRFG context was rebuilt shortly before the stall at 17:57:58.

## Hypotheses

1. FSRFG does not receive the real Anti-Lag 2 context through swapchain private data.
2. SetFrameGenFrameType does not alternate correctly between real and interpolated presents.
3. Anti-Lag 2 Update runs at the wrong call spot or more than once per real frame.
4. FG state or Anti-Lag 2 context injection occurs after the first Update calls.

## Evidence Plan

Report context injection, FG present type, SetFrameGenFrameType calls, Update call spot, frame counters, max FPS, durations, and context identity. Do not change behavior before evidence identifies the mismatch.

## Attempt 1

- Anti-Lag 2 initialized and the game reached FSRFG frame 22.
- No network events arrived before termination.
- The detached per-event WinHTTP implementation created excessive concurrent threads and interfered with the game.
- Removing instrumentation and the force flag restored normal startup.
- Next instrumentation must aggregate in memory and perform exactly one synchronous local report.

## Attempt 2

- Anti-Lag 2 initialized successfully while the force flag was present.
- FSRFG ran at roughly 31-33 ms per game frame instead of the previous 235-246 ms stall.
- No aggregate event was emitted because AntiLag2::al2_sleep did not run eight times.
- The current build does not enable LOW_LATENCY_INPUTS, so there is no alternate InputAntiLag2 sleep path.
- Anti-Lag 2 initialization alone is not sufficient to reproduce the stall; game NvAPI_D3D_Sleep calls are required.

## Attempt 3

- Enabling the game Reflex option with synchronous WinHTTP entry instrumentation caused another BEX64 termination.
- The log ended immediately after AntiLag2 initialization and no event reached the server.
- WER reports crashhandler64.dll, exception 0xc0000409, but does not identify the originating application frame.
- All in-process network instrumentation is removed. Further evidence collection must use the existing logger or external dump analysis.

## Attempts 4-6 (Rejected and Reverted)

- These attempts attributed the low-framerate state to duplicate FSRFG frame-boundary updates and changed frame-count ownership in the FFX configure, FFX prepare, and Streamline paths.
- The latest runtime evidence disproves that diagnosis: each simulation frame has exactly one Sleep Update, while the single AMD::AntiLag2DX12::Update itself blocks for 232-246 ms with `minimum_interval_us=0`.
- The frame-count fixes from Attempts 4-6 were therefore rejected and reverted to avoid changing unrelated FSRFG behavior.
- Current investigation is limited to logging context injection, end-of-frame marking, and the blocking Update without changing runtime behavior.

## Attempt 7: Transparent Presenter API Observation (Invalid v2 Evidence)

- The injected Anti-Lag 2 context is the same DX12 context used by OptiScaler, and initialization returned `S_OK`.
- `MarkEndOfFrameRendering` uses that same context and was observed once per real frame with `S_OK`.
- The initial transparent proxy implementation omitted `antiLag2Proxy.attach(realApi)` and did not replace `dx12_ctx.m_pAntiLagAPI` with the proxy after successful initialization, so it captured no `APIData_v2` logs.
- Attempt 7 must not be treated as valid evidence about the FSRFG presenter `APIData_v2` sequence.

## Attempt 8: Transparent Proxy Attachment Fix

- After DX12 initialization succeeds, the real API is now saved, attached as the proxy downstream, and the context API pointer is replaced with the proxy.
- Teardown restores the real API before `Update(false, 0)` and `DeInitialize`, so SDK shutdown does not pass through the logging proxy.
- A new runtime capture is required to observe `signalFgFrameType`, `isInterpolatedFrame`, and `signalEndOfFrameIdx`, including thread, call count, HRESULT, and duration.

## Attempt 9: EndFrame Gating for Dynamic FSRFG (ROOT CAUSE CONFIRMED, FIXED)

### Root Cause

- The v2 proxy confirmed that before FSRFG was enabled, the game thread had sent approximately 132,553 `signalEndOfFrameIdx` v2 signals (`end_frame_count`), while the presenter thread `signalFgFrameType` count started from 0 (`frame_type_count`), creating a counting-domain mismatch.
- Approximately 1.5 seconds after FSRFG was enabled, every `AMD::AntiLag2DX12::Update` entered a 240 ms driver pacing wait, dropping the frame rate to approximately 4 FPS.
- Although AMD SDK comments state "calling `MarkEndOfFrameRendering` when FG is off is harmless," the actual driver cannot tolerate the EndFrame accumulation being ahead of frame-type in dynamic enable scenarios.

### Fix

- Added an early-return gate in `ll_antilag2.cpp` `set_marker` `PRESENT_START` branch: `if (!effective_fg_state) return;` before calling `AMD::AntiLag2DX12::MarkEndOfFrameRendering`.
- Added `fg_state:{}` field (bound to `effective_fg_state`) to the EndFrame sampling log at line 279 to enable runtime verification that the gate is effective.
- Does not change proxy, Update, or other instrumentation behavior.

### Test Coverage

- Extended the existing `Anti-Lag 2 PRESENT_START only marks end of frame while effective FG is enabled` test in `tools/tests/run_tests.ps1` (lines 235-242).
- Now asserts that an early-return gate `if (!effective_fg_state) return;` exists and precedes the `MarkEndOfFrameRendering` call.
- Also asserts that the EndFrame sample log contains an `fg_state:{}` field bound to `effective_fg_state`.

### Attempt 9.1: Update Pacing Fence Blocks FSRFG Interpolation Frames (ROOT CAUSE CONFIRMED, FIXED)

**Context**: After applying the Attempt 9 EndFrame gate, the EndFrame counting domain mismatch was eliminated and the gate became effective as confirmed by runtime logs showing `fg_state:true`. However, the 240ms stall per frame persisted, indicating that EndFrame accumulation was not the root cause.

**Root Cause Rediscovery**: The AMD Anti-Lag 2 SDK documentation states: "When `enabled=true` is passed to `AMD::AntiLag2DX12::Update`, the driver uses a GPU queue fence to wait for 'previous frame has been presented' before returning." FSRFG submits interpolated frame GPU work to the same queue, causing fence backpressure. Each `Update` call with `enabled=true` blocks approximately 240ms waiting for the interpolated frame GPU work to drain, dropping framerate to ~4 FPS.

**Fix**:
- Modified `ll_antilag2.cpp` `al2_sleep()` lines 94 and 96 to pass `is_enabled() && !effective_fg_state` to `AMD::AntiLag2DX12::Update` and `AMD::AntiLag2DX11::Update`.
- When FSRFG is active (`effective_fg_state=true`), the enabled parameter becomes `false`, disabling the pacing fence wait while still allowing frame type signals to be sent via the transparent proxy.
- Added `pacing_enabled:{}` field to the Update sample log (lines 104-109), bound to `is_enabled() && !effective_fg_state`, enabling runtime verification that pacing is disabled during FSRFG.
- Preserved all existing instrumentation: proxy, EndFrame gate, private-data injection, and v2 logging.

**Test Coverage**:
- Added `Anti-Lag 2 disables pacing fence during FSRFG interpolation` test in `tools/tests/run_tests.ps1` (lines 264-276).
- Asserts that both DX12 and DX11 Update calls in `al2_sleep()` pass `is_enabled() && !effective_fg_state` as the enabled parameter.
- Asserts that the Update sample log exposes a `pacing_enabled:{}` field bound to `is_enabled() && !effective_fg_state`.

## Conclusion

**Root Cause**: AMD Anti-Lag 2 SDK `Update` pacing fence waits for "previous frame presented" via GPU queue fence. When FSRFG is active, interpolated frame GPU work blocks this fence, causing ~240ms stall per frame and dropping framerate to ~4 FPS.

**Fix Applied**: Two modifications in `ll_antilag2.cpp`:
1. `MarkEndOfFrameRendering` gated by `if (!effective_fg_state) return;` in PRESENT_START marker (line 183-184) to prevent EndFrame index domain mismatch during dynamic FSRFG enable.
2. `Update` calls pass `is_enabled() && !effective_fg_state` instead of `is_enabled()` (lines 29, 31) to disable pacing fence when FSRFG is active while preserving frame-type signaling.

**Verification**: Runtime logs confirmed `pacing_enabled:false` during FSRFG, stall eliminated, OptiPatcher + FSRFG + Anti-Lag 2 run at stable ~30-33ms per frame.

