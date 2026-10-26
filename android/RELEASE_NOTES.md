# CYN Studio Android v0.2.35-preview — Search music inside CYN

- Replaces the YouTube Music and SoundCloud launch-only buttons with an in-app search browser.
- Uses the official providers' own search pages, including song, artist and playlist exploration in CYN, with a search field and provider tabs.
- Allows opening a specific result and saving its exact track/playlist URL to CYN Library without favoriting it.
- Saved music links now reopen in CYN's search/browser screen instead of immediately launching another app.
- Saves through CYN's active Library service; new items appear in the Library without restarting the application.
- Includes explicit Open original fallback when a provider restricts embedded browsing, playback or sign-in.
- Preserves the v0.2.34 visible Remove control, independent Favorites, offline Audius downloads, local audio, Connect and AI models.
- This is provider-site browsing, not direct YouTube Music account sync or SoundCloud OAuth API access. Provider authentication restrictions and rights continue to apply.
- No Spotify sign-in, unofficial API extraction, streaming bypass or protected music downloads.

Build 39. The installer must pass Android build/lint, on-device tests, original signing identity and 0.2.34 to 0.2.35 upgrade verification.

---

# CYN Studio Android 0.2.34 preview — Music links and easy Library removal

- Adds a visible Remove action to every saved track, whether favorited or not.
- Removing a Library entry requires confirmation and never deletes original or downloaded audio files.
- Gives saved songs two-row cards, so titles no longer collide with heart and download actions.
- Adds YouTube Music and SoundCloud link bookmarks to CYN Library.
- Supports sharing an exact track or playlist link directly into CYN from another app.
- Can save a search shortcut for the currently playing YouTube Music or SoundCloud song when Android media-session access is enabled. This is not account library synchronization.
- Saved provider links open in their original music apps; they never enter the CYN audio player or offline downloader.
- Preserves independent Favorites, Library membership, Audius artist-permitted downloads, Android DSP, AI profiles and existing local presets.
- Does not add Spotify login or protected-provider offline downloads.

The release must pass Android build, lint, device tests, original signing-key verification and in-place upgrade from v0.2.33-preview.

---

# CYN Studio Android 0.2.33 preview — Library and layout polish

This update follows the signed 0.2.32-preview build. It preserves the existing Android package identity, trained LIBERTY asset workflow, AI profiles, Connect routing, native DSP, presets, and user data.

- Adds breathing room between stacked Material 3 action buttons in Settings and other pages.
- Separates Library, Favorites, and Downloads with dedicated Library filters.
- Adding an Audius track saves it to Library without making it a favorite.
- Favorites are toggled independently; removing a heart leaves the song in Library and keeps previously downloaded audio.
- Permitted Audius downloads no longer require favoriting a song first.
- Adds an Add-and-Download option beside Audius search results, with permissions verified by Audius for each download.
- Keeps offline music in Music/CYN when Android supports MediaStore storage.
- Adds regression coverage for saved but unliked music and for download eligibility.
- Does not add YouTube Music, SoundCloud or Spotify account integration in this release.

Release publishing requires passing build, lint and Android device tests, verifying the original signing certificate, and validating in-place upgrade from the signed 0.2.32-preview APK. Installation continues to require user approval.

---

# CYN Studio Android v0.2.32-preview — LIBERTY trained kick awareness

- Bundles the separately trained **LIBERTY kick recognizer** from the public Magenta Groove recorded-drum set (Creative Commons Attribution 4.0). Small 5,185-weight ONNX model, 90 measured training epochs.
- Held-out test kick recognition: F1 **0.7626** versus a simple onset baseline **0.5422**, using 55 independent test recordings. Model achieved 0.8769 precision and 0.6746 recall; these are *detection metrics*, not audible sound-quality scores.
- In **AI → LIBERTY**, adds an *off-by-default* **Experimental trained kick guard**. When Library music has fresh PCM features, confidently detected kick and low headroom, it may gently reduce low-frequency EQ by **up to 0.6 dB**, never boost, duplicate, synthesize or replace audio.
- **External music stays unchanged**: LIBERTY continues to use Android-compatible EQ routing and cannot run this trained PCM recognizer against third-party music without legal and technical access to audio. AUDONIC creative mode stays in its Library-only Creative Lab.
- Preserves the liked-song download/removal interface, APEX 6M/10M, AUDONIC, the CYN native engine, saved settings and in-app updater.

