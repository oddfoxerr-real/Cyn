# CYN Windows v2.0.78 — AUDONIC Studio, reactive BrainScan & Material 3

This Windows stable-channel update packages the Windows changes previously merged through PR #73. It is the successor to the public **v2.0.73** stable installer and uses **v2.0.78** to avoid a version downgrade for users testing experimental installers through v2.0.77.

## Changes
- **AUDONIC Studio**: optional native C++ adaptive music/creative DSP policy, including six bounded channels (Harmonics, Attack, Width, Colour, Rhythm, Overtones), saved Creative intensity and native offline AI Hub selection.
- **Reactive 3D BrainScan**: JUCE 3D-projected neural visualization wired to available fresh inference and creative-control measurements, with reduced-motion support and a switch back to the established neural graph.
- **Material 3-inspired Windows theme**: optional dark tonal palette in Theme Studio, without borrowing Android wallpaper colours.
- **Updater compatibility**: the native executable, JUCE project, Windows version resource and Inno Setup all report v2.0.78. The Windows updater prefers the stable GitHub `releases/latest` endpoint to avoid being crowded out by Android and AI-model prereleases, with a paginated fallback when the latest release is unusable.
- Existing FxSound audio pipeline, upstream attribution, Windows driver, APEX 6M, settings, other themes and Android release channel are retained.

## Boundaries & validation
- **Windows AUDONIC is a native bounded creative DSP profile**, not a port of the Android AUDONIC ONNX student and not a 3-billion-parameter neural model.
- BrainScan renders measured control/inference activity; its geometry is a visualization, not a literal model-architecture scan.
- Release gates must build the Windows native GUI, model smoke test, installer, portable archive and SHA-256 checksum before publishing `v2.0.78`. Do not overwrite any previously published installer.
- Upgrading should preserve the same application identity and user settings; driver installation requires administrator approval where necessary.
- Device-specific audio routing, headphone sound quality and real-machine installation remain unverified until tested on Windows hardware.
