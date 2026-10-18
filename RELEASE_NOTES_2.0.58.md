# CYN v2.0.58 — APEX 6M Windows Activation Recovery

Focused repair for APEX 6M activation failing on some Windows PCs.

## What changed
- **Real diagnostics:** the AI Hub now distinguishes a missing or incomplete full ONNX model from an unavailable ONNX Runtime DLL, missing Windows native prerequisites, incompatible ONNX APIs, and model initialization/inference errors. Windows DLL loader errors are surfaced rather than silently discarded.
- **Retry without restarting CYN:** selecting APEX can retry the native worker if a prior load failed, without starting duplicate inference threads.
- **Correct slow-PC behavior:** when the full model is still initializing, an explicit, user-approved activation stays pending and automatically completes only when the ONNX session is actually ready. On failure, the Hub reports the real error instead of repeating "try again shortly".
- **No fake neural fallback:** CYN will not pretend that neural inference is active when it is not. Existing trained personal AI, music presets, full ONNX weights, GPU/audio device settings and DSP are unchanged.
- **Prerequisite guidance:** if Windows cannot load ONNX Runtime, the AI Hub identifies the DLL startup error and suggests checking Microsoft's Visual C++ x64 runtime. It does not automatically download or install any system software.

## Verification and troubleshooting
- Native Windows x64 builds, ONNX inference smoke test and source regression contracts run in GitHub Actions before publication.
- Exact failure cause on an individual PC cannot be confirmed without its diagnostic output. For missing native prerequisites, use Microsoft's official VC++ x64 redistributable, then restart or retry CYN. For an incomplete APEX model, reinstall the verified CYN installer after fully exiting CYN from the tray.
