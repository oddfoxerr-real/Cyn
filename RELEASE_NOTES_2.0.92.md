# CYN Windows v2.0.92 — Original Audio Engine Without Classic Dependency

This CYN Windows repair fixes a Material/Riff interface regression that could require opening **Cyn (Classic)** before audio processing or native settings became accessible.

## What's fixed
- The Material app now **prioritizes the native `Cyn.exe` bundled beside its interface**, ignoring stale executable paths saved in previous previews or another CYN installation.
- On startup it launches the **original FxSound/CYN native DSP host** minimized, and reconnects automatically if that host exits. Attempts are serialized and rate-limited to avoid multiple audio hosts or runaway process spawning.
- Material's update check, device settings, presets, AI models, EQ, effects and DSP actions all communicate with that *same* authoritative native engine. Buttons no longer require Classic to bootstrap it.
- The host is started with its correct installation working directory; launch failure and disconnected state are reported rather than presenting successful connection.
- Classic remains a separate optional advanced-control view, not required for system audio.
- A regression test verifies bundled engine preference over outdated manual paths, native startup lifecycle, and preservation of the original processing pipeline.

## Existing native engine retained
This is not a separate sound-processing implementation. The unchanged original FxSound audio engine, signed virtual audio driver, native AudioPassthru capture and DfxDsp effects, EQ presets, APEX/AUDONIC models, Windows output selections and updater remain part of the combined CYN installer.

## Installing and verifying
Install over v2.0.91 without uninstalling. Start **CYN** (the redesigned interface) directly, without launching Classic first. On **Sound Studio**, confirm the engine connects and Windows audio is routed via CYN, then toggle processing off/on while playing audio to verify an audible difference. If Windows routes to a physical output directly, select the physical output in CYN and use **Restore system audio** to route the Windows default through the original FxSound virtual device, only if you want system-wide enhancement. App-specific audio output overrides may still bypass the virtual endpoint. If the signed virtual audio device is missing, choose Repair install.

## Limits
A successful CI compilation and packaging check cannot guarantee the routing of every physical Windows device. This version corrects the known source-level engine selection/startup defects; final validation of real audio still requires testing on the affected Windows 11 PC. Android releases are unchanged.

Riff UI © rootscripts (MIT). Original FxSound DSP © FxSound authors (AGPL-3.0-or-later).