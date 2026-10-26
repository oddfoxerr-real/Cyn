# CYN Windows v2.0.91 — Stable JBL/headphone selection and native DSP controls

CYN Windows v2.0.91 repairs an issue where Material CYN repeatedly lost the selected Bluetooth/USB audio device and incorrectly reset controls while the native FxSound/CYN engine was reconnecting or reporting transiently incomplete Windows device scans.

## Fixes
- Preserve the **last verified output-device name** through transient empty native audio-device enumerations. Clearly indicate when the device has not yet been reverified; keep stale/offline entries disabled rather than falsely routing to them or selecting another device.
- Use one coherent native Windows device enumeration per `Cyn.exe --status` response for both real FxSound route health and the output-device list.
- Serialize UI status refreshes. A status request started before a user turns processing on/off or selects JBL headphones cannot later overwrite that action.
- Verify real native `--power` and `--output` command outcomes and report if the requested setting did not apply.
- Wait for pre-command status requests to finish before reading command results in the Electron/native bridge. Retry native JSON status-file reads when they overlap a file write.
- Keep the last confirmed settings visible—but **unverified and non-editable**—during brief native IPC failures, rather than erasing output labels, EQ controls and power position.
- Maintain the original FxSound signed driver, `AudioPassthru`, `DfxDsp`, factory and user presets, APEX/AUDONIC AI and the Riff-derived Material UI.

## Install and confirm
Install this Windows update over the existing CYN. Do not uninstall first. In Sound Studio choose your real headphones or speakers, then enable processing. If Windows device enumeration drops out, the last selected device is shown as temporarily unavailable instead of silently disappearing. If Windows audio isn't routed through CYN, use **Restore system audio** with the correct physical output selected. Sound will be processed through the original FxSound virtual device only when Windows playback routing is genuinely active.

This release does not claim that a Windows PC or Bluetooth headset has been physically tested by CI. If the device still disconnects, inspect the reported native engine connection and Windows Bluetooth/driver state.

Riff © rootscripts (MIT); native FxSound/CYN audio DSP © respective authors (AGPL-3.0-or-later).
