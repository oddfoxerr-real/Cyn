# CYN APEX 6M v1.0.0 — Full Neural Audio Preview

This is the **first actually runnable 6,307,477-parameter APEX model**. The executable runs the existing checkpoint through ONNX Runtime on your own PC, rather than loading a small Cyn preference profile or changing a visual indicator.

## How to test

1. Download **CYN-APEX-6M.exe**, open it and select an ordinary **WAV** track.
2. Choose an output WAV path and a processing strength (try 35–65% first).
3. Press **Process with APEX**. The full neural mask is executed over the audio and writes a real processed WAV.
4. Listen to the original and processed versions at matched volume. Use short 10–30-second samples initially.

Supports mono/stereo WAV and 8–192 kHz source rates. The neural model internally operates at 48 kHz. Music may sound worse; this model's current training objective is **synthetic denoising**, not perceptual mastering, music flow or instrument separation.

## Training remains on hold

- Exactly **6,307,477** model parameters are executed from a PyTorch checkpoint converted to ONNX.
- The current weights were **already trained for 225 steps on synthetic audio** before this release.
- **New training steps in this release: 0**. No further training or fine-tuning is performed until the user has tested it.
- Includes a first-class ONNX parity check against PyTorch and a WAV processing test.

## What it does NOT do

This is a **standalone offline file processor**; it is **not yet plugged into CYN's real-time system-wide playback driver** or the existing small-model AI Hub importer. It is not a replacement for YAMNet or an upgrade to the CYN live music preference model. Work on the live driver path will need its own safety, synchronization, latency and quality testing.

Processing is local to your PC; the app does not upload audio. There is no music-generation, streaming, or learning telemetry.
