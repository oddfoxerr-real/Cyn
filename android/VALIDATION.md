# Android 0.1.2 preview validation — 2026-10-12

Built locally with JDK 17, Gradle 8.11.1, AGP 8.9.2 and Android SDK 35.

- `:app:assembleRelease :app:lintRelease` succeeds. Zero lint errors; five
  existing English localization warnings.
- Pure-Java host tests: **285 assertions passed**, covering audio features,
  learned More/Less separation, inference/telemetry equivalence, connection
  contributions, candidate-training snapshots and Android-only version selection.
- Package: `io.github.oddfoxerr.cyn`, versionCode **3**, versionName **0.1.2-preview**,
  minimum SDK 28, target SDK 35. Original package identity retained.
- APK signature verified with `apksigner`. Original release certificate SHA-256:
  `20698db6b762137a5e5199c0c5a6c728e8b90374d9dc76973c6ea21c005ee13d`.

## Interface changes

Music is the default screen. Its primary Enable Cyn action starts monitoring and
turns effects on together. Disable Cyn bypasses effects and stops monitoring without
stopping the file player. Connection status separates waiting, EQ control and
unavailable effects; repeated refreshes do not repeatedly announce unchanged state.
Opening Cyn from a player's equalizer panel returns to Music.

The local player is in Files, with a dynamic Play/Pause button and seek control.
Music retains bass, clarity, presets, EQ and personal adjustments. Connection help
contains setup steps, diagnostics and optional legacy compatibility. Learn contains
the learning timeline; More contains themes, updates and credits. Navigation uses
flexible height, short labels and a selected state. The primary action uses a
contrasting foreground, and buttons expose their role to accessibility services.

The update checker, legacy compatibility, connection diagnostics and measured neural
activity from the local 0.1.1 preview are retained in this source revision.

## Device checks still required

No Android device was connected and no emulator was installed. Build/lint and host
tests do not verify rendered layout or device audio. Check a small phone, landscape,
large fonts, TalkBack, light/dark themes and rotation. Exercise enable/disable before
and after a music app opens a session, competing equalizers, unsupported sessions,
permission denial, notification Stop and switching tabs during file playback.

Galaxy A56/Spotify routing, legacy output-mix support, playback/Bluetooth and updating
an existing install need physical-phone testing. EQ ownership reports Android's API
state; it does not prove that a specific player's audio passes through that effect.

Install over the existing Cyn app to retain model, labels and settings. The signing
key and encrypted password are excluded from Git and the source archive. This is an
Android preview, without a claim of universal system-wide processing or measured
real-song model accuracy. Internet access is only for explicit GitHub update checks;
audio, numerical features, model weights and feedback are not uploaded.
