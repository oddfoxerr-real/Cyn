# Neural telemetry and updater regression checks — 2026-10-11

Windows candidate: 2.0.17. Android candidate: 0.1.1-preview, versionCode 2.

## Automated checks

- Windows x64 Release build succeeds with zero errors. Inno Setup 6.7.3 compiles
  `Cyn-Setup-2.0.17-x64.exe`; the payload and setup report version 2.0.17.0.
- Android signed release and lint succeed: zero errors, five existing localization
  warnings. APK signature verified against the original 0.1.0 certificate.
- C++ model tests reconstruct weighted connections and hidden activations, compare
  inference against display outputs, and validate all 21 training snapshots against
  the candidate weights at their actual epoch. Existing source-analysis, stereo,
  invalid-input and reversed-feedback checks remain passing.
- Android host tests: 285 assertions including inference/trace equivalence, weighted
  contributions, real training epochs, Windows-tag exclusion, malformed/older/same
  Android version rejection and numeric version ordering.
- PowerShell updater tests under Windows PowerShell 5.1: script parsing, old portable
  copy versus registered installation, missing registrations, installed/running version
  floors, post-install version mismatch, portable fallback and invalid target rejection.
  These tests mock binary versions; no processes are stopped and no installation occurs.
- WPF update window loads with all existing progress/animation controls.
- Native AI Hub/layout integration checks pass. Licensing asset tests: 5 passing.

## Limits

No live Windows audio session was stopped or replaced during development. No
end-to-end privileged upgrade/downgrade or stuck-process test was performed on the
user's running installation. The installer and application must be tested together
on a disposable Windows machine before broad deployment.

No Android device is attached. Galaxy A56 / Spotify processing, legacy output-mix
support, permission flows, APK installation over the old version and rendering
remain unverified on hardware. Android effect ownership confirms the API state,
not an audible difference in an external player's output. Built-in file playback
and compatible-session processing remain available.

Android updates require the same package ID and signing certificate and an increased
versionCode. Keep the installed app and choose Update, rather than uninstalling it.
Update checks run only on request, query GitHub release metadata, and download via
the browser; Android retains responsibility for installation consent and signatures.

Public Windows binaries remain held by `scripts/check_license_release.py` pending
the distribution evidence recorded in `licenses/release-review.json`. The local
Windows build and Android-only package do not resolve those questions.
