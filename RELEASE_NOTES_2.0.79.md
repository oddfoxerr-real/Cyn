# CYN Windows v2.0.79 — Installer recovery and diagnostics

This patch addresses **installer error 5** reported after downloading and SHA-256-verifying the Windows v2.0.78 update.

## Changes

- The in-app update handoff now invokes Inno Setup with `/SILENT`, not `/VERYSILENT /SUPPRESSMSGBOXES`. The setup progress window and error/Retry/Cancel choices remain visible when action is required. Inno Setup's default suppressed response to Abort/Retry is **Abort**, which could terminate an otherwise recoverable update.
- Updater launches the already-verified downloaded installer with `/LOG` and retains the log in `%LOCALAPPDATA%\Cyn\Updates`. A failed attempt offers a **View install log** action instead of hiding the underlying failure.
- Exit code 5 is described accurately as cancelled/aborted setup, not automatically an access-denied error. The stale progress display is cleared.
- The updater checks for an open FxSound.exe before elevation and prompts users to close it from the notification area rather than forcibly terminating it.
- All Windows executable, JUCE and installer version metadata increase to **v2.0.79**; application ID, existing FxSound driver, settings, presets, saved AI training, native AUDONIC, BrainScan, Material 3 theme and Android releases are unchanged.

## Recovery from v2.0.78's old updater

Because a failed v2.0.78 update leaves the old updater installed, download the **v2.0.79 x64 setup** from this release and run it directly once after exiting CYN and FxSound from their tray icons. Do **not** uninstall CYN or delete your saved application data to upgrade.

If setup reports another error, retain the Inno Setup log and report its final error lines. The actual file lock, driver, permission or other local cause cannot be identified from an exit code alone.

## Validation

GitHub Actions verifies Windows native compilation, APEX ONNX smoke test, update-hand-off contract, PowerShell syntax, installer prerequisite/signature checks, generated installer, portable archive and SHA-256 before publishing. The user's original machine-specific installation error cannot be reproduced within CI.
