# Debug Session: floor-boiling

Status: [OPEN]

## Symptoms

- Floor debug views show black edge trails while rotating the camera.
- None view shows boiling noise on the ground while rotating the camera.

## Hypotheses

- H1: The 5x5 luminance percentile seed crosses geometry edges and changes discontinuously under camera motion.
- H2: The A-Trous depth rejection weakens with distance and permits cross-edge color bleeding.
- H3: The computed depth gradient is not used by the floor filter, leaving disocclusion edges unprotected.
- H4: The boiling originates in the floor residual path rather than the denoiser or temporal stabilization.

## Evidence

- Runtime observation: Floor-prefixed debug views reproduce black edge trails during camera rotation.
- Runtime observation: None view reproduces ground boiling during camera rotation.
- Post-fix observation: floor artifacts are significantly reduced.
- Temporal Stable Layer is disabled.
- DenoiserOutput responds immediately after movement stops.
- UpscalerBypass responds immediately after movement stops.
- The delayed lighting response therefore appears only after the FSR upscaler history stage.
- User confirmed both FloorColor and FloorVariance reproduce the black edge trails.
- FSRDFloor loaded center/tap depth gradients but did not use either value in its bilateral weight.
- The failing regression check confirmed no depth-gradient edge rejection existed before the fix.

## Progress

- [x] Capture initial runtime symptoms.
- [x] Define falsifiable hypotheses.
- [x] Inspect floor seed and filter data flow.
- [x] Apply minimal instrumentation or use existing debug views to isolate the stage.
- [x] Implement evidence-backed fix.
- [x] Build Release successfully.
- [ ] Verify post-fix behavior in game.
- [x] Isolate delayed lighting response to the FSR upscaler stage.
- [x] Add an FSR-RR-only upscaler response override.
- [x] Build the updated Release successfully.
- [ ] Verify lighting response in game.

## Fix

- Predict tap depth from the center depth gradient before computing depth rejection.
- Reject taps whose local depth gradient differs from the center gradient.
- Regenerated FSRDFloor_Shader.h with fxc and built Release with v145.
- FSR-RR now uses at least 4.0 shading-change scale and 0.75 accumulation added per frame.
- Ordinary FSR keeps the configured values and behavior.
