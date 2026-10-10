# Cyn for Android — 0.1.0 preview

An installable, signed Android app based on the current Cyn repository at
`bcf2e41ce8842fe009049f1476565142338a3cf1` (desktop v2.0.16). The Android code is
independent of the Windows build and does not replace or change the desktop app.
Requires **Android 9 or later**. This first mobile preview needs real-phone audio
and layout testing; compilation and host tests cannot establish device compatibility.

## Install and use

1. Transfer `Cyn-Android-0.1.0-preview.apk` to your phone and open it in Files.
2. If Android asks, allow that Files/browser app to install unknown apps, then install.
3. Open Cyn → **Studio → Open audio file**. Choose music through Android's file picker.
   Play/pause and seeking work inside Cyn; the notification also offers Pause and Stop.
4. For another player, open **Other music apps → Enable compatible-app enhancement**
   before starting/reopening its playback. If that player has an equalizer selector,
   select Cyn. The Studio status lists the session and effects actually available.
   Stop the monitor from Cyn or its notification when finished.

Other players must expose an Android audio-effects session. This is **not universal
system-wide capture** and does not guarantee Spotify, browsers, DRM playback or any
specific phone. No root is required. Audio session 0 is deliberately not used.
Enabling the monitor does not take audio focus away from another app. The built-in
player requests focus, pauses for calls/other focus owners and headphone disconnects,
and follows Android's chosen output. Cyn never sets system volume or forces routing.

## Features in this APK

- Cyn branding; desktop palettes adapted to mobile, including **Charcoal Desktop**.
- Single bottom navigation, rounded cards, expandable animated categories, reduce motion.
- Local-file playback, background playback notification and seek control.
- Compatible-app session enhancement while the user-enabled foreground monitor runs.
- Bass, clarity, a five-point EQ curve mapped to the device's native bands, optional
  Android virtualizer width and loudness enhancement where the session supports them.
- Local neural learning for **bass and clarity**, selectable feature timeline,
  More/Less labels, training progress, sensitivity and actual hidden-neuron activity.
- Persistent effects/theme/model/feedback settings; no network permission or uploads.
- In-app AGPL text, FxSound credit/support link and corresponding-source notice.

## Smart listening

Turn on Personal adjustments and allow audio analysis. Android requires its
`RECORD_AUDIO` permission to use Visualizer on a playback session; Cyn does not open
the microphone or use AudioRecord. Declining permission leaves manual effects working.

Play varied music, open Learning, drag across a section (or select the last five
seconds), choose Bass or Clarity, then label More or Less. Supply at least five frames
in each direction for an effect, then Train my model. Adjacent frames are correlated;
label sections from several different songs instead of treating them as independent
examples. Training runs off the UI thread with 240 real optimization epochs. Saving a
new label for the same frame/effect replaces its old label. Up to 512 labels are kept.
Resume live timeline unfreezes your selection.

The model is a Java adaptation of Cyn's 11→12→5 neural network (209 parameters).
Only the Bass and Clarity heads control Android effects in this preview. User feedback
trains real weights; it is not a set of fixed bass/vocal detection rules. The initial
personal model is inactive until trained. The optional v2.0.16 desktop starter is
included, explicitly experimental, and loaded only on request. It was trained using
synthetic audio, not a real-song preference dataset.

**Android analysis differs:** Visualizer provides a low-resolution mono spectrum
which can include applied effects. Desktop Cyn uses pre-DSP biquad features. Stereo
width is unavailable, and the Android band estimator differs. Desktop accuracy scores
do not transfer to this APK. No Android listening evaluation has been performed.
Live changes are bounded and smoothed; turning Smart listening off restores manual
settings. Sensitivity zero prevents automatic changes. A manual Bass/Clarity baseline
of zero also prevents automatic boosting of that effect. On missing analysis the
adjustments return to the baseline rather than manipulating system volume.

The rolling history contains at most 60 seconds of numerical features, not recordings.
Learned weights and labeled features live in app-private `learning.json`; reset or
uninstall removes them. Cloud backup and device transfer are disabled for this data.

## Differences from Windows

This is an Android adaptation, **not the FxSound Windows DSP/driver in an APK**.
Android's Equalizer/Virtualizer/LoudnessEnhancer implement the effects. Available bands
and effects vary by device/session. Loudness is manual gain, not desktop Dynamic Boost
or automatic volume leveling. Desktop ambience, driver routing, CLI/MCP, AI Hub sharing,
desktop model exchange and automatic app updates are not included. The Android source
does not remove any Windows features. One local audio file plays at a time; there is
no library scanner, streaming login or playlist manager in this preview.

## Build on this PC

From the `android` folder in PowerShell:

```powershell
.\TEST_ANDROID.ps1
.\BUILD_ANDROID.ps1 -Offline
```

The script uses the already installed JDK 17, SDK 35 and Gradle 8.11.1 in
`C:\Users\wiese\Documents\Codex\2026-10-05\design-m\work\android-build` when environment
variables are absent. Result: `app\build\outputs\apk\release\app-release.apk`.
Run without `-Offline` if Gradle dependencies have not been downloaded.

On another Windows PC, install JDK 17 and Android SDK platform 35/build-tools 35.0.0,
accept their licenses, set `JAVA_HOME` and `ANDROID_HOME`, and run the same commands
without `-Offline`. The wrapper downloads checksum-pinned Gradle 8.11.1.
`BUILD_ANDROID.ps1 -DebugBuild` produces a development APK.

On Linux/macOS with JDK 17 and SDK 35 configured:

```sh
chmod +x gradlew
./gradlew :app:assembleDebug :app:lintDebug
mkdir -p app/build/host-tests
javac -d app/build/host-tests app/src/main/java/io/github/oddfoxerr/cyn/PreferenceModel.java app/src/main/java/io/github/oddfoxerr/cyn/AudioFeatures.java tests/AudioModelTest.java
java -cp app/build/host-tests io.github.oddfoxerr.cyn.AudioModelTest
```

The release script creates a private signing key once in `.signing/cyn-android.jks`
and encrypts its password for the current Windows account in `password.dpapi`.
**Keep this key and a recoverable password backup private.** Android updates must use
the same signing key and a higher versionCode. The password file is not portable to
another Windows account. Neither signing file is committed or included in source ZIPs.
No APK is automatically published by the build.

## License and provenance

Cyn Android additions and adapted Cyn code/model/palettes: AGPL-3.0-or-later; see
`LICENSE` and `app/src/main/assets/NOTICE.txt`. Source is provided alongside the APK.
Thanks to FxSound LLC and contributors: https://www.fxsound.com/.

The APK contains no Gilroy, JUCE, Microsoft runtime, Windows driver, driver installer,
or third-party music. Fonts and audio implementations come from the Android platform.
Gradle wrapper build tooling has its Apache-2.0 license and notice included. Android
SDK/JDK tools are installed separately. This Android package does not resolve the
separate distribution questions documented in the Windows licensing-audit branch.

Platform references:
- https://developer.android.com/reference/android/media/audiofx/AudioEffect
- https://developer.android.com/reference/android/media/audiofx/Visualizer
- https://developer.android.com/develop/background-work/services/fgs/service-types

## Device acceptance checks before a public stable release

Test on actual Android 9 and Android 15+ phones: file open/play/seek/completion, calls
and audio focus, headphone/Bluetooth changes, lock-screen playback, notification Stop,
unsupported formats, missing effect engines, permission denial/revocation, compatible
external-player open/close sessions, competing equalizers, model save/reset/retrain,
light/dark themes, small screens, large font sizes, and process recreation. Visualizer
and external effect compatibility cannot be established by an emulator or host test.
