# CYN Studio v0.2.8-preview — Material 3 Redesign

- Rebuilds CYN Mobile's shared UI on Google's Material 3 Android components: **MaterialButton**, **MaterialCardView**, **MaterialSwitch**, and **MaterialAlertDialogBuilder**, with a Material 3 DayNight base theme.
- Modern compact bottom navigation with four original vector icons, tonal selection, and matching labels for Studio, AI, Library and Style.
- Larger rounded cards, tonal surfaces, tactile elevated-looking action buttons, ergonomic switch rows and roomy touch targets. The main Connect CYN and Update CYN actions use a distinct primary filled button.
- Four **fixed CYN palettes** remain selectable. **No wallpaper-driven or dynamic colors**, location services or wallpaper permissions.
- Preserves CYN's real audio controls, draggable fox sliders, AI baseline rings, native library, liquid page transition, local models, update flow and source/license notices.
- Reduces motion if the user's Reduce motion preference is on, and keeps screen-state navigation and ongoing playback unchanged.

# CYN Studio v0.2.7-preview — Fox Sliders & FxSound Credits

- A stylized small vector fox rides every manual and AI-controlled slider thumb, including Bass, Vocal clarity, Space & Dynamics, all 31/35/39-band EQ sliders, and AI sensitivity. The fox bounces and wags its tail when actual slider positions move; it never fabricates AI activity or changes sound values.
- The fox animation runs briefly only during real thumb motion and respects Reduce motion. No external images or heavy renderer are required.
- The built-in **FxSound credits, licenses & no warranty** panel now clearly attributes original FxSound DSP and source copyrights to FxSound LLC and original contributors, while identifying CYN-specific changes separately.
- GNU AGPL-3.0-or-later license remains unchanged, with original code notices and applicable third-party licenses preserved. Clarifies that attribution is not an endorsement, transfer of liability, or release from applicable obligations.
- App package and signing identity are unchanged.

# CYN Studio v0.2.6-preview — Mechanical Update Splash

- One prominent **UPDATE CYN** button in Style → Updates replaces the separate check/download and install actions.
- Opening Update CYN displays a full-screen native two-tone 2D mechanical splash with clamp rails, cogs and dripping fill. Download fill tracks **actual byte progress**, while checking and verifying show activity without fabricated percentages.
- A single context-sensitive action inside the splash handles retry/check/download or installation of a verified APK. Closing the splash never cancels an already running background download.
- One **Automatic updates** toggle combines scheduled release checks and Wi-Fi-only downloads. Checks use Android JobScheduler and notify users when a newer APK is ready.
- Releases are still validated against APK SHA-256, exact package name, monotonically newer version and the installed app's signing certificate; Android's permission and install confirmation remain required.
- Respects reduced motion, preserves the existing application package and signing certificate, and leaves audio settings and personal learning untouched.

# CYN Studio v0.2.5-preview — AI-driven slider thumbs

- The outlined ring now stays at **your** manual sound setting. The physical draggable SeekBar thumb animates toward CYN's actually applied live AI target, then returns to your setting when AI turns off or its live target becomes unavailable.
- Extends the reversible UI to all Sound Controls, stereo width, ambience, dynamics and every 31/35/39-band equalizer slider, including inside expandable sections.
- Headroom receives the consistent outlined manual setting marker; it stays manual because CYN's AI does not automatically adjust headroom.
- Moving AI thumbs **never overwrite** manual settings, trained preferences or presets. Dragging temporarily gives your finger control and updates your manual baseline; the thumb returns to live AI motion after release.
- Uses native DSP targets for CYN Library; other Android apps are limited to compatible, actually controlled session EQ effects and cannot claim library-only effects are applied.
- Respects reduced-motion settings and adds instrumented UI state tests.

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
