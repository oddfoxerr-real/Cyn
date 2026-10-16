# Cyn v2.0.37 — 13-hour real-audio neural model in the native AI Hub

This release **bundles an optional trained Cyn AI profile** in Cyn's built-in AI Hub, alongside the unchanged original Cyn Starter and Cyn Cinema profiles.

## New AI profile
- **Cyn Real Audio 13h (Experimental)** — 209-weight Bass/Clarity preference model, trained for 46,800 seconds of real gradient optimization on spectral features derived from permission-cleared real speech and recorded instrument notes.
- Training source and results: [verified GitHub Actions run](https://github.com/oddfoxerr-real/Cyn/actions/runs/38854165237) and [original published checkpoint and report](https://github.com/oddfoxerr-real/Cyn/releases/tag/cyn-real-audio-lab-13h-20261016).
- Independent held-out error (on *heuristic preference targets*, lower is better): prior starter **0.195295**, newly trained **0.001956**. This metric **does not prove audible enhancement**. Training used speech and individual instrument notes, not a representative set of full commercial music.
- The native Cyn AI Hub automatically includes this profile offline. Open **AI Hub** to view it and press **Import** when you want to try it.
- Import follows Cyn's existing backup process. Existing personal learning, music/movie preferences, effects, presets, driver settings, YAMNet worker, theme selector and other application functions remain unaffected until you opt in.
- The trained network predicts only Bass and Clarity preferences. YAMNet continues providing pretrained music/speech scene information in real time. Experimental Silero VAD and DeepFilterNet smoke tests are **not** runtime components of this release.

## Implementation
- Updated native app and signed-driver-preserving setup metadata to **2.0.37**.
- Added model-format, finite-weight, and hub-catalogue checks to the existing Windows release workflow.
- No paid runner, GPU, cloud-audio upload or music copyright download is required to run Cyn.

**Limitations:** Real-recording heuristic prediction is not equivalent to listening preference; this release does not claim improved phonk bass, movie dialogue, de-essing, true-peak loudness, or objectively better perceived audio. Try the profile before choosing it as your everyday model.
