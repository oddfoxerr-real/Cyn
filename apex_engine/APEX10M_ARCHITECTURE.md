# APEX 10M: Spatial Audio + Audio Reasoning (architecture prototype)

**Status: architecture-only; NOT trained as a 10M audio model; NOT integrated into real-time CYN.**
The existing APEX 6M checkpoint was trained for 225 synthetic steps; that does not
establish perceptual enhancement quality. This work intentionally does not alter
the APEX 6M release, CYN playback driver, or current AI modes.

## Model capacity (verified by PyTorch parameter enumeration)

| Component | Trainable parameters | State |
| --- | ---: | --- |
| Existing APEX-6M feature-mask model | 6,307,477 | Earlier published synthetic checkpoint |
| New spatial analysis and policy head | 3,693,992 | **Random initialization; training required** |
| **Architecture total** | **10,001,469** | Not a fully pretrained 10M model |

The 3,693,992 new parameters are real layers in the forward/backward pass:
a convolutional stem, bidirectional recurrent context encoder, refinement
projection, recurrent policy head, left/right spectral band suggestions, and
an auxiliary coherence path. They are **not filler parameters or fictitious
AI model counts**. The included tests verify shape, gradient flow and counts.

`Apex10M` accepts an actual 6,307,477-parameter APEX6M PyTorch module;
the isolated CI test uses a clearly marked fake base **only** to verify
architecture wiring, not model quality or pretrained-weight correctness.

## What "reasoning" means

`SpatialReasoner` is a **small temporal policy network**, not an LLM.
It emits eight bounded control probabilities per frame:

- Dialogue priority, spatial activity and width target
- Transient protection, bass protection and sibilance protection
- Directional motion activity and decision confidence

The controls are **untrained**; their values are not safe to apply to live
playback. To become useful, train on professionally labeled/derived examples,
test on unseen music, games and films, calibrate confidence, and add temporal
smoothing/hysteresis before controls reach DSP.

The 32-band per-channel spatial suggestions are **not** a source separation
system, HRTF renderer, object-audio decoder or Atmos/DTS decoder. Real spatial
rendering requires sound-source positions, multichannel/object metadata,
appropriate HRTFs (with compatible licensing), head tracking where available,
and latency-safe rendering. Stereo-only content can be respectfully widened
but cannot reliably recover original object positions.

## Stereo-safe DSP baseline

`apex10m_spatial.py` supplies a separate, deterministic reference:

- High-pass-only ambience-side boost, maximum 20% amplitude at full strength
- Centered speech and mono material remain untouched
- Near-anti-phase stereo is bypassed, heavily diffuse content widens less
- Final peak limited to 0.999; 0-strength is exact bypass
- No phase reconstruction, object-position invention, or system integration

The DSP reference is designed for **offline evaluation** and is **not**
proof of improved listening quality. Level-matched listening tests and
mono-downmix/phase-coherence metrics are required before a consumer toggle.

## Free-source AI components to evaluate later

- [DeepFilterNet](https://github.com/Rikorose/DeepFilterNet), MIT/Apache-2.0:
  pretrained *speech* denoising candidate, not intended to be applied
  blindly to music. License, quality and CPU cost require evaluation.
- [Asteroid](https://github.com/asteroid-team/asteroid), open-source:
  evaluate source separation architectures and pretrained checkpoints
  individually; avoid stacking them merely to increase parameter count.
- [SpeechBrain](https://github.com/speechbrain/speechbrain), Apache-2.0:
  evaluation/train recipes for speech-related control labels.

No source files, weights or code from those projects are copied into this
prototype. When incorporating weights, track each artifact's license,
exact parameter count, provenance, checksum, and performance. Models cannot
be assumed to combine correctly merely because source code is available.

## Release gates

1. Pin and verify existing APEX6M checkpoint and compare output with 6M.
2. Train the spatial network on licensed material with proper references.
3. Validate true stereo cues, mono compatibility, intelligibility, transient
   and low-frequency preservation, clipping, and blind level-matched ABX tests.
4. Benchmark real-time inference, latency, memory, CPU and resilience.
5. Only then enable trained policy outputs; unsupported devices default off.

### Local prototype tests

```bash
python -m pip install torch numpy scipy
python -m unittest discover -s tests -p test_apex10m.py -v
```

### Test the conservative stereo DSP stage (not neural inference)

```bash
python -m pip install numpy scipy soundfile
python apex_engine/apex10m_preview.py input.wav output.wav --strength 0.4
```

This runnable CLI never loads random spatial model weights; it only tests
the high-frequency stereo-side enhancement independently of CYN playback.
Compare the resulting WAV against the original at matched loudness.
