# CYN Studio Android 0.2.8 validation plan — 2026-10-19

## Studio identity and signing

Studio uses application ID `io.github.oddfoxerr.cyn.studio`, launcher label
`CYN Studio`, version code 12 and version name `0.2.8-preview`. It installs alongside
legacy `io.github.oddfoxerr.cyn` because the original signing key was not recovered.
The old app and its data remain intact. Studio starts with independent storage.

The registered Studio certificate SHA-256 is
`045d9c122b2158d569c0ef8d10bbb08496a9f19e7397b702e99e9330c780bcb7`.
The private key is in encrypted GitHub Actions secrets and a restricted local
backup outside the source checkout. No private key or password is shipped.

Studio accepts only `android-studio-v` releases and `Cyn-Studio-Android-*.apk`
assets. Before installation it verifies the APK checksum, package, increasing
version code and equality with the installed signing certificate. Legacy Android
and Windows release channels are excluded.

## 0.2.8 control-animation checks

- In CYN Library, play audio with enabled Smart Listening: bass, clarity,
  stereo width, ambience, dynamics and EQ rings follow actual DSP target
  values, distinct from the user's baseline.
- In external playback, animate only proven supported bass/clarity EQ offsets;
  never represent local-only width, dynamics or EQ bands as applied.
- Without model inference, or when enhancement/Smart Listening is disabled,
  AI rings disappear instead of animating without measurements.
- User dragging a manual slider must never be overwritten by AI target updates.
- Verify reduced-motion disables smooth interpolation and that all EQ sizes work.

## 0.2.8 visualization checks

- Connect CYN without audio permission: user is offered a clear opt-in.
- External playback with consent: measured spectrum and neural visuals when
  Android makes playback measurements accessible.
- Stop CYN: capture resources close; stale spectrum and model trace disappear.
- Android blocks measurements: status accurately explains unavailable visuals.
- Check reduced motion, training view, and local player are not regressed.
- Verify on physical phones; emulator results cannot establish Spotify routing.

## 0.2.8 validation requirements

- Verify spectrum is derived from measured FFT bins and silence has zero visual activity.
- Verify local Connection details omit manufacturer, model, app names and session IDs.
- Check native AI processing status distinguishes external Android EQ and CYN library PCM.
- Check neural view goes inactive without recent activations and respects reduce-motion.
- Verify existing music-session monitoring remains functional and the user off-switch
  stays off until re-enabled.
- These tests cannot prove physical-device system-mix routing, headset quality or
  external Spotify compatibility. Verify on target devices before stable release.

## 0.2.8 connection regression gate

The Android 15 instrumentation suite adds an external-session test that starts
monitoring while enhancement is off and asserts the on/off state is preserved.
It also asserts waiting and unavailable-EQ statuses instead of claiming a
connection without effect control. The workflow runs these tests for pull
requests targeting `codex/android` before release.

Physical-phone verification is still necessary: actual routing through Spotify,
browser audio, Samsung EQ interoperability and the experimental session-0
compatibility path cannot be established by emulator tests. This update does not
claim unrestricted other-app interception or desktop-equivalent DSP for Spotify.

## Earlier checks (Studio 0.2.0)

- 288 pure-Java assertions passed, including learning, inference telemetry,
  numeric release ordering, legacy channel exclusion and malformed release data.
- Both Windows build/signing helpers passed PowerShell parser validation.
- Git diff whitespace checks passed.
- The new private key was opened and its public certificate verified before
  uploading it to the repository's encrypted signing secrets.

## Publication gates

The Studio release workflow runs all native DSP, Java learning, preprocessing,
real ONNX inference, Android build and lint checks. Android 15 instrumentation
covers native/model loading, disabled PCM bypass, 44.1-to-48 kHz library playback,
seek/pause, persistent favorites, rapid page changes and four phone screenshots.
A fifth test checks Studio's application identity, launcher name and private
update FileProvider. The sixth test checks the music-app monitor and enhancement
state. Publication requires all instrumentation tests to pass.

