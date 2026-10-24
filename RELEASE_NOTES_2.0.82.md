# CYN Windows v2.0.82 — Riff UI refinement and EQ validation

This is the follow-up Windows installer to the Riff-inspired native redesign in v2.0.81.

## What's updated
- Integrate CYN Desktop UI refinement from PR #80, retaining the Riff-derived Material surfaces, compact navigation and native JUCE controls.
- Route the native equalizer slider's existing gain write through an isolated, testable callback, preserving the original FxController/DSP gain-setting path.
- Validate real callback behavior for 31-, 35-, and 39-band EQ modes, including repeat values and reset-to-zero.
- Add fifteen source-linked offscreen SVG layout previews across five desktop workspaces and three band counts.
- Keep the Windows native MSVC/JUCE build and expressive-motion regression checks.

## Compatibility
- No change to FxSound audio driver, DSP processing modes, APEX/AUDONIC models, profile data or personal learning settings is intended.
- Install over v2.0.81 using CYN's existing updater or GitHub installer. No uninstall is required.
- CYN executable, JUCE metadata, installer and updater release identifier are aligned at v2.0.82.
- Preserve Riff MIT attribution alongside existing FxSound and JUCE licensing notices.

## Validation
- PR #80: CI Windows native compilation, motion/layout checks, neural regression and isolated EQ callback tests passed.
- Windows release CI builds the real installer, verifies version consistency, SHA-256 and model/runtime payloads before publishing assets.
- Geometry previews are not screenshots of the running native app. Manual visual inspection and live audio checks on actual Windows hardware remain separate.

Source: https://github.com/oddfoxerr-real/Cyn/pull/80
