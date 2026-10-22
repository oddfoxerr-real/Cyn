# CYN Windows — AUDONIC Studio, 3D BrainScan and Material 3 theme

This work is committed on a separate Windows feature branch. It does **not** change Android APKs or overwrite previous releases.

## Included
- **AUDONIC Studio (Music AI)**: optional six-channel, native C++ creative-control policy (Harmonics, Attack, Width, Colour, Rhythm, Overtones) driven by local PCM input features and available YAMNet scene outputs. Bounded, headroom-aware targets adjust the existing FxSound clarity, ambience, surround, dynamics and bass controls plus subtle EQ. No synthetic audio generation or network inference.
- **Creative intensity**: 0–100% native slider in Smart Listening. Persists in the existing CYN settings store. AI selection preserves current personal learning and previous Music AI profile.
- **3D BrainScan**: native JUCE rotating, two-hemisphere visual mesh that responds to fresh personal inference and (for AUDONIC) fresh six-channel DSP target telemetry. Animation pauses without audio and honours Reduced motion. A **Neural graph** button restores the existing faithful 11→12→5 graph.
- **Material 3**: opt-in dark tonal design with indigo/lavender accents, rounded native JUCE controls, restrained native background elements, and persistent Theme Studio registration. Not Android UI components or wallpaper extraction.
- **AI Hub**: AUDONIC Studio can be activated from the native offline profile list even if the community catalogue is unavailable.
- **Preserved**: existing FxSound DSP and driver, APEX 6M, personal training, movie mode, device routing, saved settings, other native themes and the Android project.

## Scope and verification
AUDONIC Studio for Windows is a native scene-adaptive DSP profile. It is **not** the Android AUDONIC ONNX student and does not claim to be a port of that model. The BrainScan's artistic mesh geometry is not a map of model architecture; its brightness comes from actual live inference/creative control snapshots.

The feature branch contains a C++17 native policy test, source-integration assertions and a GitHub Actions regression job. Native Windows JUCE builds, installer signing, performance on low-end PCs, and listening comparisons require separate validation before release.

No installer or Windows release has been published by these source commits.