**Research limitations:** Training audio is drum performances, not complete mixed songs; no perceptual listening advantage is established. Headroom policy is hand-designed, not learned. The model remains experimental and disabled by default. Training workflow: https://github.com/oddfoxerr-real/Cyn/actions/runs/40233691628

**Attribution:** Google Magenta Groove MIDI Dataset v1.0.0, CC BY 4.0; Gillick et al., *Learning to Groove with Inverse Sequence Transformations* (ICML 2019), https://magenta.tensorflow.org/datasets/groove.

**Upgrade:** `io.github.oddfoxerr.cyn.studio`, versionCode **36** (0.2.32-preview). The signed APK must retain the existing Studio signing key and verify in-place upgrade from signed 0.2.31-preview.

# CYN Studio Android v0.2.31-preview — LIBERTY AI and Liked Music Downloads

- Adds **LIBERTY**, a separate non-generative AI mode using AUDONIC's existing on-device control model for CYN Library music. It applies adaptive EQ to original sound rather than triggering creative FX. External music receives bounded session-FFT-driven EQ adjustments **only where Android exposes a compatible, controllable effect session**. Inaccessible third-party audio remains inaccessible.
- Moves **AUDONIC** to a distinct **Creative Lab · CYN Library only** section so its experimental native creative DSP is no longer presented as a Connect enhancement mode. Selecting it disables external neural creative effects.
- Adds **Download liked** for Audius tracks whose metadata explicitly allows public downloading. Saved copies live in Music/CYN on Android 10+, play back offline in CYN and are never a disguised stream cache. Already-on-phone music is already offline; non-downloadable tracks are skipped.
- Adds per-liked-song download action and an **offline saved** indicator. Album/search and favorite handling remain intact.
- Adds **Clear likes** with explicit confirmation and a long-press **Remove from Liked** action; unliking a track never deletes music from your phone. Individual heart controls still work.
- Preserves all prior CYN Studio identity, FX/native playback engine, APEX 6M/10M, saved presets, model assets, settings and Android updater.
- Includes regression checks for session-safe LIBERTY DSP and persisted download/liked state.

**Version:** `0.2.31-preview`, versionCode **35**, `io.github.oddfoxerr.cyn.studio`. The signed preview must verify an in-place upgrade from the existing signed `0.2.30-preview` using the registered CYN Studio certificate.

**Limitations:** LIBERTY's six-control AUDONIC-based model needs actual CYN Library PCM analysis; other apps use the permitted Android session EQ/FFT fallback rather than full AUDONIC inference. Audius download permissions are artist-controlled and may change. Playback on particular streaming apps and phones still requires physical-device checks.

# CYN Studio Android v0.2.30-preview — External Audio Routing Recovery

- Repair Connect CYN session routing: match supported Android audio-effect sessions to *currently playing media apps* using audio-session broadcast package metadata and user-authorized media-session access, rather than disabling external effects whenever more than one session exists.
- Keep other apps, unmatched sessions and ambiguous multiple-session routes disabled by default. Players omitting package information retain the previous strict single-session compatibility fallback.
- Add an **Enable music-app detection** shortcut to Android Notification Access when necessary; CYN inspects active media-session playback state and package names, not notification message contents. The platform permission itself is broader.
- After wired, USB or Bluetooth audio-device changes, recover Android audio-effect objects through a debounced non-destructive restart without resetting saved CYN EQ controls or AI selection.
- Preserve CYN Library's original native PCM/DSP path; APEX 6M, APEX 10M, AUDONIC, YAMNet, BrainScan, learning, Material 3 Graphite, saved presets and updater remain unchanged.
- Preserve the existing **no screen recording / no MediaProjection** Connect CYN design; no wireless ADB privilege is requested.

