# CYN Android external-session routing recovery — 2026-10-25

This is a **source and test branch**, not an app update or an APK release.
Base: `feature/android-cyn-graphite-settings-no-capture-20261025`
(CYN Studio Android 0.2.29-preview, versionCode 33).

## Root cause

The previous `Chain.apply()` enabled an external equalizer only when
`externalSessionCount() == 1`, regardless of the available playback-app
metadata. A second session (including an unrelated paused app) could disable
every external effect. The broadcast receiver also discarded
`AudioEffect.EXTRA_PACKAGE_NAME`, so it could not associate an effect session
with the media application that opened it.

## Repair

- Retain session package identifiers **in memory only** from documented Android
  `ACTION_OPEN_AUDIO_EFFECT_CONTROL_SESSION` broadcast extras.
- With separately authorized Notification Access, read the packages of
  currently *playing* media sessions, never notification bodies.
- Match each external AudioEffect session to its playing app. Multiple eligible
  sessions now work independently; unmatched sessions remain disabled.
- An old player omitting the package extra still has the original sole-session
  fallback, but only when exactly one media app is playing.
- Preserve explicit enhancement-off, audio-library separation and the
  separate user-selected experimental system-mix mode.
- Debounce audio-device changes (headsets/Bluetooth/USB) then reinitialize
  existing external effect objects without restarting the application or
  modifying saved preferences.
- Surface the Android Notification Access setup action in Connect CYN.
- Keep screen recording / MediaProjection disabled for Connect CYN.
- Keep Studio DSP, manual EQ, APEX, AUDONIC and BrainScan paths intact.

## What this does *not* do

Android media metadata does not expose another app's audio session ID. Its
AudioEffect broadcasts are optional, and Android or other equalizers can deny
control even when a session ID is present. A working Equalizer object therefore
does **not** prove that an unrelated app routes sound through it.

Wireless ADB debugging can help the owner inspect playback/session behavior and
supported settings. It does not turn CYN into a privileged system audio driver,
unlock another app's DRM audio, or make screen capture unnecessary for arbitrary
PCM access.

## Tests

Run the host-only routing regression (no emulator required):

```sh
mkdir -p /tmp/cyn-session-policy
javac -d /tmp/cyn-session-policy \
  android/app/src/main/java/io/github/oddfoxerr/cyn/SessionEffectPolicy.java \
  android/tests/SessionEffectPolicyTest.java
java -cp /tmp/cyn-session-policy io.github.oddfoxerr.cyn.SessionEffectPolicyTest
```

The dedicated GitHub Actions workflow also assembles a debug APK and runs
Android lint after preparing verified model assets.

**Physical Android QA still required:** enable CYN media controls in Android
Notification Access; open a player that exposes AudioEffect session broadcasts;
compare CYN enhancement off/on while music plays; switch between wired,
Bluetooth and phone speaker outputs; try two simultaneously reported sessions;
confirm manual DSP settings, updater identity and Android notification
on/off controls survive. Test a non-compatible player too: CYN should report
limitations rather than falsely claim system-wide processing.

No signing key or release version is changed by this branch.
