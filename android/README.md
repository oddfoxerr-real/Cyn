# CYN Studio Android 0.2.9 preview

Android 9+ on 64-bit ARM or x86. The app is named **CYN Studio**, package
`io.github.oddfoxerr.cyn.studio`, version code 13. It installs alongside the older
Cyn app because that app's original signing key could not be recovered. Keep the
old app to preserve its settings and learning; Studio starts with separate data.
Disable the old app's audio monitoring before enabling Studio to avoid competing
effects. Future Studio releases retain this identity and signing key.

Download `Cyn-Studio-Android-0.2.9-preview.apk` from the
[Studio release](https://github.com/oddfoxerr-real/Cyn/releases/tag/android-studio-v0.2.9-preview),
open it on your phone and confirm installation. Do not uninstall the old Cyn to
install Studio.

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
- Automatic Studio-channel release checks approximately every 12 hours, respecting
  Android background scheduling. Wi-Fi/unmetered download by default; manual check
  may download on mobile data. APK size, SHA-256, package, increasing version code
  and current signing certificate must verify before install. Android confirms the
  installation. Legacy Android and Windows releases are never selected. Background
  checks can be disabled.

## AI-controlled Sound Studio

Sound controls display independent AI target rings. The primary slider is
always the user's manual setting. Live rings are backed by applied native
DSP values for local playback; on compatible third-party sessions, only bass
and clarity offsets supported by the Android EQ are shown. No ring appears
when neural updates are absent, unavailable, or turned off. Animations respect
reduced motion.

## Connect CYN visuals

CYN requests permission before displaying measured playback visualizations.
When individual app sessions are unavailable, it tries Android's output-mix
Visualizer. No device microphone is opened. External audio may still be
unavailable on restricted or incompatible Android devices. The interface
then reports the limitation rather than generating simulated activity.
The visualizer stops on disconnect; model activity appears only when
measured audio features are available. This does not bypass Android's
third-party audio processing restrictions.

## CYN as a music AI assistant

Smart Listening is the primary feature. The Sound Studio shows measured audio
and its actual processing state for compatible external sessions. Audio Library
is a bonus local player, not a required input or the focus of the app.

The app does not automatically collect device model, manufacturer, Android
version, media session IDs or app identities in connection details. Optional
on-device diagnostics are anonymized by default. Measured Visualizer FFT data is
processed on the phone and not uploaded. Android's playback-session and system
mix limitations still apply; no root or covert recording is requested.

The live spectrum interpolates measured source frequency bins only and fades
when samples stop. Personal neural animations use recorded activations and
weighted connections, never artificial inference telemetry. Output percentages
appear only for trained personal preferences. The untrained fallback for
compatible external sessions is conservative spectral adaptation, not YAMNet.
The complete native YAMNet + CYN PCM chain remains a separate path for
locally played tracks. This does not imply mobile interception of every app.

## Music-app connection

Tap **Connect CYN**, then start playback in a compatible music app, and
choose CYN as the app's equalizer if offered. The status now distinguishes
monitoring from a real session with Android EQ control. A connected effect does
not establish that another player routes through it. **Disconnect CYN**
stops external monitoring. **CYN enhancement** separately disables all effects
without silently being re-enabled when monitoring starts.

For phones that do not expose sessions, **Try device-wide EQ (experimental)**
requests the legacy output-mix effect (session 0), after a warning. Android may
refuse it, or the player may ignore it. Compare music with the toggle off and
on; unsupported apps cannot be forced through CYN. Granting RECORD_AUDIO permits
session visualization where supported; it does not grant global audio interception.

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
the alias is `cyn-studio`. The registered Studio certificate SHA-256 is
`045d9c122b2158d569c0ef8d10bbb08496a9f19e7397b702e99e9330c780bcb7`.
Without that key, `assembleRelease` produces an unsigned APK that is **not** an
installable update. Debug APKs are for testing and do not update production installs.

GitHub Actions builds source, runs DSP/model/lint checks, retains build artifacts and
publishes only after signature and package verification. The registered Studio key
is stored in encrypted repository secrets `CYN_STUDIO_ANDROID_KEYSTORE_BASE64` and
`CYN_STUDIO_ANDROID_KEY_PASSWORD`. An access-restricted local signing backup is held
outside the repository. Do not commit or include keys/passwords in source archives.

On the account holding the Studio signing backup, with JDK 17 and GitHub CLI signed
in to the repository owner, run from the updated repository:

```powershell
.\android\scripts\configure_release_key.ps1 -SigningDirectory 'C:\path\to\studio-signing-backup' -Publish
```

The helper verifies the registered Studio certificate, stores the key and password in the
two encrypted GitHub Actions secrets, then starts the checked release workflow.
The backup contains `cyn-studio.jks`, `password.dpapi` and a protected recovery
password for restoring access after a Windows-account change. Keep a secure copy
outside this PC. The helper never generates a replacement key or prints secrets.
Source code and unsigned build artifacts cannot replace the registered private key.

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

## 0.2.9 reversible Sound Control motion

The coloured outlined ring anchors the user's saved manual setting. The **SeekBar thumb** follows current, actually applied AI DSP targets during local playback (and supported bass/clarity adjustments on compatible external sessions); it slides back to the ring on AI-off. This behavior covers Bass, Vocal clarity, Stereo width, Ambience, Dynamics and every EQ band (31/35/39). Output headroom has the same fixed manual ring but remains manual-only. Dragging the thumb adjusts and saves the user's own choice; AI updates never save the thumb's temporary position.

## One-button update flow (0.2.9)

Style → Updates has one **UPDATE CYN** action and one optional **Automatic updates** switch. Tapping Update opens the native mechanical two-tone dripping splash. The progress fill uses actual bytes downloaded (0–100); search and signature verification show non-quantified activity. The same screen offers INSTALL VERIFIED APK only when a newer signed APK has been downloaded and checked. Automatic mode uses periodic JobScheduler checks and downloads on unmetered connections; Android always requires user approval for installation. Dismissing the splash leaves an active download running, and a notification can take the user back to Updates when ready.

## 0.2.9 Fox sliders and upstream credit

All studio controls that use sliders render a lightweight two-tone vector fox attached to the real SeekBar thumb. Only genuine user/AI position changes trigger a short bounce/tail movement; Reduce motion renders a static fox. The existing AI/manual ring separation and sound DSP bindings remain unchanged.

The Style → CYN for Android → **FxSound credits, licenses & no warranty** screen shows the project's upstream authorship, original FxSound team links, retained AGPL-3.0-or-later terms, individual CYN modifications, source/build location and no-warranty disclaimer. This is factual attribution; it does not change copyright ownership or transfer responsibility between independent maintainers.

## Material 3 layout (0.2.9-preview)

CYN now uses Google's Material Components 1.13.0 as its Android UI foundation: a fixed Material 3 DayNight theme, MaterialButton actions/navigation, MaterialSwitch toggles, MaterialCardView surfaces, and Material-styled dialogs. Four vector icon tabs keep the Studio / AI / Library / Style flow. Palette choices intentionally use CYN colors instead of Android wallpaper-based Dynamic Color. Fox sliders and AI manual/target rings remain native CYN views; the liquid page transition remains available except when Reduce motion is enabled. All audio and updater behavior is unchanged.

## 0.2.9 navigation hotfix

Material 3 tabs now use the real BottomNavigationView to measure icons and labels separately, avoiding two-line or three-line button text on narrow phones. The hero spectrum is compact while idle and full-height during playback or connected monitoring. No theme/AI/audio/update behavior changed; regression tests verify visible, single-line labels on a 360 dp display.
