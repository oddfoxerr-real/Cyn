# CYN Windows v2.0.80 — Expressive desktop motion

CYN 2.0.80 brings the Riff-inspired Material 3 expressive design update to the native Windows application, building on the v2.0.79 installer reliability fixes.

## Desktop UI

- Native animated sidebar selection glides and morphs between CYN workspaces, with tonal hover feedback.
- Sound Studio Overview, Equalizer and Enhancements now share an interruptible morphing selection indicator.
- More tactile buttons use Ripple Press and Glass Sweep by default on new installations; existing saved CYN Studio preferences are preserved.
- Linear controls use detached, Material 3-style slider thumbs and track gaps, with hover feedback.
- Animations retarget smoothly when clicking quickly; reduced motion remains supported.
- Retains CYN branding, native JUCE controls and existing liquid content transitions. No Riff source code/assets or Electron dependencies are bundled.

## Compatibility

- All CYN DSP, EQ (31 / 35 / 39 bands), audio driver, APEX, AUDONIC, AI DJ, neural BrainScan and user learning/profile data remain unchanged.
- Windows x64 installer upgrades v2.0.79 in place; no uninstall is needed. Exit both CYN and FxSound from their tray icons before installation when prompted.
- App, Windows executable, JUCE metadata, installer and update identifier are consistently v2.0.80.
- The release is intended to be discoverable through CYN's built-in GitHub update check.

## Verification

Windows release automation compiles native JUCE and AI code, tests version/update handoff, audio/visual regressions, validates bundled ONNX model and VC++ installer prerequisites, verifies the generated installer SHA-256 and publishes the assets only on success.

Source PR: https://github.com/oddfoxerr-real/Cyn/pull/78

**Note:** The Windows installer is not code-signed; review Windows security prompts before proceeding.
