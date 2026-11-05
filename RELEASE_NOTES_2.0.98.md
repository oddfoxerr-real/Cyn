# CYN Windows 2.0.98 — Unified desktop app

CYN 2.0.98 makes the current Riff/Material desktop UI the only user-facing Windows window. The original FxSound driver, DSP processing and AI workers continue to run in an automatically managed background audio host.

## One desktop experience
- Replaces the old processing checkbox with one accessible **CYN power ON/OFF** button. The button switches the original native DSP state; it does not kill the audio service or reset presets.
- Removes **Start engine**, **Connect/Reconnect engine**, **Locate Cyn.exe**, duplicate native interface management and unnecessary frontend IPC routes.
- Retires construction of the legacy JUCE CYN main window. Old Classic shortcuts redirect to the new interface.
- Reconnects a temporarily unavailable native host automatically with rate-limited recovery. Packaged builds use the bundled native audio executable only.
- Keeps genuine audio-routing diagnostics and an installer-repair link when needed, without opening separate warning windows during headless recovery.

## Preserved
- The signed FxSound virtual audio driver and original audio-processing chain.
- CYN native DSP effects, Windows output selection and device pinning, 31/35/39-band EQ, presets and saved settings.
- Smart Listening, APEX, AUDONIC, AI DJ, BrainScan activity, real telemetry, training and updater integration.

## Installation
Download **Cyn-Setup-2.0.98-x64.exe** from this release and close the existing CYN and FxSound tray processes before updating. The installer upgrades the existing installation and preserves user settings.

The unified UI and native audio host remain distinct operating-system processes to preserve native Windows audio compatibility, but there is only one visible CYN desktop application.

Windows 10/11, x64. Unsigned community application; the included Windows audio driver retains its existing Microsoft signature. Native physical-device playback must be tested on the actual Windows hardware in addition to CI checks.
