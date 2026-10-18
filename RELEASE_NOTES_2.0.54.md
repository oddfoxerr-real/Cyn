# CYN v2.0.54 — Responsive Studio Navigation

Inspired by the fast, decisive page changes and low-latency highlights in the reference DroidDeck video, while retaining Cyn's own themes, controls and audio processing.

## Faster interaction

- Selecting **Sound Studio**, **Smart Listening**, **Devices**, or **Misc / AI DJ** updates the selected page and navigation state **immediately**.
- The previous page now fades and glides away within **165 ms** rather than covering the entire new window for 220 ms. The transition snapshot is limited to the content workspace; the sidebar stays clear and usable.
- Repeated clicks on the selected tab do not restart transitions. Rapid consecutive switches discard any stale animation snapshots rather than accumulating full-window overlays.
- Theme Studio carousel previews finish within **240 ms** instead of 420 ms. Motion is frame-rate independent and follows a tested smooth easing curve.
- Reduce Motion disables nonessential transitions. No inference, network calls, file reading or DSP modifications are performed inside the transition renderer.

## What stays unchanged

- Two separate real-activity-driven DJ hands/cursors/PNG/flower layers: one follows effects, the other follows real 31/35/39-band EQ corrections, up to four independently targeted flowers per panel. The visuals remain optional.
- All existing audio parameters, 6.3M APEX ONNX model weights, personal AI training, Smart Listening settings, current presets, Theme Studio themes and user settings.
- No new model training.

## Verification

CI runs deterministic compiled C++ motion math tests, JavaScript integration regression checks (no full-window capture, no delayed selection, both AI Touch panels intact), and builds the real CYN Windows application. Crossfade visuals are designed to stay responsive even on low-end PCs; detailed GPU/CPU frame timings are not yet benchmarked on consumer hardware.
