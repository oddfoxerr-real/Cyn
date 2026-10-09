# Cyn starter 1

A real, small neural network: 11 audio features, 12 tanh hidden units and five sigmoid preference outputs. There are 209 learned float parameters. Inference and user retraining run locally in C++; no Python runtime, cloud service or model download is needed in the app.

The optional starter has trained heads for **Bass** and **Clarity**. Ambience, Surround and Dynamic Boost stay at their manual settings until the user supplies More and Less examples and trains. Sensitivity controls the strength of the applied changes. The preset is the baseline; switching Smart listening off restores it.

## Starter data and limitations

Training uses 1,800 generated mixtures, with bass stems and dull/bright variants. A separately seeded set of 600 generated mixtures is held out. See `starter-evaluation.json` for the synthetic results. **These scores do not measure performance on real songs.** No human preference dataset, real-vocal recognition benchmark or listening evaluation has been used. The model may confuse deep vocals and instruments. The starter is experimental and is not a reliable vocal classifier or a guarantee of better sound.

The model learns from spectral ratios, crest factor, stereo width and level. It has no long-term song context or source separation. More/Less labels express a listener's preference, not an objective fact about a song. User training keeps a small starter example set at reduced sample weight; each personal example has five times the weight of a starter example before class balancing. Adjacent frames are correlated, so many frames from one selection do not equal many independently labeled songs.

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
