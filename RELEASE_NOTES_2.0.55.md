# CYN v2.0.55 — Liquid Swallow & Faster Controls

A focused native UI update. Audio processing, presets, neural models, themes, and 31/35/39-band EQ behavior are unchanged.

## What's new
- **Liquid close-to-tray:** Pressing the window **X** captures one native frame and distorts it into the real X position over approximately 390 ms, then leaves CYN safely running in the tray with audio uninterrupted.
- **Faster controls:** Buttons, hover responses and toggles use stronger interpolation per frame so the UI reacts more promptly.
- **Smoother power animation:** The native power iris runs in approximately 410 ms with a lightly irregular, organic edge instead of a rigid circle.
- **Accessibility and performance:** Reduce Motion bypasses the liquid effect. The close animation uses a single capture and a short-lived click-through JUCE overlay, without a continuous fluid simulation or DSP processing in the renderer.

## Verification
- Liquid-close and power-swallow regression workflow passed.
- Native Windows x64 build validation passed for the PR before the version bump.
- Actual visual quality, frame-rate characteristics on low-end PCs and tray behavior must still be verified on a user's Windows machine.

## Planned separately, not in this release
- Precomputed fluid-simulation animations inspired by [tmarrec/fluid-simulation](https://github.com/tmarrec/fluid-simulation), potentially generated in CI and packaged as optimized visual assets. This release does not import or run that simulator.
