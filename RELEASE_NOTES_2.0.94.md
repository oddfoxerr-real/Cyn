# CYN Windows 2.0.94 — Riff CYN Is the Main App, FxSound Is the Engine

This release replaces the old stacked Windows startup with **Riff CYN (main desktop UI) → original FxSound native audio host**. It is not a second audio-processing application over CYN over FxSound.

## Architecture fixed
- Windows shortcuts, the normal launcher, startup registry target, and post-update restart now open **CYN Audio** (Riff) directly.
- Riff starts and reconnects to one bundled **Cyn.exe --engine-host** process. That process owns the original FxSound driver, capture/routing, DSP, CYN audio settings and AI inference.
- Normal native startup does not construct the legacy JUCE/CYN main window. The redundant "Original controls" button and Classic desktop shortcut are removed.
- Native CYN's window-dependent callbacks now tolerate a headless process, including FxSound power/EQ changes, audio device callbacks, tray actions and shutdown.
- Engine shutdown still saves presets, stops timers and releases the original audio passthrough driver, even when there is no JUCE window.

## Preserved
- FxSound's original audio DSP, Windows virtual audio device and physical output selection; no separate Riff audio engine.
- Existing CYN AI/ONNX workers (including APEX and supported models), Smart Listening, native presets and 31/35/39-band equalizer.
- Riff/Material UI, live native status and neural telemetry, Settings, updater and signed driver installer.
- App data and preferences from 2.0.93; install directly over the existing Windows CYN installation.

## Verify on your PC
After upgrading, start CYN from its normal shortcut. One Riff window should open, with no second JUCE/Classic GUI. In Sound Studio, select the correct output device, confirm the actual native processing status and play audio. Check that switching power or bass/EQ audibly changes the output, then restart and test updates and presets.

**Limitations:** Windows CI can prove source integration, native compilation, package integrity and update metadata, but cannot verify your specific Windows audio routing, installed FxSound driver or speakers. If the native host reports the virtual device is missing, use Repair install; a UI-only update cannot replace the signed driver.

Windows x64. Riff © rootscripts (MIT). FxSound original engine © FxSound LLC and contributors (AGPL-3.0-or-later). Existing source notices apply.
