# CYN Mobile v0.2.28 preview — Material 3 BrainScan

- Redesigns AUDONIC's Creative BrainScan and CYN's connected/APEX neural view using the app's **real Material 3 palette roles**: surface containers, outlined rounded viewport, on-surface typography, softened neural colours and theme-aware contrast across all 11 palettes.
- AUDONIC's six named channels now appear as compact rounded **tonal activity tiles** in a responsive three-column/two-row arrangement. Their progress tracks are driven exclusively by fresh actual AUDONIC policy outputs multiplied by the user's Creative intensity; disconnected or stale telemetry fades to idle.
- Retains the 3D-projected brain mesh, measured neural pathways and decision-triggered pulses. The mesh is clipped to its own viewport so it no longer overlaps the channel tiles.
- Adds a compact LIVE / WAITING / OFFLINE state and a readable signal footer. Respects Reduce motion, device font scaling and existing CYN accessibility preferences.
- Enlarges the AUDONIC scan area to make six-channel readings legible, without replacing the AI engines or reworking the underlying audio processing.
- Preserves Base AI, APEX 6M, APEX 10M, AUDONIC ONNX/native DSP, Connect, Library, saved themes/settings, user learning, and prior installation identity.

**Android upgrade:** package `io.github.oddfoxerr.cyn.studio`, versionCode **32** (`0.2.28-preview`); signed using the existing CYN Studio key, verified against installed v0.2.27 and earlier versions. The in-app updater continues using the existing official GitHub Android release feed.

Release CI checks the Material 3 and fresh-signal contracts, native DSP and actual ONNX inference, Android compile and lint, signing certificate, packaged model integrity, and increasing version code. Hardware-specific Connect and perceptual audio quality still need real-device verification.
