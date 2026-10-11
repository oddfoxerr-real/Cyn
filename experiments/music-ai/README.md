# Cyn research: stronger music AI candidates (not yet in the installer)

**Status: model comparison and isolated experiments, not an auto-update.**

Cyn's real-time audio DSP currently uses the YAMNet sound-event classifier and
a local preference controller. Adding a giant foundation model isn't automatically
better: **audio-event recognition, beat timing, and mastering are different tasks**.

| Model | What it actually predicts | Candidate role | License risk / cost |
|---|---|---|---|
| [Beat This!](https://github.com/CPJKU/beat_this) small0 | Offline beats and downbeats | DJ groove structure; compare with fast onset detector | MIT **code and weights**; ~8.1MB checkpoint; whole-track inference, so online delay is unproven. |
| [musicnn](https://github.com/jordipons/musicnn) | Pretrained music tags (techno, electronic, drums, vocals, hip-hop) | Smaller second music tagger | ISC code; bundled pretrained weights need artifact provenance check. Old TensorFlow dependency; test ONNX conversion first. |
| [MusicFM](https://github.com/minzwon/musicfm) | Learned music representations / embeddings | Potential higher-context Music AI teacher; offline preference training | MIT/Apache code; independently verify license for each downloaded checkpoint; CPU latency/size unknown. |
| [LAION CLAP](https://github.com/LAION-AI/CLAP) | Text-audio semantic embeddings | Optional user-described mood presets and scene tagging | Repository code CC0, individual checkpoints/dependencies must be reviewed. Much heavier. |
| [MuQ](https://github.com/tencent-ailab/MuQ) | Music representation, joint music/text embeddings | Research baseline only | MIT **code**, **CC BY-NC 4.0 weights**: cannot bundle in broadly distributed commercial Cyn without authorization. |
| [Essentia MTG](https://essentia.upf.edu/models.html) classifiers | Genre, danceability, mood and musical traits | Reference models / research | MTG model weights are CC BY-NC-SA 4.0 (unless separately licensed); don't bundle without approval. |

## Actual evaluation plan

1. **Prove a pretrained checkpoint loads.** The companion workflow loads
   Beat This! **small0** in an isolated Python 3.11 CPU environment. It generates
   a copyright-free 130 BPM drum/bass synthetic probe and measures tempo output,
   runtime and simple synthetic beat alignment. These metrics are NOT evidence
   of real phonk performance.
2. Use licensed real-music recordings with annotated beat/downbeat positions,
   different tempos and mixed genres. Evaluate onset F-measure, tempo ambiguity,
   robustness to low-frequency bass and Windows x64 CPU/RAM usage.
3. Test a high-speed (DSP-native) spectral flux/onset detector as baseline,
   then compare with Beat This on latency, temporal precision and CPU usage.
4. Test musicnn or MusicFM embeddings against YAMNet on music tagging or human
   labeled comparison, with permissions and checkpoint licensing verified.
5. Train and cross-validate a **mixing preference head** using user A/B
   judgments and separate music/movie preferences. **No candidate directly
   predicts the best bass/EQ** simply from being an audio foundation model.
6. Only ship a model if it improves actual blinded listening comparisons with
   calibrated loudness, peak/true-peak, no clipping and stable frame-time. Run
   inference on a background worker; user audio never goes to cloud by default.

## Most important limitation

Beat This is an accurate offline beat/downbeat model, but **offline inference
over seconds of music is not guaranteed to identify kick timing with subsecond
latency**. A streaming tempo estimation algorithm may be preferable in the
shipping app even if this model wins an offline beat benchmark.

This research workflow makes **no** changes to Cyn's currently shipped YAMNet
processor, live volume controls, AI Hub or theme renderer.
