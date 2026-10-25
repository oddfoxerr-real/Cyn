# CYN Windows 2.0.90 — Original FxSound audio routing + Material interface

This release repairs Windows audio bypassing CYN even when the new desktop UI could report a connected engine.

## The real audio engine remains CYN/FxSound
CYN.exe continues to host the original FxSound signed virtual playback driver, AudioPassthru capture/playback, native DfxDsp effects, equalizer, and AI DSP. The Material/Electron program is only a user interface and controller; it does not process or reroute audio itself.

## What changed
- Sound Studio now separately shows the real virtual driver state, selected physical speaker/headphone availability, whether **Windows is actually routed through CYN**, and whether native audio samples are being processed. A connected process alone no longer gets a green processing indicator.
- **Restore system audio** (explicit user action) asks the original native host to enable FxSound default-device routing, restart the existing capture/playback thread, and reconnect the selected physical output. No duplicate audio engine or Web Audio pipeline.
- Handles an unavailable-device sentinel that could otherwise prevent native audio from retrying after hardware or driver availability changes.
- **Original controls** in Sound Studio opens CYN's native FxSound control UI for audio device and driver diagnostics.
- Corrects the installed **Cyn (Classic)** Start Menu shortcut to use `--classic`; the previous `--view=2` argument reopened Material instead.
- Keeps the original native presets, YAMNet/APEX/AUDONIC support, normal update channel, driver installation protections and user settings.

## Recovery steps
Install v2.0.90 over CYN (do not uninstall first). Open Sound Studio and check **Windows via CYN**. If **No**, select your headphones/speakers as the CYN physical output and click **Restore system audio**. This intentionally changes the Windows default playback endpoint to the FxSound virtual driver while sending the processed sound to your chosen physical device. If the driver is missing or disabled, use **Repair install** or **Original controls** for diagnosis. Apps individually pinned to a specific physical device in Windows Volume Mixer can bypass a system-wide virtual default: set those apps' output back to **Default** to include them.

## Limits
Automated Windows checks validate compilation, original DSP code path, updater, and release packaging. They cannot certify sound on every real Windows PC, USB headset, HDMI output or Bluetooth device. Some previous Classic UI features still require **Original controls**. This is Windows only (Android unchanged).

Riff UI: © rootscripts, MIT. Original FxSound/DSP code © respective FxSound authors, AGPL-3.0-or-later. CYN contributors retain their own contributions.
