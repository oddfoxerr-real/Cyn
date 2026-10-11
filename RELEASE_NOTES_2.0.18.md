# Cyn 2.0.18 — Native YAMNet Audio Understanding

This release integrates Google's **pretrained YAMNet** as Cyn's asynchronous audio-perception model. Unlike earlier tests, the production C++ worker reads real pre-effects playback PCM, downmixes it to a bounded 16 kHz mono ring and executes a verified ONNX YAMNet graph on a low-priority CPU worker.

## Improvements
- 521 AudioSet event scores from the official YAMNet model; use speech, music and explosion/gunfire likelihoods as scene context for Music and Movie AI
- Music and Movie profiles remain independent and preserve your saved personal learning
- Movie AI raises clarity more conservatively when YAMNet detects speech and limits bass boosts during impacts
- Music AI also guards against excessively aggressive bass boosts
- 16 kHz mono bounded lock-free buffer; **no ONNX execution, disk I/O or blocking calls from audio callback**
- 0.975-second pretrained model window, background inference throttled to limit CPU usage
- Bundled tested YAMNet ONNX model + ONNX Runtime x64 DLL + license/manifest files
- Safe fallback to existing personal neural model if YAMNet runtime is missing, unavailable, slow, or fails
- Native activity display remains tied to the personal preference neural model; it is not YAMNet's internal activation map

## Validation
- Official pretrained TensorFlow Hub YAMNet loaded successfully
- Exported ONNX graph passed numerical comparison against TensorFlow on multiple synthetic signals
- Windows x64 ONNX Runtime CPU inference and the full native Cyn C++ build passed
- Installer pipeline verifies the model and runtime files before publication, and SHA-256 checks the installer

**Limitations:** These are model-compatibility and compilation tests, not a benchmark of real streaming movies, speech intelligibility, subjective audio quality or every Windows device. YAMNet event labels are not direct sound preferences. This release is an experimental integration, not a guarantee of better movie mixing. Users retain manual controls and personal feedback.

**Upstream licensing:** Google's YAMNet model and TensorFlow Models code under their published licensing terms; ONNX Runtime's MIT license and attribution are included in the installation.
