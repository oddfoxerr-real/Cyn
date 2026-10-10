# Android preview validation — 2026-10-10

Desktop base: `bcf2e41ce8842fe009049f1476565142338a3cf1` (v2.0.16).

Completed locally:

- JDK 17 / Gradle 8.11.1 / Android Gradle Plugin 8.9.2 / SDK 35 release build.
- `:app:assembleRelease :app:lintRelease`: successful; **0 lint errors**.
  Five warnings concern English-only text formatting/localization. The preview UI is English.
- Pure-Java host test: **32 assertions passed**. Covers finite silence/invalid inputs,
  low/high-frequency discrimination, unsigned waveform decoding, bounded model outputs,
  sensitivity limits, real neural optimization and learned More/Less separation,
  insufficient-label rejection, completed epochs and training-copy isolation.
- Android `apksigner verify --verbose --print-certs`: signature verified, APK v2 signing,
  RSA 3072. Certificate SHA-256:
  `20698db6b762137a5e5199c0c5a6c728e8b90374d9dc76973c6ea21c005ee13d`.
- Android `aapt dump badging`: package `io.github.oddfoxerr.cyn`, versionCode 1,
  versionName `0.1.0-preview`, minSdk 28, targetSdk 35. Launcher is MainActivity.
  No Internet permission. Microphone hardware is optional.

Not validated: installation, rendering, actual audio, battery behavior, Bluetooth,
notifications or external music-app compatibility on a physical Android device.
`adb devices` returned no connected devices. There was no emulator available.
No claim of Android or real-song model accuracy, full desktop feature parity,
universal external-app processing or a finished stable mobile release is made.

Build and signing happen locally. This task does not publish an APK to GitHub or a
store, install software onto a phone, change Windows audio routing or run an installer.
