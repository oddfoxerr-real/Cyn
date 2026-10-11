# Cyn starter 1

A real, small neural network: 11 audio features, 12 tanh hidden units and five sigmoid preference outputs. There are 209 learned float parameters. Inference and user retraining run locally in C++; no Python runtime, cloud service or model download is needed in the app.

The optional starter has trained heads for **Bass** and **Clarity**. Ambience, Surround and Dynamic Boost stay at their manual settings until the user supplies More and Less examples and trains. Sensitivity controls the strength of the applied changes. The preset is the baseline; switching Smart listening off restores it.

## Starter data and limitations

Training uses 1,800 generated mixtures, with bass stems and dull/bright variants. A separately seeded set of 600 generated mixtures is held out. See `starter-evaluation.json` for the synthetic results. **These scores do not measure performance on real songs.** No human preference dataset, real-vocal recognition benchmark or listening evaluation has been used. The model may confuse deep vocals and instruments. The starter is experimental and is not a reliable vocal classifier or a guarantee of better sound.

The model learns from spectral ratios, crest factor, stereo width and level. It has no long-term song context or source separation. More/Less labels express a listener's preference, not an objective fact about a song. User training keeps a small starter example set at reduced sample weight; each personal example has five times the weight of a starter example before class balancing. Adjacent frames are correlated, so many frames from one selection do not equal many independently labeled songs.

Windows 2.0.18 adds a **separate low-bass control limit**, not new model weights.
Relative pre-effect 45/75 Hz activity constrains bass proposals when those bands are
weak against 120/200/400 Hz. This index does not use loudness or stereo width and is
not a calibrated probability or source classifier. Saved models are preserved;
their raw output can differ from the limited, smoothed effect setting. Upper-bass
instruments can also be attenuated by this conservative limit. See
`RELEASE_NOTES_2.0.18.md` for tests and limitations.

## Personal learning

The timeline retains at most 60 seconds of **features**, not recorded audio. Selecting a range freezes the displayed timeline; Resume timeline returns to live history. Choose an effect, label More here or Less here, then Train my model. Supply both directions across several varied songs. Each output needs at least five frames in each direction before it is trained. Training runs in a background thread. Relabeling the same selected frame/effect replaces its old label. At most 2,208 examples are retained including starter examples; the oldest personal labels are removed when the limit is exceeded.

Weights and labeled features are stored in `%APPDATA%\Cyn\preferences.json`. No audio recording, song title, account identity or network upload is stored. Reset to starter replaces these labels after an in-app confirmation. There is no claim of real-music validation, calibrated probabilities or a held-out personal accuracy score.

## Reproduce training

From this source directory in VS 2022 Developer PowerShell, with Python and NumPy installed:

```powershell
python training\generate_starter.py prepare
cl /nologo /O2 /std:c++17 /EHsc /Idsp\include training\TrainStarter.cpp /Fe:training\TrainStarter.exe /Fo:training\TrainStarter.obj
.\training\TrainStarter.exe training\generated\train.txt training\generated\result.txt
python training\generate_starter.py finalize
```

Feature extraction in the generator uses steady-state filter frequency responses; live extraction uses the matching causal biquad bank in 50 ms windows. Transient differences are a known domain gap. All starter data is generated locally; there are no third-party training recordings. Code and generated parameters are supplied with the app's AGPL license.

## Hour-trained experimental Cyn base
Gradient optimization: **3600.0s**; **33,175,712 gradient updates**; 49 validated checkpoints.

| Synthetic held-out metric | Original | Hour-trained |
|---|---:|---:|
| Clarity balanced accuracy | 98.63% | 98.61% |
| Clarity log loss | 0.049275 | 0.032193 |
| Bass balanced accuracy | 100.00% | 100.00% |
| Bass log loss | 0.009301 | 0.000150 |

**Limitations:** All examples were synthetically generated. No real music, human preferences, vocal detection or source separation was evaluated. Only Bass and Clarity have starter labels.
Existing personal learning remains intact. Select **Reset to starter** in Smart listening to activate the new bundled base model on an existing installation.
