# CYN Windows 2.0.93 — Bundled Original FxSound Engine Starts Without Classic

This maintenance release builds directly on the original CYN/FxSound engine restoration and **native-first CYN launcher** in v2.0.92. It fixes a remaining problem where the redesigned Material interface could connect to an outdated manually selected `Cyn.exe` or show disconnected controls until Classic was opened.

## Fixed
- **Always use the installed native engine.** The combined Material installer now prioritizes the adjacent `Cyn.exe` from its own installation over any older preview/Classic executable path or environment override.
- **CYN starts independently of Classic.** The Material interface can start, reconnect to and control the original FxSound/CYN DSP process without the Classic window. A short retry cooldown avoids multiple competing audio hosts.
- **Reliable process launch.** Concurrent launch attempts are coalesced, the correct native working directory is set, and Windows process spawn failures are reported instead of treated as successful connections.
- **Same engine for every feature.** The original native CYN handles audio capture, signed virtual playback driver, FxSound effects, 31/35/39-band EQ, output selection, AI models and presets, power and updater. The new Riff/Material interface is only a controller.
- Automated regression tests reject stale executable selections and assert that Material can bootstrap the real native host.

## Upgrade and test
Install over v2.0.92 without uninstalling or deleting settings. Launch CYN from its normal Start Menu or desktop shortcut; **do not open Classic first**. In Sound Studio, check the engine's actual reported power and selected playback device. Play music and toggle processing to hear the effect. If the FxSound virtual driver is missing/disabled, use the normal repair installer; app UI cannot substitute for the original signed Windows driver.

**Known limitation:** Windows CI validates source, native compilation and packaging, but cannot guarantee audio routing on your individual PC until physical playback is tested. Closing Material does not intentionally stop the original DSP host.

Windows x64 only. Riff UI © rootscripts (MIT). FxSound/CYN native DSP © FxSound contributors (AGPL-3.0-or-later).