# CYN Windows 2.0.95 — Physical Output Selection and Real FxSound Routing

This is a targeted fix for **CYN v2.0.94** showing a headphone name beginning with an escaped quote (for example, `\\"Headphones`) and reporting that native CYN selected a different output.

## Corrected
- The Riff/Material main interface sends device names through one unquoted UTF-8 hex command argument rather than nested quoted `--output="..."` values; spaces, Unicode device names and Windows quote escaping are preserved exactly.
- The original native FxSound/CYN host decodes and validates the requested name against **active stereo physical Windows audio endpoints** before committing a device selection. Invalid/phantom selections are not saved.
- Native status now reports the **actually selected physical endpoint** from the same Windows device snapshot, while `configured_output` remains available separately for troubleshooting old saved settings.
- Output changes are not marked successful until the native controller confirms the real endpoint. When audio is powered on, the FxSound playback target must also match.
- The Riff status distinguishes **Power ON** from real **processing/routing** and gives a specific warning when Windows uses a different default playback device instead of the FxSound virtual endpoint.
- Reused existing signed FxSound driver and original DSP/AI signal path; no Riff DSP or simulated routing was added.

## Upgrade and test
Install over 2.0.94 without uninstalling; settings and presets are preserved. Close CYN, install the new version, launch Riff CYN, select a physically connected stereo device from Output Devices, and play music. Confirm the on-screen processing status, then toggle power/bass/EQ and listen for a real difference.

When processing is on, Windows Sound > Output should have **FxSound Audio Enhancer** as the default playback device, while CYN's Output Devices selector should point to your **actual headphones or speakers**. These are two different roles; do not select the FxSound virtual endpoint as the physical output.

**Important:** Windows CI proves compilation and integration, not hardware audio on your PC. If audio still bypasses FxSound, collect the actual CYN native status and the Windows endpoint details instead of assuming the UI proves playback.

Windows x64. Existing CYN/FxSound AGPL and Riff MIT notices apply.
