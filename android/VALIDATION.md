# Android 0.2.0 preview validation — 2026-10-19

The source adds a phone sound studio, persistent library, native PCM effects,
desktop ONNX inference and verified automatic Android updates. Release publication
requires all workflow gates plus the original Android signing certificate.

## Completed build and functional checks

[Workflow run 39256987103](https://github.com/oddfoxerr-real/Cyn/actions/runs/39256987103)
checked application commit `416031cc36319998c0005b543549d1b3e73f3048`.

- Native C++ DSP tests passed: exact settled bypass, 31/35/39 bands, mono/stereo,
  silence, bounded boosted output, feature snapshots and both neural model windows.
- Existing pure-Java learning and Android release tests: 285 assertions passed.
- Java APEX preprocessing matches an independent NumPy STFT within 2e-5.
- Actual desktop YAMNet and APEX model inference returned finite, correctly shaped
  outputs. Weights are SHA-256 pinned by `scripts/prepare_models.py`.
- Release, debug and instrumentation APK builds and Android lint passed with
  ONNX Runtime 1.23.2.
- Android 15 emulator instrumentation passed: **4 tests, 0 failures, 0 errors**.
  This covered native/model loading, exact disabled PCM bypass, 44.1-to-48 kHz
  library playback, seek/pause, persistent favorites and rapid page transitions.
- Four phone-page captures were checked in touch mode after transitions completed.
  Pixel assertions verified the header, mini-player text/play control and every
  navigation tab on each page. The captured header and mini-player pixels are
  identical across all four pages. The screenshots use Android PixelCopy to capture
  CYN's actual window without unrelated emulator launcher dialogs.
- Liquid transitions draw their bitmap mesh in a separate overlay view. Cancelled
  transitions remove that view; Android retains bitmap ownership until rendering
  has finished. Reduced motion skips the overlay.
- Independent inspection of the earlier `4464fe9` unsigned APK confirmed that all
  six native libraries across ARM64 and x86_64 have 16 KB ELF and APK alignment.
  Both real desktop ONNX model assets were present. Native libraries and build
  configuration are unchanged in `416031c`.
- The Windows signing setup helper passed PowerShell parser validation. Its actual
  key upload requires the original Windows account and has not been executed here.

## Release status

The workflow stopped at **Verify original signing key and sign update** because
`CYN_ANDROID_KEYSTORE_BASE64` and `CYN_ANDROID_KEY_PASSWORD` were missing. Build,
lint and device gates passed before that step. **No signed 0.2.0 APK was published.**

From the updated repository on the original Windows account, with JDK 17 and an
authenticated GitHub CLI, run:

```powershell
.\android\scripts\configure_release_key.ps1 -Publish
```

The helper verifies the original certificate, configures the encrypted repository
secrets and starts the release workflow. It requires the existing
`android/.signing/cyn-android.jks` and `password.dpapi`; use `-SigningDirectory`
if those files are in an older checkout. A replacement key cannot update existing
installations.

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
