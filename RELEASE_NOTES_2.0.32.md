# Cyn v2.0.32 — Crescendo Studio UI redesign

This update redesigns Cyn's **native Windows Sound Studio** to follow the previously approved #5 Crescendo Studio HTML concept. It is implemented in JUCE/C++ and uses real DSP and measured audio telemetry, not an image overlay or a website.

- Dark midnight/violet workspace with a quieter composition, distinct rounded panels, and a headline inspired by the #5 design.
- Dynamic Energy Timeline: recent pre-effect input RMS readings, captured from the existing native analyzer on the UI thread. Silence displays an idle state; there is no synthetic waveform.
- Intelligent Dynamics: current YAMNet music and percussion confidence, plus the actual Crescendo detector's quiet-to-strong rise score. Scores are explicitly shown as live/idle, not guessed percent.
- Five **interactive** rotary controls for Clarity, Ambience, Surround, Dynamics and Bass. They retain the real native DSP setters, manual keyboard input, saved-preset comparison markers and smooth applied-DSP following.
- Compact editable equalizer and the existing Flip-to-output-tuning action. Existing preset and device selectors, Smart Listening toggle, AI Hub, model training and AI DJ pages remain present.
- Dark theme palette now uses the purple/violet hues from #5. Other pre-existing user theme modes are preserved.

**Limitations:** Cyn's native 1240x620 layout is a compact adaptation of the 1600x960 HTML mockup, not a pixel-perfect website transplant. This release does not add Spotify or fabricated track metadata. It changes presentation and telemetry, not the existing audio engine. The real Windows build and app smoke tests run in GitHub Actions; on-device visual and accessibility review is still desirable.
