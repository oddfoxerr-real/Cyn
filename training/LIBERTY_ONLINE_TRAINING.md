# LIBERTY — free online training on real recorded drums

## What this experiment actually trains

CYN Mobile v0.2.31 introduced LIBERTY as **non-generative** adaptive EQ. The current app
reuses AUDONIC's verified control model for its own Library PCM audio, while external
apps get only platform-supported Android session EQ adjustments. **This experiment
does not change the signed APK or override the existing model.**

This branch trains a new independent **kick-onset detector** on real drum recordings.
The label is a kick-drum MIDI note and the input is the same **8 x 20 audio-feature
window** format computed by `phone_engine.cpp::collectPolicy`. The output is a kick
probability, not an audio waveform. It cannot manufacture or sample any music.

It must not be marketed as trained for clarity, voice separation, full music mastering,
creative generation or perceptually better bass without further independent data.

## Online source, license and attribution

- Google Magenta **Groove MIDI Dataset v1.0.0**, aligned played drum WAV and MIDI.
- Dataset homepage: https://magenta.tensorflow.org/datasets/groove
- Author/provider: Google LLC; license **Creative Commons Attribution 4.0 International**.
- Attribution: Gillick, Jon; Roberts, Adam; Engel, Jesse; Eck, Douglas; Bamman, David.
  *Learning to Groove with Inverse Sequence Transformations*, ICML 2019.
- Full original audio ZIP SHA-256 (from publisher):
  `21559feb2f1c96ca53988fd4d7060b1f2afe1d854fb2a8dcea5ff95cf3cce7e9`.
- This workflow reads portions of that publisher-hosted ZIP over HTTPS byte ranges;
  **it does not download and hash the entire 4.76GB file**, so this publisher checksum
  is provenance information, not an assertion of locally verified full-archive bytes.
- No WAV files, MIDI performances or individual training examples are committed
  or uploaded as GitHub artifacts.

Unlike MTG-Jamendo's non-commercial research-only set or FSD50K's mixed individual
sample licenses, this particular dataset permits redistribution and commercial use
subject to CC BY attribution.

## Reproduce using free CPU

The workflow `.github/workflows/liberty-real-audio-training.yml` is triggered on
this branch and uses a standard GitHub-hosted CPU runner (no paid GPU or private
runner). It installs Python 3.11, NumPy, SciPy, mido, libsndfile, FFmpeg fallback,
remotezip, ONNX and ONNX Runtime. The default run:

```bash
python training/train_liberty_groove.py --max-files 850 --seconds-per-file 30 \
  --max-wav-bytes 65000000 --epochs 90 --output liberty-training-output
```

Standard runner limits apply. The job has a 300-minute timeout. The actual number
of recordings decoded, gradient epochs, dataset examples and time are measured
in `evaluation.json`, not assumed in release notes.

It honors Google's official train/validation/test splits; a validation threshold is
chosen without using the test set, then both model and a simple onset baseline are
evaluated on test recordings. `candidate_beats_baseline_f1` is true **only** when the
trained classifier exceeds the baseline by more than one percentage point on held-out
test F1. This comparison is not an audio listening-quality measurement.

## Candidate assets

The workflow uploads **liberty-kick-candidate.onnx**, **evaluation.json**,
**training_history.json**, and a data license note as one GitHub Actions artifact,
only when actual training and ONNX inference succeed. It never uploads a source WAV.
No passwords, user music, SoundCloud tracks or Spotify recordings are used.

## Requirements before shipping in an Android release

1. Read the measured metrics, inspect failure cases and confirm more than baseline
   performance on an *additional independent* corpus of music with vocals and bass.
2. Confirm the exact audio-feature computation and streaming inference on Android,
   with bounded gains and silence/stale-data bypass. Never generate new audio.
3. Measure device CPU, latency, battery, route-switch continuity, and actual sound
   on Bluetooth/headphones; retain a user-selectable fallback to v0.2.31.
4. Sign a new CYN Studio versionCode with its **original** certificate and run the
   prior-version in-place upgrade checks. Do not publish automatically from a
   training job.

This is real supervised training of a small classifier, not a three-billion-parameter
AUDONIC model and not proof of an audio quality improvement.
