# Riff UI continuation after CYN Windows v2.0.81

The merged PR #79 remains the native JUCE implementation based on Riff's MIT-licensed design references. This branch improves verifiability without replacing the FxSound DSP/AI engine.

## Added verification

- Five native page layout geometries across EQ 31/35/39-band selections: 15 SVG layout previews.
- Preview geometry is checked against actual FxProView.cpp and CynRiffLayout.h positions and transformation constants.
- Real FxEqSlider::valueChanged uses the tested CynEqUiCallbacks::dispatchGain adapter to call the existing FxController equalizer setter. No DSP setting or behavior is removed.
- Isolated C++ tests exercise 31/35/39 callbacks, repeated values, NaN/infinity safety and returning to zero. Tests do not start audio, create a device, or read user settings.
- Windows JUCE/MSVC build checks continue on the PR alongside existing motion and AI regressions.

**Limitation:** SVGs are deterministic layout previews, not actual native JUCE screenshots. Successful source checks and native compilation do not replace a hands-on screen and audio inspection on Windows. In particular check navigation, 31/35/39 EQ mouse/keyboard changes, AUDONIC/APEX, BrainScan, and reduced-motion mode.

No uninstall, no model changes, and no app version bump are required for these validation-only changes.
