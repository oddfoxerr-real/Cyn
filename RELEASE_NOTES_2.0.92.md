# CYN Windows v2.0.92 — UI-only redesign compatibility fix

CYN's Riff-inspired dark layout is a **new interface for the original CYN application**, not a second audio engine or replacement for the existing FxSound processing pipeline.

## Original CYN engine restored
- Revert the custom audio-thread reset and routing additions introduced by v2.0.90. The original FxSound `AudioPassthru` source (public/private API and implementation) matches its pre-redesign v2.0.89 source, which in turn matches the older v2.0.85 engine.
- Remove the `--restore_audio` command and its Material **Restore system audio** button. The new UI must not silently force Windows audio device selection or restart the original capture/playback thread.
- Preserve original CYN's virtual driver, device selection, native audio capture, equalizer, effects, APEX/AUDONIC AI model handling, presets, and power-state controls. Audio remains processed by native `Cyn.exe`, not Electron or a Web Audio simulation.
- Retain the existing Material/Riff-style interface and **Original controls** button for the full native CYN device/driver configuration and advanced functionality.

## Fix confusing output display
- The Material device selector now shows the **actual output reported by the original native CYN host** when present. It does not mislabel an older cached JBL device as selected while native CYN reports Speakers.
- After selecting a device, the dropdown reconciles with the next native response instead of remaining focused on an unconfirmed requested device.
- Status indicators report native engine power and processing, not a blanket claim that Windows audio is routed correctly solely because the app is connected.
- If a Bluetooth/USB device or the signed virtual driver cannot be confirmed, use **Original controls** to diagnose the Windows audio output. Don't change Windows' default endpoint on the user's behalf.

## Upgrade and limitations
Install this release **over the existing CYN installation** to retain presets and settings. The native FxSound driver and audio path are not replaced with a new implementation. This update changes the UI integration and removes recent routing overrides; it does not guarantee audio will work with every hardware driver until it is tested on that PC.

Riff UI © rootscripts (MIT); FxSound/CYN native DSP © respective contributors (AGPL-3.0-or-later). Windows desktop only.
