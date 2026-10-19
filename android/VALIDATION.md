# CYN Studio Android 0.2.0 validation — 2026-10-19

## Studio identity and signing

Studio uses application ID `io.github.oddfoxerr.cyn.studio`, launcher label
`CYN Studio`, version code 4 and version name `0.2.0-preview`. It installs alongside
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

## Checks completed locally

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
update FileProvider. Publication requires all five tests to pass.

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
