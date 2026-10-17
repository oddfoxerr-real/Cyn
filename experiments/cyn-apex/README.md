# CYN APEX-6M — independent experimental neural audio engine

**Status:** source-only Git branch plus a separate downloadable trained checkpoint for synthetic-noise removal, not a new CYN installer or a proven commercial-AI replacement.

APEX-6M is a **6,307,477 trainable-parameter neural network** implemented in PyTorch. It operates on 48-kHz PCM audio and predicts a time-varying, per-frequency denoising mask from a complex STFT magnitude plus six auxiliary, synthetic-label audio-scene scores. The network contains 14 depthwise-convolution temporal residual blocks with trainable scaling, and an auxiliary scene head. It is a new model, not a merger or rebranding of 30 incompatible external checkpoints.

The checkpoint included in the downloadable experiment bundle (not checked into Git) was **actually optimized for 225 gradient steps** on procedural bass/drum/lead/harmonic sound mixtures with added white, colored and high-frequency noise. The speech-like synthetic generator is *not* human speech.

## What it does NOW

- Fully local/offline WAV denoising with an actual trained neural model (no cloud uploads).
- Loads and processes 48-kHz mono or stereo PCM WAV files, including longer files using overlapping 1-second segments.
- Supports dry/wet control (`--wet 0.0` for bypass, `--wet 1.0` for fully processed). Keeps default processing optional.
- Includes reusable model code, trainer, synthesizer, deterministic unit tests, CPU benchmark and before/after reference audio.
- Runs research offline only. It is **not connected to the CYN audio driver**, doesn't replace its default AI, doesn't download third-party weights, and doesn't claim to isolate real vocals/instruments.

## Verified benchmark (synthetic 48 kHz, 96 held-out samples)

| Processing | Mean SI-SDR (higher is better) |
| --- | ---: |
| Noisy unprocessed input | 0.1389 dB |
| Classical stationary spectral gate (same inputs) | 2.6255 dB |
| APEX-6M, 225-step checkpoint | **5.7100 dB** |

Mean APEX improvement vs input: **+5.5711 dB**. Mean improvement vs classical gate: **+3.0844 dB**. APEX beat the gate on 75% of examples. One-second CPU inference including STFT took **~24 ms median** in the local Linux test environment with 2 PyTorch threads. Results are reproducible using `python -m apex.benchmark`.

**These scores cannot show that APEX sounds better than Moises, LALAL.AI, Sonible or CYN Experimental 13h.** Unlike CYN's 13h Bass/Clarity preference predictor, APEX outputs a waveform denoising mask. Their tasks and objectives are different and cannot be compared using the above SI-SDR scores. Real-speech/music, professionally mixed stems, 44.1/48 kHz mastering, true-peak, Windows driver integration and blinded listening tests are still necessary. In addition, the current network is *non-causal*, making it inappropriate for a low-latency playback callback without redesign.

## Windows installation (Python 3.11 or newer, x64)

```powershell
py -3.11 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
.\.venv\Scripts\python.exe -m unittest discover -s tests -v
.\.venv\Scripts\python.exe -m apex.infer demo\synthetic_noisy_input.wav demo\new_output.wav --checkpoint weights\apex6m-225.pt --wet 0.85
.\.venv\Scripts\python.exe -m apex.benchmark --checkpoint weights\apex6m-225.pt --trials 96
```

Linux/macOS: `python3 -m venv .venv`, activate the virtual environment and run the same `python -m ...` commands.

`requirements.txt` pins minimum versions for compatibility; first install will download PyTorch. **Do not use 8-kHz model settings**: this is a 48-kHz model, though it is still early research and real-audio fidelity is unverified. Convert source files to 48-kHz lossless WAV separately if required.

## Experimental training

```powershell
.\.venv\Scripts\python.exe -m apex.train --steps 100 --batch-size 2 --threads 2 --output reports\new-training
```

Training is stochastic but seeded. It checkpoints every 25 steps, preventing loss when execution is interrupted. To continue from included 225-step weights:

```powershell
.\.venv\Scripts\python.exe -m apex.train --steps 50 --resume weights\apex6m-225.pt --output reports\continued
```

This continuation resets the optimizer, since only model parameters, not Adam moments, are saved in the distributed checkpoint. Checkpoint filenames stay `apex6m-225.pt` for consistency but the internal `training_steps` records the actual new count.

## Safety, distribution and scope

- Never put PyTorch inference inside the Windows audio callback. Use an optional bounded worker that cannot delay playback.
- Release gate: only ship after checkpoint integrity checks, real-audio objective testing, blinded volume-matched A/B tests, clipping / true-peak evaluation and Windows device stability tests.
- No music/speech recordings are included. Demo and training audio are algorithmically synthesized here. No cloud, copyrighted stems or proprietary competitor code.
- License for the new source implementation: AGPL-3.0-or-later to match the CYN repository. The checkpoint was produced from synthetic data created by this code. See `MODEL_CARD.md`.

## Paths

- `apex/model.py`: 6.307M parameter architecture, STFT / mask / scene head
- `apex/data.py`: deterministic synthetic mixtures for training and fresh evaluation
- `apex/train.py`: actual PyTorch training and evaluation
- `apex/benchmark.py`: matched naive vs classical gate vs APEX test
- `apex/infer.py`: offline mono/stereo WAV processing
- `tests/test_apex.py`: sanity, determinism, parameter count and audio-shape assertions
- `weights/apex6m-225.pt`: trained 225-step model state (~25 MB), distributed in the downloadable ZIP; CI creates separate 15-step smoke-test weights
- `reports/benchmark-final.json`: fixed-seed test metrics
- `demo/`: synthetic noisy input, clean reference and processed output WAVs