# Cyn 2.0.27 — Real AI Control Reference Markers

This release improves the v2.0.26 real DSP-driven Sound Studio visual system. Every displayed change continues to correspond to an effect/EQ value **actually applied by Cyn's DSP**.

## What's improved

- **Uninterrupted AI movement:** Sound Studio refreshes, including Music/Movie AI model selection and preset changes, no longer force EQ and enhancement sliders to jump directly to a new value while Smart Listening is active. They follow the live DSP using the existing smooth UI interpolation.
- **Saved-preset markers for the five enhancement sliders:** A subtle neutral notch marks the manually selected preset value whenever the *actual applied* effect differs meaningfully from it. The moving knob remains the actual live effect.
- **Original-vs-applied EQ positions:** Hollow dots mark the saved preset's EQ gains and fine connectors show genuine differences against the AI-adjusted EQ points. Markers appear only if actual applied gains differ by at least 0.20 dB.
- **Truthful, minimal presentation:** No simulated bass pulse, random wobble, artificial EQ oscillation, or animation while values are unchanged.
- **Reduced motion:** UI snaps to the actual applied value when the user has reduced-motion enabled. Initial rendering, AI-off, and disabled processing similarly avoid stale/interpolated state.
- Preset reference markers disappear when AI is disabled or the applied values return to the preset.

## Safety and compatibility

- This is a UI-only change; it does not change the audio engine, training, driver management, AI output calculations, or user EQ/preset values.
- Manual control gestures, keyboard adjustments and Alt-drag multi-band EQ editing remain supported.
- Windows native compilation and automated tests validate that the UI reads the real DSP and that markers appear only when applied values differ from the preset. Actual appearance on all devices has not been personally tested by the project owner.

## Downloads

- Windows x64 installer: **Cyn-Setup-2.0.27-x64.exe**.
- Portable Windows x64 build: **Cyn-Windows-x64.zip**.
- SHA-256 checksum for the installer included.
- Releases are not yet code-signed; Windows may display a security warning.

Licensed under GNU AGPL-3.0-or-later.
