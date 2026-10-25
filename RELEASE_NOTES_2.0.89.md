# CYN Windows 2.0.89 — Bring back the original updater experience

This Windows release restores real in-app **Check for Updates** to the Riff-derived CYN Material interface. It preserves the original FxSound/CYN DSP, signed Windows audio driver installation, saved CYN settings and AI models, and the classic verified update worker.

## Restored
- The title bar **Updates** and **Settings → Check for CYN Updates** open a first-class Material update-check window.
- Users see checking, **already up to date**, **update available** with installed and latest versions, network errors and Retry.
- Checking is read-only: no installer download or system changes unless the user requests them.
- **Download & install** starts the original CYN updater's animated progress window. It still performs the real GitHub download, SHA-256 verification, UAC installer launch, safe exit of both the native FxSound DSP host and Material frontend, and relaunch after installation.
- Native CYN version is read from the actual executable, even if audio processing is disconnected.
- PowerShell uses the STA apartment for the WPF updater window. Immediate updater exits are reported rather than claimed as success.
- Regression tests validate update states and the real updater process; Windows CI exercises both current and available release metadata.

## Upgrading
Install v2.0.89 **over your existing CYN**. No uninstall, model reset or new audio driver installation should be required if the original driver is healthy. If an installer detects other FxSound/CYN audio hosts, close them from the Windows tray as requested.

## Known limits
Some advanced Classic AI Hub, training and appearance tools remain outside the redesigned interface. Automated tests do not prove every physical Windows audio routing configuration; please check real output-device DSP after installing.

This release is for Windows PC, not Android. Riff © rootscripts (MIT). FxSound/CYN DSP © respective authors (AGPL-3.0-or-later).
