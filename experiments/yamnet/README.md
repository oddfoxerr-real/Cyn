# Cyn / YAMNet feasibility probe

Status: **experimental compatibility test — NOT bundled into Cyn or selected as its default AI.**

This folder runs the **actual pretrained Google YAMNet network**, not a
heuristic intended to look like YAMNet. GitHub Actions downloads the official
model from [TF Hub](https://tfhub.dev/google/yamnet/1) and checks that inference
produces **521 AudioSet sound-event scores** and **1,024-dimensional embeddings**.
The probe makes a synthetic tone only to exercise the model, not to benchmark
music or movie detection. See the
[compatibility workflow](../../.github/workflows/cyn-yamnet-compat.yml).

## Why not overwrite Cyn Starter with this file?

Cyn's current \`PersonalAudioModel\` takes eleven spectral statistics and
predicts five sound-effect preferences with 209 trainable weights. The model
works directly on audio feature summaries; **YAMNet is a different architecture
and cannot be loaded as a \`.cynai.json\` profile**. It expects **raw mono 16 kHz
PCM**, extracts spectrogram patches approximately every 0.48 seconds, and
classifies 521 categories such as speech, music, impacts, and environmental
sounds. It does not directly learn optimal DSP effect levels.

Correct eventual design:

\`\`\`
Windows audio PCM
  |-> existing passthrough/DSP immediately (audio never waits on AI)
  |-> bounded sample tap / downmix / resampling to 16 kHz mono
       -> separate low-priority CPU worker / YAMNet inference
       -> 521 smoothed event scores + 1,024-d embedding
       -> speech/music/impact likelihoods with hysteresis
       -> supervised preference head + gentle DSP adjustments
       -> fallback to current PersonalAudioModel on CPU/availability failure
\`\`\`

## Requirements before shipping

1. Use a supported **native inference runtime** (TFLite C API or ONNX Runtime)
   and a verified, conversion-compatible YAMNet model. Do not put Python or
   TensorFlow into the audio callback.
2. Acquire source PCM from Cyn's DSP input safely. The existing
   \`SourceAudioAnalyzer\` exports only eleven spectral features; those are
   **insufficient to recover the original waveform**. Sampling/downmix and
   bounded lock-free buffering must never allocate, block, or drop audio.
3. Validate latency, memory, licensing, Windows x64 packaging, idle CPU
   utilization, and output-device changes on actual hardware.
4. Avoid mistaking audio event scores for taste: impose hard caps on Bass,
   scene-aware smoothing, and separate preference feedback. Movie mode should
   preserve dialogue and dynamics rather than automatically maximizing Bass.
5. Use independent movie/game/music audio and human listening evaluation
   before promoting the model to the default engine.

The YAMNet implementation, original weights and its upstream dependencies
require their applicable licenses and attribution (see Google's
[upstream project](https://github.com/tensorflow/models/tree/master/research/audioset/yamnet)
and its Apache 2.0 code license). Never describe the compatibility probe as
a production integration.

Local experiment (Python 3.11 with workflow's pinned dependencies):

\`\`\`shell
python experiments/yamnet/probe_yamnet.py --json yamnet-test-report.json
python experiments/yamnet/probe_yamnet.py --wav "movie_excerpt.wav"
\`\`\`
