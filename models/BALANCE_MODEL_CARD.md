# Cyn Balance 1 — autonomous audio adjustment prototype

Cyn Balance estimates spectral energy roles in incoming audio. The control
policy combines these estimates with YAMNet scene classification and optional
personal preferences. New installs start with Smart listening enabled
and Music AI selected. Existing saved on/off and mode choices are retained.
Cyn makes its adjustments without reference tracks, labels, cloud access or retraining.
The model is compiled into the executable, so no model download is needed for Balance.
YAMNet retains its existing separate ONNX runtime and model requirements.

## What is integrated

- Google YAMNet: existing pretrained scene recognition and embeddings.
- Cyn Balance 1: new 11-input, 24-hidden-unit, five-output neural network, trained
  to estimate bass, kick, snare, high percussion and other **spectral energy roles**.
- Existing personal preference model: optional refinement; not required for use.
- Native DSP controller: combines those estimates with measured frequency energy,
  crest factor and input peak estimate. It can reduce excess low end and muddy
  low mids, reduce bright percussion, and add modest low EQ when headroom allows.

All new inference runs on the control thread. The existing real-time audio analyzer,
audio callback, driver and signal filters are unchanged. Frequency corrections are
limited to -2.5/+2.7 dB, scaled by sensitivity, and slew by at most 0.2 dB per
control update. Disabling Smart listening restores the manual baseline. Zero
sensitivity also disables autonomous leveling, correcting the previous behavior.
Speech/silence recognized by YAMNet suppresses music shaping. Without YAMNet,
Balance operates at reduced strength. Movie mode retains its existing policy.

## Actual training

See `balance-evaluation.json` for measured metrics, dataset and weight hashes.
6,000 procedural mixtures are used for training, 1,500 for validation and 1,500
for the final test. Every split uses a different deterministic random seed.
Signals include randomized bass, decaying kicks, noise/body snares, high percussion
and harmonic tones. Level, panning, component gain, pitch, decay and onset vary.
The exact shipping `SourceAudioAnalyzer` extracts input features at 16, 44.1 and
48 kHz. Target values are the known energy shares of the original signal sources.
No recordings are scraped, uploaded or supplied by the user.

Adam trains for 300 epochs; validation selects the checkpoint. The held-out test
is evaluated only after selection, and export requires a 20% improvement over a
constant training-mean baseline. Native inference is checked against 64 unseen
Python predictions. These are source-role estimation metrics, **not sound quality
scores**. The test does not establish a real-music improvement.

Reproduce on Windows with VS C++ tools, Python and NumPy:

```powershell
python -m pip install numpy
.\scripts\train_balance.ps1 -Python python
```

## Limitations and next validation gate

Validation currently covers procedural audio only. Bass and kick estimates
show appreciable cross-confusion.
It does not separate instruments, identify sample-accurate onsets or provide
independent kick/snare faders. EQ acts on the complete incoming mix, so overlapping
instruments are affected together. Generated training probes are never played
or used to generate music in the app.

Input peak estimates prevent extra gain on hot input but are **not** an output
true-peak limiter; effects can still produce overs. Listening comparisons, output
true-peak measurements, pumping/latency measurements and real-music evaluation
are required before treating this as a production audio-quality upgrade.

## Additional AI systems researched

| Project | Useful role | Integration decision |
| --- | --- | --- |
| [DrumSep](https://github.com/inagoy/drumsep) | Separates drum recordings into kick, snare, cymbals and toms | Candidate offline teacher; not integrated. Needs a drum stem and checkpoint/runtime validation before live use. |
| [Diff-MST](https://arxiv.org/abs/2407.08889) | Reference-guided multitrack mixing | Research reference; requires a reference song rather than autonomous input analysis. Not integrated. |
| [dasp-pytorch](https://github.com/csteinmetz1/dasp-pytorch) | Differentiable EQ/dynamics for training controllers | Training framework candidate, not a pretrained AI. Not integrated. |
| [Beat This!](https://github.com/CPJKU/beat_this) | Beat/downbeat estimation | Existing repository experiment only; not presented as an active live mixer. |

No third-party weights from these candidates are bundled or claimed to have been
trained. This change combines the existing Google model with the newly trained
native Cyn model. Model code and procedural data generator are AGPL-3.0-or-later.
