# Cyn 2.0.25 — Optional Audio Auto Repair

Cyn 2.0.25 introduces optional Windows audio-device diagnostics, output recovery, and permission-controlled driver downloads. Music and Movie AI, AI DJ, Theme Studio, and Cyn's existing enhancement engine remain available.

## Audio Repair in Settings → Misc

- **OFF by default.** No automatic repair monitoring or driver downloads until enabled.
- **Diagnose now** checks Windows Plug and Play device errors and identifies previously selected headphones that may have disconnected.
- **Try reconnecting** performs a user-approved device refresh and may change the active Windows playback device; it does not install or remove drivers.
- **Restore missing output** is an additional optional toggle, OFF by default, that attempts safe audio-route recovery when Windows detects an issue.
- For Windows-reported physical audio-driver errors, Cyn can check for a matching driver package through **Microsoft Windows Update** only, verifying audio hardware identifiers.

## Download permissions

- **Ask every time (default)** prompts before each driver package download.
- **Always allow verified downloads** may be explicitly approved in Settings. Only future Microsoft Windows Update downloads for matching audio devices are preapproved.
- **Never download drivers** disables driver downloads while allowing diagnostics.
- **Reset permissions to Ask** revokes standing download permission.
- Driver *installation, removal, rollback, and system restart* always remain separate from these controls. Cyn never performs those actions automatically.
- Turning Auto Repair off stops its monitor and worker; Windows Update may retain already downloaded files in its own cache.

## Limitations and testing

- Native Windows builds, PowerShell syntax validation, updater diagnostics, and automated permission checks passed before publication. Recovery behavior has not yet been validated across real Bluetooth hardware configurations.
- A headset can be connected but silent even when Windows reports no driver fault. Cyn cannot guarantee a repair when Windows lacks a matching package or the problem is physical, radio-related, or elsewhere in the OS.
- Users choose installation separately in **Windows Settings → Windows Update → Optional updates**.
- Cyn's built-in FxSound virtual audio driver and music enhancement functions are unchanged by this feature.

## Downloads

- **Cyn-Setup-2.0.25-x64.exe** — Windows x64 installer.
- **Cyn-Windows-x64.zip** — portable Windows build.
- Installer **.sha256** checksum included.
- Builds are not code-signed and may trigger a Windows security warning.

Licensed under GNU AGPL-3.0-or-later.
