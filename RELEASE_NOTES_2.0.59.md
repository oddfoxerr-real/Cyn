# CYN v2.0.59 — Windows APEX x64 Runtime Compatibility

Fixes an installer prerequisite gap exposed by **APEX 6M error 126** on a second Windows PC. ONNX Runtime was already bundled, but its dependency on the Microsoft Visual C++ **x64** runtime was not guaranteed by the CYN installer.

## New installer options
- On PCs without the Visual C++ x64 runtime, the installer offers a checked **Install Microsoft Visual C++ x64 runtime** task. It can be deselected.
- On PCs where the runtime is registered but APEX shows error 126, choose the unchecked **Repair Microsoft Visual C++ x64 runtime** task.
- The official Microsoft redistributable is **downloaded and Authenticode-verified only during GitHub's build**, then bundled in the installer, allowing offline installation with explicit user choice.
- Installation/restart failures are surfaced to the user; the package does not silently install or alter dependencies without the selected setup task.

The original x86 vcruntime140.dll used in driver staging is not a substitute for ONNX Runtime's x64 prerequisites.

No changes to APEX ONNX weights, audio processing, presets, personal AI training, driver installation, themes, or close animations. Windows error 126 can also mean another DLL dependency is missing; installing VC++ x64 addresses the common cause, not all possible failures. After installing, exit CYN from the system tray and restart it. The previous v2.0.58 runtime diagnostic messages remain.
