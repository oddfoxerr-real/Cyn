# Android 0.1.1 preview validation — 2026-10-11

Built locally with JDK 17, Gradle 8.11.1, AGP 8.9.2 and Android SDK 35.

- `:app:assembleRelease :app:lintRelease` succeeds. Zero lint errors; five
  existing English localization warnings. No drawing-allocation warning remains.
- Pure-Java host tests: **285 assertions passed**. Covers audio feature extraction,
  learned More/Less separation, inference/telemetry equivalence, every connection's
  contribution, real candidate-training snapshots and Android-only version selection.
- Package: `io.github.oddfoxerr.cyn`, versionCode **2**, versionName **0.1.1-preview**,
  minimum SDK 28, target SDK 35. Original package identity retained.
- Release signing key retained from 0.1.0. Certificate SHA-256:
  `20698db6b762137a5e5199c0c5a6c728e8b90374d9dc76973c6ea21c005ee13d`.
- Internet permission is used for explicit GitHub update checks. No audio, numerical
  features, model weights or user feedback are sent. Android's installer handles
  installing the downloaded APK and enforcing its signature/version.

**Not device-tested:** Galaxy A56/Spotify routing, legacy output-mix support,
rendering, phone permissions, playback/Bluetooth, and installing over 0.1.0 on a
physical phone. No device or emulator was available. EQ control status reports
Android's API state and cannot establish that a particular external app is audible
through that effect. Compare Enhancement on/off using the same music section.

Install the new APK over the existing Cyn app to preserve model, labels and settings.
Do not uninstall first. The signing key and its encrypted password are excluded from
Git and source ZIPs. This remains a preview, not a claim of universal system-wide
Android processing or real-song model accuracy.
