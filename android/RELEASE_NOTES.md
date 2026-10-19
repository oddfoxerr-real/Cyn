# CYN Studio Android 0.2.4 preview

Download **Cyn-Studio-Android-0.2.4-preview.apk**, open it on your phone and confirm
installation. Requires Android 9+ on a 64-bit ARM or x86 device.

This installs as **CYN Studio** alongside the old **Cyn** app. The old signing key
was unavailable, so Studio has a new app identity and a preserved release key for
future updates. Your old app and its data remain intact; Studio starts with its own
settings and learning. Turn off monitoring in old Cyn before enabling Studio.

- Sound Studio now displays a second, animated AI target marker for bass,
  vocal clarity, width, ambience, dynamics and every 31/35/39 EQ band.
  Markers follow actual parameters applied to CYN's native sound engine.
  Compatible external audio only shows adjusted bass/clarity when Android
  grants EQ control and live audio analysis is available.
- Manual sliders remain independently touchable. AI markers disappear when
  Smart Listening is disabled or when no current model adjustment exists.
- Connect CYN now offers the permission needed for live audio visuals.
- Added Android output-mix visualizer fallback for players without individual
  audio-effect sessions. Live spectrum and neural activity use measured audio,
  or report when Android provides no usable playback measurements.
- External visualizations stop on disconnect; reduced-motion still works.
- Smart Listening is the primary mobile experience; the audio library is optional.
- Reworked Connection details to show a concise, identity-free on-device status instead
  of phone model, manufacturer, Android version, player names and session identifiers.
- Added a measured, audio-reactive 31-band spectrum for library playback and compatible
  external audio with granted visualization permission, with honest silence/stale decay.
- Neural connections now interpolate real personal-model activations, with strength-based
  moving impulses and a truthful inactive state when no model measurements are available.
- Exposed the actual processing path and AI adjustment telemetry. Compatible external
  audio gets conservative spectral auto-EQ when the personal model is not trained.
  External playback does not use CYN's internal native YAMNet PCM DSP.
- Fixed the music-app connection button: it now distinguishes monitoring, no session,
  incompatible sessions, controlled Android EQ and device-dependent compatibility mode.
  Connecting no longer silently re-enables enhancement after switching it off.
- Added a visible, explicitly confirmed experimental system-mix EQ option and clearer
  diagnostics. This cannot bypass Android restrictions or guarantee Spotify/browser audio.
- Redesigned phone studio, four new dark palettes, content-warp liquid transitions,
  reduced motion and persistent mini-player.
- Audio library with multi-file import, device scan, search, favorites, queue,
  previous/next, shuffle/repeat and background playback.
- Native 31/35/39-band library DSP with bass, vocal clarity, space, dynamics and
  output protection. Desktop pre-effects analysis, scene mixer and sensitivity code.
- Offline YAMNet and optional full APEX 6M using verified desktop model weights.
  All five personal preference outputs, local training and desktop model import/export.
- Automatic verified Android updates, Wi-Fi downloads and Android install confirmation.

Full neural processing applies to CYN library playback. Other apps depend on Android
session support. The phone DSP is adapted for Android; identical Windows sound is not
claimed. APEX retains its existing experimental 225-step synthetic checkpoint.

Studio only downloads updates from its own release channel and checks package,
version, checksum and signing certificate before Android confirms installation.
The corresponding source ZIP and APK SHA-256 are attached to this release.

Signing certificate SHA-256:
`045d9c122b2158d569c0ef8d10bbb08496a9f19e7397b702e99e9330c780bcb7`.
