# CYN APEX-6M model card — 2026-10-17

## Architecture

PyTorch module `Apex6M`: 6,307,477 trainable parameters. 48,000 Hz waveform STFT (1024 FFT points, 256 sample hop). Input shape `[batch, 513 frequency bins, time frames]` of `log1p` magnitude. 224 feature channels, 14 temporal residual blocks with depthwise 7-tap kernels and expansion MLP ratio 4. One real [0,1] magnitude mask and a 6-logit auxiliary synthetic scene head. Phase preserved. Waveform reconstructed with inverse STFT.

## Training provenance

100 AdamW gradient steps from seeded initialization (seed 20261017), followed by 125 steps resumed from its model weights with a fresh Adam optimizer. 2 examples per minibatch, 48 kHz procedural audio recipes. Models and data are synthesized in the bundled source; no recordings or third-party model weights are bundled. Training uses a combined log magnitude L1 error and weak BCE loss of synthetic scene labels.

## Metrics

96 fresh held-out synthetic examples (seed 910100), different from training seeds and with intentionally modified noise coloration and pitch ranges. SI-SDR in dB: input 0.1389, spectral gate 2.6255, Apex 5.7100. This metric is objective only for the synthetic noise-suppression target. It does not establish professional audio quality, interference-free instrument isolation, human-speech intelligibility, source fidelity on commercial songs or real-time headphone compatibility.

## Risk and limitations

1. Synthetic voice-like harmonics are not speech; loudness/genre control scene logits are uncalibrated on real audio.
2. Masks only attenuate magnitude; no artifact-free separation or improvement across all tracks is guaranteed.
3. Current model is non-causal and uses offline overlapping 1s windows; inference compute time is not end-to-end low-latency playback proof.
4. 48kHz audio processing does not mean professional-grade sample-rate transparency or mastering.
5. No comparative commercial benchmark has been run against Moises, LALAL.AI, Sonible or Endel.
6. CYN Experimental 13h predicts Bass/Clarity preferences, so SI-SDR results are **not comparable**.
7. User should only use the included experimental demo or a copy of non-critical audio files; output is separately written, never replacing the original WAV.

## License and integration

New source AGPL-3.0-or-later. Weight file created by included training code and synthetic samples. For native CYN use, implement a dedicated inference worker and keep original DSP in default bypass until compatibility and listening benchmarks pass.