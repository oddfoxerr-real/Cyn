# Cyn Audio Repair (opt-in Windows utility)

This feature is in Settings -> Misc -> Auto Repair. It is intentionally separate from music/movie AI and the DSP engine.

## Consent

- Auto Repair is OFF by default. When off, no monitoring, repair check, or driver download is started.
- Allow minor recovery is a separate, off-by-default switch. It allows Cyn to recheck devices and reselect a playback output when a saved headset goes missing or Windows reports an audio fault. This can change which output Windows uses, but does not uninstall or install drivers.
- Ask every time (default): before each Windows Update driver package download, a Cyn prompt identifies the update and source, and offers Allow or Decline.
- Always allow verified downloads: explicitly selecting this setting permits future matching Windows Update driver downloads without showing the per-download prompt. It does not permit driver installation or restart.
- Never download drivers: no automatic downloads occur. Users can diagnose or open Windows Optional Updates manually.
- The Try reconnecting button explicitly asks before rechecking and potentially switching Windows playback routing, even if automatic minor recovery is switched off.
- Disabling Auto Repair stops the monitor and local worker. Changing permission to Never during a download stops Cyn's local worker; Windows Update may retain previously downloaded content in its own cache. It does not roll back anything.
- Permissions can be reset to Ask at any time. The choice is stored locally using Cyn's existing application settings, not sent to a server.

## Detection and download workflow

1. Windows Plug and Play (SetupAPI/Configuration Manager) is queried for reported errors among media and audio endpoint devices. No PnP error is not proof that a paired headset works correctly; users can run manual diagnostics.
2. With minor recovery enabled, Cyn calls its existing device-detection and routing logic. This may change the active Windows playback output. It does not change driver packages, Bluetooth pairing state or OS services.
3. For an identified faulty physical MEDIA driver, Cyn may query Windows Update for an audio-hardware-ID-matching driver. Logical AudioEndpoint errors are not automatically associated with a different driver.
4. Windows Update Agent performs the scan. Driver candidates must match a currently present MEDIA PnP hardware identifier, not just a display name. Manual scanning can consider all present MEDIA devices.
5. After download permission, Cyn launches a separate Windows PowerShell worker. The worker rechecks the hardware ID, Windows Update identity and revision before downloading via the Windows Update Agent COM API. No arbitrary download URL or external installer is used.
6. The driver is downloaded, not installed. A separate user decision through Windows Settings -> Windows Update -> Optional updates is required to install. Windows may require administrator permission or acceptance of additional license terms.
7. Results and attempted recovery steps appear in the in-app status and local repair activity setting. Windows Update network/driver errors are reported; not every headset or error has a matching driver update.

## Known boundaries

- Cyn can't guarantee a third-party Bluetooth pairing problem is actually caused by its driver, or repair missing hardware, bad cables, radio interference, defective dongles or OS issues.
- Detection depends on Windows PnP-reported status. A connected-but-silent device may need manual troubleshooting rather than a driver update.
- Audio-only detection does not touch Bluetooth stack drivers or unrelated system hardware.
- The Windows Update Agent may not expose an eligible driver update even when one exists on a manufacturer support site. This feature never downloads from third-party sites automatically.
- The separate PowerShell worker uses only Windows built-in commands and Windows Update COM, without admin elevation and without driver installation.
- Test on Windows 10/11 with multiple playback endpoints, a disconnected headset, a genuinely faulting MEDIA device, Ask/Always/Never, and restart/disable transitions. Native CI verifies source safety and compilation, not all possible hardware failures.

## Test

Run node scripts/test_audio_repair.mjs, then BUILD_CYN.ps1 on Windows with the required build tools. The GitHub pull request workflow also parses the PowerShell worker and compiles the Windows x64 app.
