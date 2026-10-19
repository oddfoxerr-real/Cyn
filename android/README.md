# CYN Android 0.2.0 preview — Sound Studio

Android 9+ on 64-bit ARM or x86. Package identity stays `io.github.oddfoxerr.cyn`,
version code 4. Install a correctly signed release over the existing preview to
keep settings and learning. Never replace the original signing key.

## What changed

- Phone Sound Studio: near-black cards, four new palettes, persistent transport,
  actual-content liquid page distortion, reduced motion, accessible touch targets.
- Persistent audio library: Android document picker or permissioned MediaStore scan,
  track/artist/album search, favorites, queue playback, previous/next, shuffle and repeat.
- Media3 playback through CYN's native PCM processor: 31/35/39 EQ bands, bass,
  midrange clarity, width, ambience, dynamics and linked output-peak protection.
  Playback is resampled to 48 kHz for the desktop APEX model. No system-volume changes.
- Same checksum-pinned desktop YAMNet and full APEX-6M ONNX weights. APEX has
  6,307,477 parameters and uses the existing 225-step synthetic checkpoint; no new
  training or unverified quality score. YAMNet is the lighter default; APEX is opt-in.
- Desktop pre-effects feature extractor, learned spectral-role model, scene mixer
  and sensitivity range code are compiled into the app. Provenance and hashes live
  in `app/src/main/cpp/desktop/provenance.json`.
- Local personal feedback supports all five effects during library playback, with
  desktop `.cynai.json` preference-model import/export and measured neural telemetry.
- Automatic Android-channel release checks approximately every 12 hours, respecting
  Android background scheduling. Wi-Fi/unmetered download by default; manual check
  may download on mobile data. APK size, SHA-256, package, increasing version code
  and current signing certificate must verify before install. Android confirms the
  installation. Windows releases are never selected. Background checks can be disabled.

## Audio scope

Full neural processing applies to audio played **inside CYN's library**. Other apps
must expose a compatible Android audio-effects session; only supported native Android
session effects can apply there. Spotify, browsers and DRM audio are not guaranteed.
The Windows FxSound virtual driver and exact DSP implementation are not in this APK.
Same model weights and mixer decisions do not establish identical audible output or
listening quality across the two platforms. This is a preview, not a certified parity
release. Unsupported formats report playback errors without claiming active processing.

Inference runs on one background worker with two ONNX compute threads. Stale, missing,
silent or disabled AI returns control to the manual settings. Personal training is
explicit and local. The app uses no microphone capture, root, cloud audio upload or
forced routing. Library files are read using Android access grants. External-session
visualization needs `RECORD_AUDIO`; library analysis does not.

## Build

Use JDK 17, Android SDK 35, build-tools 35.0.0, NDK 27.2.12479018 and CMake 3.22.1.
No Android Studio is necessary. From the repository root:

```sh
python3 android/scripts/prepare_models.py
cd android
./gradlew :app:assembleDebug :app:lintDebug
```

The model script downloads official existing CYN release assets and verifies fixed
SHA-256 values. Models are generated build inputs, not committed duplicate binaries.
Gradle dependencies: Media3 1.5.1, AndroidX Core 1.15.0, ONNX Runtime Android 1.23.2.
The native engine and ONNX libraries support 16 KB page alignment.

For release signing, provide `CYN_KEYSTORE` (absolute JKS path) and `CYN_KEY_PASSWORD`;
the alias remains `cyn-android`. The original certificate SHA-256 is
`20698db6b762137a5e5199c0c5a6c728e8b90374d9dc76973c6ea21c005ee13d`.
Without that key, `assembleRelease` produces an unsigned APK that is **not** an
installable update. Debug APKs are for testing and do not update production installs.

GitHub Actions builds source, runs DSP/model/lint checks, retains build artifacts and
publishes only after signature verification. Configure the existing original key in
repository secrets `CYN_ANDROID_KEYSTORE_BASE64` and `CYN_ANDROID_KEY_PASSWORD`.
The old key was created by `BUILD_ANDROID.ps1` in `.signing/cyn-android.jks`; its
DPAPI password file is tied to the original Windows account. Do not commit keys.

On that original Windows account, with JDK 17 and GitHub CLI installed and signed
in to the repository owner, run from the updated repository:

```powershell
.\android\scripts\configure_release_key.ps1 -Publish
```

The helper verifies the original certificate, stores the key and password in the
two encrypted GitHub Actions secrets, then starts the checked release workflow.
Use `-SigningDirectory 'C:\path\to\original\android\.signing'` if the original
key lives in an older checkout. The helper never generates a new signing key and
does not print either secret. It needs the actual original key files; source code
and unsigned build artifacts cannot replace them.

## Verification

Recorded build, emulator and signing results are in [VALIDATION.md](VALIDATION.md).

`tests/native/engine_test.cpp` exercises real DSP: settled exact bypass, worst-case
boosts, mono/stereo, silence and model windows. `tests/AudioModelTest.java` checks
training/inference and Android release ordering. `tests/ApexFeaturesTest.java` plus
`scripts/test_models.py` compare Java preprocessing to an independent NumPy FFT and
run both real ONNX graphs. `StudioTest` exercises Android navigation, actual native
model loading, playback/resampling, seek/pause, and favorite persistence.

Physical-phone listening, Bluetooth routes, external-player compatibility, background
scheduling and install-over-old-version require device validation. No physical-phone
or perceptual sound-quality result is implied by build or numerical tests.
