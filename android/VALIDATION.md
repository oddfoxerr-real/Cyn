# Android 0.2.0 preview validation — 2026-10-19

The source adds a phone sound studio, persistent library, native PCM effects,
desktop ONNX inference and verified automatic Android updates. Release publication
requires all workflow gates plus the original Android signing certificate.

## Completed local checks

- Native C++ DSP tests passed: exact settled bypass, 31/35/39 bands, mono/stereo,
  silence, bounded boosted output, feature snapshots and both neural model windows.
- Existing pure-Java learning and Android release tests: 285 assertions passed.
- Java APEX preprocessing matches an independent NumPy STFT within 2e-5.
- Actual desktop YAMNet and APEX model inference returned finite, correctly shaped
  outputs. Weights are SHA-256 pinned by `scripts/prepare_models.py`.
- Earlier development builds compiled; the final ONNX Runtime 1.23.2 build, lint,
  device instrumentation and release signing are verified by the release workflow.

## Reproducible release gates

`.github/workflows/android-release.yml` builds from source, runs the checks above,
then exercises Android 15 with real native/ONNX loading, 44.1-to-48 kHz playback,
seek/pause, favorites, rapid navigation and four captured phone screens. Reports
are retained with the run. A failed gate prevents publication.

The final certificate must be
`20698db6b762137a5e5199c0c5a6c728e8b90374d9dc76973c6ea21c005ee13d`.
Package `io.github.oddfoxerr.cyn`, version code 4, minimum API 28, target API 35.
An unsigned build artifact is not an installable update.

## Remaining physical-device checks

Headphone/speaker listening, Bluetooth routes, power use and inference latency on
real ARM phones; background update scheduling; installation over 0.1.2; TalkBack,
large fonts and landscape; and third-party playback-session compatibility.
Numerical, build and emulator checks do not establish perceptual Windows parity.
Full neural processing is confined to CYN library playback. The existing Windows
virtual driver and exact Windows DSP are not included in Android.
