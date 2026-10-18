# CYN v2.0.58 — APEX 6M Compatibility & Startup Repair

Addresses reports that CYN APEX 6M can activate on one Windows PC but displays **"Full APEX model is missing or still loading"** on another, even when model files appear installed.

## Fixes
- **Honest startup diagnostics:** Distinguishes missing/incomplete ONNX model, missing onnxruntime.dll, Windows dependency loading errors (126 / 193 / 1114), ONNX API version mismatch, failed session creation and failed inference. Displays actionable details in AI Hub instead of a vague loading message.
- **Patient activation:** AI Hub can queue APEX activation while the model initializes and automatically activate it if the neural engine becomes ready (up to 60 seconds). On failure, it reports the reason and retains the user's previous saved AI choice.
- **Retry without restarting:** Retry the APEX engine from AI Hub if it failed on this machine. A saved APEX selection is no longer silently treated as successful if its runtime stopped working.
- **Included diagnostics:** `DIAGNOSE_CYN_APEX.ps1` ships alongside Cyn.exe and checks APEX checkpoint size/SHA-256, ONNX DLL load, and Microsoft Visual C++ x64 runtime registration. It does not install anything, touch drivers, download files, or read private AI model settings.
- **Installer checks:** Native Windows CI compiles CYN, runs the real full APEX ONNX smoke test, and validates and exercises the diagnostics script against staged files.

## If APEX still fails on one PC
Exit CYN through its system tray icon (X only hides it), then run `DIAGNOSE_CYN_APEX.ps1` in the Cyn install folder using PowerShell. The script saves a report under the user's Windows TEMP directory. If DLL load error 126 indicates missing Visual C++ runtime dependencies, the official Microsoft x64 redistributable is at https://aka.ms/vs/17/release/vc_redist.x64.exe. Install it only with the device owner's approval.

**No changes** to model weights, private training, Music AI personal profiles, DSP algorithm, audio routing, CYN themes, or close animation. As the affected device has not been inspected directly, a PC-specific loader or hardware issue may still require manual remediation.
