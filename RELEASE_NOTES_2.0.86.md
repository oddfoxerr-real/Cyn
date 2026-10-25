# CYN Windows 2.0.86 — Open the redesigned Material desktop by default

This update fixes the v2.0.85 launch routing regression. The previous installer correctly bundled the redesigned Riff-based Electron interface, but the updater restarted `Cyn.exe`, which opened the classic JUCE screen instead. The native audio controller could also reopen JUCE when it received EQ commands, while a version upgrade reset the minimized setting.

## Fixed
- The updater gracefully closes **both** CYN Audio's Material interface and the native DSP process, waits for them to exit, and relaunches `CYN-Interface\\CYN Audio.exe` after successful installation.
- Old taskbar pins and legacy startup shortcuts pointing at `Cyn.exe` now redirect normal interactive opens to the redesigned interface, including when the native engine is already running.
- Native status/preset/EQ/effect CLI commands keep running in the background and never open the classic window unexpectedly.
- The native system-tray **Open** action launches CYN Material, and repeated opens focus the existing Electron instance.
- The native DSP remains in the same installed folder and continues processing Windows system audio, APEX/AUDONIC, and the existing sound preferences.
- **CYN (Classic)** remains an explicit Start Menu shortcut for legacy training tools and advanced troubleshooting. Normal CYN launch never silently falls back to the legacy interface if the Material executable cannot open; it instead displays an installation-repair message.
- Keep the original Windows Inno Setup AppId, signing/update verification, launcher shortcuts and saved configuration. The stable v2.0.85 installer is not overwritten.

## Legacy shortcut compatibility
- Windows taskbar pins and pre-upgrade shortcuts using `Cyn.exe --view=1`, `--view=2` (and space-separated forms) now open exactly the same redesigned interface as the updated Start Menu shortcut.
- The native engine continues to accept `--run_minimized`, `--status`, preset, EQ and other audio IPC commands without opening an extra window.
- The old JUCE display is not a normal launch target. `--classic` remains an explicit diagnostic path for native features still being migrated.

## Design
The default interface remains the Riff-derived graphite Material 3 design from the separate CYN Audio Preview 0.2.0: animated connected-pill dock, side drawers for presets/AI Hub/appearance, new Settings and custom close animation. Visual attribution for Riff's MIT code remains in the included notices, without branding the application as "CYN Riff".

## Notes
This is a Windows PC update, not a new Android APK. No Windows audio driver reset or model uninstall is required. Some unported native views still live under **CYN (Classic)**.

Riff © rootscripts (MIT). FxSound/CYN native DSP © respective authors (AGPL-3.0-or-later).
