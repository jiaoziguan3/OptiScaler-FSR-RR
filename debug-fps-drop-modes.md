# Debug Session: fps-drop-modes

Status: [OPEN]

## Symptoms

- With FSR-RR enabled, changing the upscaling quality preset cuts frame rate roughly in half.
- Switching between path tracing and regular ray tracing causes the same persistent frame-rate drop.

## Hypotheses

- H1: The denoiser context is recreated every frame after the render resolution changes.
- H2: Mode switching creates a second RR feature while the previous context remains alive.
- H3: The multi-frame backend change state fails to converge after a runtime mode switch.
- H4: Denoiser, conversion buffers, and upscaler retain mismatched render sizes after switching.

## Evidence

- Runtime observation: both quality changes and PT/RT changes reduce frame rate by approximately 50%.
- The reduction occurs only while FSR-RR is active.
- FSR-RR called UpdateSize before GetRenderResolution refreshed the current frame dimensions.
- UpdateSize ignored CreateDenoiserContext failure and continued dispatching.
- DestroyDenoiserContext did not explicitly clear the context pointer after destruction.
- Static tracing found no duplicate denoiser/upscaler dispatch inside one FSR-RR Evaluate.
- Post-fix runtime screenshot shows repeated magenta lighting remnants after RR disable and a persistent 50% frame-rate loss.
- The symptom is consistent with stale RR and newly created SR/RR handles both being evaluated after a mode switch.
- User chose direct single-active-handle enforcement instead of collecting the existing handle trace log.
- First single-active implementation restored frame rate but produced severe striped/uninitialized output.
- This proves create-order arbitration selected a pre-created but currently unevaluated handle and skipped the handle that needed to write Output.
- Evaluate/reset-driven arbitration produced the identical corrupted output; Reset is not a reliable switch signal in this title.

## Progress

- [x] Capture initial runtime symptoms.
- [x] Define falsifiable hypotheses.
- [x] Trace resolution and feature lifecycle changes.
- [x] Identify stale-size context rebuild ordering.
- [x] Implement the minimal evidence-backed fix.
- [x] Build Release successfully.
- [x] Enforce a single active SuperSampling/RayReconstruction handle.
- [x] Build the single-active-handle Release successfully.
- [x] Replace create-order arbitration with Evaluate/reset-driven activation.
- [x] Build the dynamic activation Release successfully.
- [x] Remove unsafe handle suppression and rebuild a usable baseline.
- [ ] Collect the real Create/Evaluate/Release sequence from OptiScaler.log.
- [ ] Verify both switch scenarios in game.

## Fix

- Refresh current render dimensions before checking the denoiser context size.
- Rebuild only when the refreshed dimensions differ from the context dimensions.
- Stop the frame if context recreation fails.
- Clear the context pointer after destruction.
- Removed the ineffective native SuperSampling passthrough experiment from the previous issue.
- The newest successfully created RR/SR handle becomes the only handle allowed to dispatch.
- Evaluate calls for stale RR/SR handles return success without submitting GPU work.
- Releasing a stale handle does not clear the current active handle.
- Released handle metadata is removed to avoid stale feature classification.
- Handles are no longer activated during CreateFeature.
- The first evaluated RR/SR handle becomes active; a handle evaluated with Reset=1 immediately takes over during a mode switch.
- Non-active handles remain suppressed to prevent duplicate GPU work.
- Both handle-suppression experiments were removed after runtime falsification.
- Further handle arbitration requires the actual runtime handle sequence and output-resource evidence.