The signed APK must pass apksigner certificate verification, 16 KB zip alignment,
and an aapt package/version check. A SHA-256 file and corresponding-source archive
are published with it. The workflow refuses to overwrite an existing release.
The workflow result, rather than this pre-build document, establishes completion:
https://github.com/oddfoxerr-real/Cyn/actions/workflows/android-release.yml

## Previous implementation evidence

Before the Studio identity/signing change, workflow run 39256987103 checked
application commit `416031cc36319998c0005b543549d1b3e73f3048`. Build, lint,
285 host assertions and four Android 15 instrumentation tests passed. Its unsigned
APK and test reports were recovered and their ZIP hashes verified. That run did
not publish an APK because the legacy signing key was unavailable.

The Studio release reruns those gates with the new identity and updater tests;
previous results alone are not used to authorize publication.

## Physical-device checks still needed

Headphone/speaker listening, Bluetooth routes, battery use and inference latency
on real ARM phones; background update scheduling; TalkBack, large fonts and
landscape; and third-party playback-session compatibility. Emulator and numerical
checks do not establish identical audible output to Windows. Full neural
processing applies to CYN library playback; external players require compatible
Android audio-effects sessions. Disable the old app's monitor before enabling
Studio to avoid competing effects.

## 0.2.8 reversible thumb regression

- Start a local library track with active inference: manual ring stays at baseline while physical SeekBar thumb moves toward applied AI DSP values.
- Toggle Smart Listening or enhancement off; thumb returns smoothly to ring; ring does not move.
- Drag a slider while AI is active: manual value/ring update from touch, not AI; release resumes AI thumb follow.
- Expand Space & dynamics to inspect stereo width, ambience, dynamics, and manual-only headroom; expand EQ to inspect every band for all 31, 35, and 39 configurations.
- No fabricated controls for external audio; only bass/clarity on Android EQ-controlled sessions; no headroom AI target. Reduced motion must disable interpolation.

## 0.2.8 splash and auto-update checks

- Under Style → Updates, verify exactly one update action (UPDATE CYN) and one automatic-update switch; prior two buttons are absent.
- Opening Update CYN shows two-tone mechanical rails/gears and a dripping progress fill; reduced motion disables moving effects.
- Check stage is not assigned a fictitious numeric progress; downloaded bytes drive the 0–100% fill, and signature verification gets its own state.
- Existing matching, newer, verified APK shows INSTALL VERIFIED APK; no APK should be installed without Android user permission/confirmation.
- Background JobScheduler checks continue with automatic mode enabled, download only on unmetered networks, and notify when ready; disabled mode cancels the job.
- Dismissing splash during active transfer does not stop the transfer. Reopening shows live stage.
- Wrong signatures, hashes or package names are rejected. The final release workflow must verify version code 12, 0.2.8-preview.

## 0.2.8 fox and license checks

- Verify two-tone fox appears on AI-following Bass and EQ thumbs, in expanded Space & Dynamics and across all 31/35/39 EQ bands.
- Move or drag a slider to trigger the fox's brief tail/bounce animation; stationary sliders should not force continuous repaint, and Reduce motion should eliminate movement.
- Fox artwork must never change user baseline rings, AI sound targets, audio values, or training.
- Verify the credits panel names FxSound LLC and its contributors as upstream authors while CYN modifications are separate and unofficial, retaining the complete AGPL license and third-party terms.
- The final APK must have version code 12, versionName 0.2.8-preview and the established Studio signing certificate.

## Material 3 redesign (0.2.8)

- Screens use the Material 3 base theme and MaterialButton, MaterialCardView, MaterialSwitch, and MaterialAlertDialogBuilder rather than Android Material 1 widgets.
- Navigation shows four labels and vector icons on the bottom rail, with a visible tonal selected state. Changing pages must preserve audio playback and respect reduced motion.
- Cards, switches, buttons, accessible labels and dynamic text all fit narrow mobile screens; verify all four screenshot pages.
- CYN's 4 theme palettes stay independent of wallpaper. Verify no DynamicColors calls, wallpaper permission or wallpaper listeners are added.
- Fox/AI thumb motion, baseline rings, 31/35/39 EQ bands, connection control, mobile updater and license dialog remain functional.
- The signed release has versionCode 12, versionName 0.2.8-preview, and the exact established Studio signing certificate.
