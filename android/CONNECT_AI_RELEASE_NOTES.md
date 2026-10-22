# CYN Mobile 0.2.27 preview — Material 3 Expressive Controls

CYN's existing Android audio engine remains unchanged. This update refines the native Material 3 interface while preserving all features from 0.2.26.

- Larger, rounded tonal buttons with CYN's existing 11 fixed theme palettes (no wallpaper-based colour extraction).
- Bigger primary playback control, clearer transport hierarchy and rounded library search.
- Connected single-choice selectors for 31/35/39-band EQ and repeat modes with checked-state accessibility.
- Selected AI-engine cards are visually highlighted; Base AI, APEX 6M, APEX 10M and AUDONIC are retained.
- A playback-aware wavy library seek bar keeps native dragging, keyboard control and accessibility. Animation obeys Reduce motion.
- While the audio service connects, the mini-player indicates that state and disables unavailable playback rather than accepting a nonfunctional tap.
- Preserves AUDONIC's six-channel measured BrainScan from 0.2.26, ONNX/native DSP, compatible Connect effects, Library, saved preferences, fox sliders, and the updater's public-feed fallback.
- Signed with the same CYN Studio certificate under package `io.github.oddfoxerr.cyn.studio`. Version code increases from 30 to 31, so Android can install this APK over 0.2.26 without erasing app data.

Release verification: actual AUDONIC and APEX model inference, native PCM/DSP tests, existing BrainScan and Connect regressions, full release APK and debug APK assembly, lint, model-byte integrity, signature, package name and higher version-code checks against 0.2.22–0.2.26. Hardware-specific Connect capabilities and physical-phone playback remain device-dependent and are not represented as validated by CI.
