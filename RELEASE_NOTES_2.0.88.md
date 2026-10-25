# CYN Windows 2.0.88 — Visible Settings, real FxSound native engine diagnostics

This repair update improves the CYN Material desktop without replacing the original FxSound/CYN native DSP. It uses the existing signed-driver setup, Inno Setup application identity and normal update path.

## Changes
- **Settings and Updates** are visible, labeled buttons in the top title bar, even if the audio engine cannot connect.
- **FxSound/CYN Native Engine** panel appears above the Sound Studio EQ, reporting native DSP readiness, active processing and actual Windows output when connected.
- **Connect engine** retries a real native `Cyn.exe` launch; **Repair install** opens the official CYN release page.
- The JUCE audio host stores actionable, bounded local diagnostics when the signed FxSound virtual audio driver is missing, unavailable or another FxSound/CYN process holds the exclusive audio host.
- Native status IPC can handle large 31/35/39-band DSP, preset and neural-state responses without hitting a 4KB output limit.
- The Windows release pipeline checks the staged real `Cyn.exe`, installed native Factsoft presets, verified updater files, runtime ONNX assets and bundled Material interface before publishing.
- The Electron UI screenshot tests click the actual Settings button and verify critical controls are onscreen.

## Installation and recovery
Install on top of the existing CYN version. Do not uninstall first: this keeps your CYN installation, user presets, model state and original Windows driver path. Quit both CYN and FxSound from the system tray before installing if requested. Use **Updates** or **Settings → Check for CYN Updates** within Material CYN. If the native engine fails, use **Repair install** to open the official installer.

## Scope and limitations
These changes preserve the native FxSound audio engine and improve its diagnostics and UI connection, but they cannot certify that every Windows audio device/driver combination processes sound until tested on the affected PC. Some Classic AI Hub and advanced tools still require the separate **CYN (Classic)** Start Menu shortcut. This is a Windows desktop update, not an Android APK release.

Riff © rootscripts (MIT). FxSound/CYN native DSP © respective authors (AGPL-3.0-or-later).