**Important limitation:** Android's third-party audio-effects API is optional per music app and device; being able to control an EQ is not proof the player's audible output uses it. This preview does not enable unrestricted interception of other apps' sound or guarantee Spotify/DRM playback enhancement.

**Upgrade identity:** CYN Studio package `io.github.oddfoxerr.cyn.studio`, versionCode **34**, versionName **0.2.30-preview**. Release uses the existing registered CYN Studio signing certificate and installs over signed prior Studio previews (no uninstall required) when Android accepts the verified update.

**Verification:** CI must pass native DSP/model tests, external routing policy tests, full Android build and lint, Android emulator UI tests, signature/package/version checks and a verified upgrade comparison with v0.2.29 before signed binaries are published. Audible support across different phones, streaming apps and Bluetooth implementations remains subject to physical device testing.

# CYN Studio Android v0.2.29-preview — Graphite Material 3 and Private Connect

- Default Android Material 3 palette now uses CYN Desktop's graphite-gray surfaces. Other saved palettes remain available.
- The Updates area is now Settings, with saved local presets (load, rename, delete, undo), native model import/export, Theme Studio, motion preferences, updates, credits and Exit CYN.
- Reduced-motion-aware 350 ms exit animation, preserving saved DSP and audio state.
- Removed Android MediaProjection screen-sharing permission and the screen recording/sharing consent prompt. Retired capture intents are refused; compatible audio-session EQ and optional device visualizers remain.
- No screen recording, microphone recording, or phone wireless debugging has been added. Wireless debugging is deferred.
- Existing package identity, APK signing certificate, APEX/AUDONIC models, library, user data and updater remain unchanged. Version code 33 follows 0.2.28-preview.

# CYN Studio v0.2.11-preview — Live Visuals & Favorite Hearts

- Separate Sound Studio's live spectrum from slower YAMNet/APEX inference: a small, bounded 4096-sample FFT now analyzes the actual decoded 48 kHz PCM stream on a dedicated low-priority worker about 25 times per second.
- Animate the 31-band spectrum at display refresh rate with independent rise/fall easing; never display invented movement when audio is silent or missing.
- Increase external Android playback Visualizer's capture request from 4 Hz to up to 20 Hz, respecting the device's allowed maximum.
- Replace the clipped 48dp text-heart MaterialButton with a proper 48dp touch-target vector icon, filled/tinted when favorited and outlined when not. Saving failures roll back the favorite state.
- Add emulator tests for a live PCM sine wave, clearing on pause, and favorite toggling and persistence.
- Preserve all existing audio enhancement, AI models, slider behaviors, saved settings and Material 3 navigation.

# CYN Studio v0.2.10-preview — Reliable Bottom Navigation

- Keeps Material 3 bottom navigation items mounted instead of clearing and recreating the menu whenever the page changes.
- Completes each selected-tab click before switching page content; rapid consecutive taps cannot overwrite the final selection.
- Preserves the one-line Studio, AI, Library and Style labels, selected indicator, current theme colors and accessibility.
- Adds real navigation item-click regression coverage for rapid taps and repeated tab changes. Sound processing, AI selection, library, and updates are unchanged.

# CYN Studio v0.2.9-preview — Material Navigation Hotfix

- Fixes the user-reported phone screenshot where the MaterialButton bottom navigation split **Studio**, **Library**, and **Style** into two or three lines and oversized selected tab.
- Replaces the manually assembled MaterialButton tabs with Google's native **BottomNavigationView**, which independently lays out Material 3 active indicator, icon and single-line label for all four destinations.
- Keeps the same CYN tonal selection colors and icons; equal-width destinations, stable labels on ~360 dp-wide phones.
- Tightens the inactive Sound Studio spectrum area to expose the sound controls sooner; visualizer expands again when playback or live connection is active. No fake audio activity is drawn.
- Preserves audio DSP, baseline/AI fox sliders, library, all themes, update flow, permissions and accessibility.
- Adds a targeted 360 dp phone layout regression test and retains all prior screen, music and AI tests.

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
