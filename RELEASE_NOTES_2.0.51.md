# CYN v2.0.51 — APEX 6M is a real selectable Music AI

The previous APEX AI Hub entry incorrectly remained categorized as **research** despite v2.0.50 having already bundled and tested its full ONNX inference engine. The v2.0.50 early-return also meant selecting APEX disabled most of CYN's adaptive sound controls.

## Fixed

- **CYN APEX 6M** is listed in AI Hub as **Neural Music AI** and can be activated directly; no GitHub research link or separate WAV-processing app is required for live use.
- Uses the same **6,307,477-parameter ONNX model**, verified and bundled in CYN. Checkpoint weights remain unchanged and no new training was performed.
- **Activate selected AI** selects Music AI, enables Smart Listening and waits for ONNX inference to process the existing 48 kHz source. It preserves the previous trained model and preferences.
- **Switch to previous AI** restores the earlier Music AI without clearing the user's learning history.
- Live ONNX spectral-mask statistics now work **alongside**, rather than instead of, CYN's YAMNet scene recognition, expressive bass, dynamics, crescendo tracking, APEX Flow passage awareness, and optional automatic preset cycling. Inference runs on a background thread, never directly in the audio callback.
- APEX appears in the regular Music AI model dropdown, with an accurate runtime-dependent Neural inference ACTIVE/waiting status. Other models and Movie AI remain separate.

## Current capability

This is a functional **neural-driven, real-time adaptive EQ and DSP AI** using the full ONNX inference network. The entire spectral mask does **not** yet reconstruct the live waveform as a low-latency neural denoiser (that feature still exists only in the standalone WAV processing tool). The ONNX checkpoint has only **225 synthetic training steps**; we are **not claiming proven improvement over CYN's flagship or a human listening assessment**. The goal is to provide an honest, working AI for the owner to compare in person.

## Test criteria

Installer version and executable versions align, catalog is no longer `kind: research`, native AI Hub activation/restore works, C++ Windows builds pass, model weights are SHA-256 validated, and the packaged ONNX Runtime performs an actual 513-bin spectral inference on Windows. Switching AI must never upload audio, auto-execute community downloads, or discard the previous model.
