# AUDONIC AI — Creative Mischief Engine v0.1

AUDONIC is a **separate, mutually exclusive experimental CYN audio AI**. It turns measured music features into precisely timed, musically bounded creative processing: controlled high-frequency harmonic drive, transient bite, stereo motion, colour filter, rhythmic shaping and bass harmonics.

This release is a **genuinely trained small neural model**, not a fake 3-billion-parameter checkpoint. Its mobile model accepts the existing 8-channel, 20-frame CYN audio feature window, runs ONNX asynchronously, and sends six parameters to a bounded native DSP that never runs network inference on the audio thread. It uses APEX 10M's real trained policy as a **teacher**, distilling bass, kick and spatial restraint without illegally concatenating neural weights. The original APEX 6M and YAMNet models are retained for other CYN profiles.

## 3B AUDONIC architecture

The planned high-capacity teacher is a **3,000,000,000-parameter offline musical planner** for event anticipation and effect choreography, with memory for context, style and deliberate restraint. No 3B weights have been trained; existing infrastructure cannot produce a working 3B model simply by declaring the parameter count. FP16 alone requires approximately 6 GB for weights; training requires much more memory/compute and diverse licensed music with human feedback.

The **running mobile AI** is a deliberately smaller trained streaming policy. Its six output decisions control a stable, independent DSP. Keeping the 3B teacher separate from the low-latency renderer avoids pushing large-model inference onto the 48-kHz callback. Full future teacher training would require additional compute and reviewed data; never label v0.1 as 3B.

## Training and inheritance

- **APEX 10M**: verified SHA-256 ONNX teacher from `apex10m-experimental-rc-v1`, trained to 95.97% of its planned 15-hour synthetic curriculum. During AUDONIC student training, its *inference outputs* inform coherent spatial control, kick and bass protection. No incompatible model tensor merge.
- **APEX 6M**: previous 6.3M trained model stays in CYN for its own AI profile; this first student does **not** directly distill its inference, because its 513x189 spectral-mask input differs from AUDONIC's streaming 8x20 feature window. No false claim of joint model training.
- **YAMNet**: existing event recognition remains available in CYN's shared scene-inference infrastructure and is not copied into AUDONIC weights.
- **AUDONIC student**: performs real PyTorch gradient updates on a procedural creative-effect timing curriculum, with APEX 10M teacher supervision when the verified model is supplied. See `audonic-manifest.json` for exact optimizer update count, elapsed training duration and model parameter count.
- **Public music**: license-gated `audonic/public_data.py` extracts locally from verified Slakh/BabySlakh recordings if supplied. This v0.1 GitHub workflow does **not** download huge third-party datasets and therefore reports `public_slakh_features_used=false`. A real-data fine-tune must be independently run and measured before claiming those results.
- **Human-rated music**: no preference or listening-test dataset yet. Model predictions are research heuristics, not evidence that listeners prefer AUDONIC over APEX.

## Safe effect gates

All real-time creative effects are bounded; bypass is bit-transparent after resetting the DSP, the mono signal stays mono, and a post-FX CYN limiter prevents unexpected peaks. No direct speaker hardware controls. External Android apps use their own Android EQ session integration; AUDONIC's true PCM DSP is available only for CYN Library playback in v0.1. Device-aware profiles need actual measurements and permission. Every AI has one exclusive selected profile.

## Licenses and sources

- Slakh2100 / BabySlakh: CC BY 4.0; source: https://www.slakh.com/ ; Zenodo: https://zenodo.org/records/4599666 ; https://zenodo.org/records/4603870 . Credit: Manilow, Wichern, Seetharaman and Le Roux (WASPAA 2019). The full set is about 104 GB; BabySlakh about 883 MB, so neither is silently downloaded.
- MTG-Jamendo: mixed per-track licenses, and the dataset's own stated condition restricts use to noncommercial research without written authorization. NOT used in v0.1: https://github.com/MTG/mtg-jamendo-dataset
- FSD50K: mixed per-clip Creative Commons including NC and Sampling+. NOT used in v0.1 absent individual license audit: https://zenodo.org/records/4060432
- MUSDB18 / MAESTRO: not cleared for a commercial CYN audio-training release; excluded.
- FxSound credit/license remain present in CYN and unchanged.

## Reproduce a local neural training run

```bash
python -m pip install torch numpy onnx onnxruntime scipy soundfile
python audonic/test_audonic.py
python audonic/train.py --steps 600 --teacher teacher/apex10m.onnx --output dist/audonic
```

`--teacher` is optional for unit preflights, but the official trained AUDONIC model requires it and the CI verifies its SHA-256. Optional public-data training requires a self-provided manifest, exact checksums, licensed source files and `--public-features`. No music or account data is uploaded.
