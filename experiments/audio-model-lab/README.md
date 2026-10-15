# Cyn Open Audio AI Lab

**Stage: evaluate open-source candidates before training. No release, no default-AI change.**

Cyn already has an asynchronous native YAMNet scene worker and a separate Beat This!
offline research probe. This lab adds **real pretrained** CPU inference probes for
Silero VAD (speech activity) and DeepFilterNet (speech denoising). They run in
isolated GitHub Actions jobs using free standard Linux runners. The model
weights are acquired from official package sources during CI, not committed
or bundled in Cyn's Windows installer.

## What these models can actually do

| Model | Added evaluation | What it cannot tell us |
| --- | --- | --- |
| Google YAMNet | Existing native worker and compatibility probe | The ideal EQ or bass level |
| Beat This! small0 | Existing pretrained offline beat probe | Guaranteed real-time beat synchronization |
| Silero VAD | New pretrained ONNX speech-activity inference and timing | Whether vocals sound more natural |
| DeepFilterNet | New pretrained denoising inference and timing | Whether a phonk song sounds better |

More reviewed candidates are in [catalog.json](catalog.json).
This is a **curated compatibility shortlist**, not every open-source AI on the internet.
Voice denoisers should not be stacked blindly onto music. Some pretrained weights
are noncommercial, have unclear rights, or have CPU latency too high for Cyn.
Code and checkpoint licensing must be checked separately before redistribution.

## Free CI

Run the PR check named **Cyn Open Audio AI Evaluation**. No GPU or paid
runner, training or release is requested. It produces three independent
jobs: catalog/safety checks, Silero pretrained inference, and DeepFilterNet
pretrained inference. JSON metrics are uploaded as Actions artifacts.
Existing full YAMNet and Beat This benchmarks remain in their own workflows.

Locally, using Python 3.11 on Linux:

1. Run `python -m unittest discover -s experiments/audio-model-lab -p test_catalog.py`.
2. Install CPU-only PyTorch, then `pip install 'silero-vad[onnx-cpu]==6.2.3' numpy==1.26.4`.
3. Run `python experiments/audio-model-lab/probe_silero.py`.
4. In a separate environment install `deepfilternet==0.5.6` and run
   `python experiments/audio-model-lab/probe_deepfilternet.py`.

## Required promotion gates (not yet passed)

- Verify actual voice detection against permission-cleared dialogue, silence and music.
- Verify speech enhancement on clean/noisy real voice pairs with delay-aligned
  SNR/PESQ/STOI and independent listening evaluations; synthetic RMS is not quality.
- Compare beat timing against annotated, licensed real music and the native baseline.
- Measure Windows x64 startup time, ongoing CPU and RAM, dropout rate and DSP-frame
  latency under real music/movie playback.
- Assess licenses of **each checkpoint** and preserve notices before bundling.
- Integrate only the winning model into an optional background worker with bounded
  buffering and a safe bypass. The sound callback must never wait for neural inference.
- Train a separate preference controller **only after** we have useful labels;
  an audio classifier alone does not teach ideal enhancements.

There is deliberately no automatic Windows release in this experiment.
The existing flagship remains unchanged until comparative metrics justify integration.
