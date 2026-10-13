# AI DJ Experience

Open **Misc** in the full Cyn window, choose **Download AI DJ**, wait for verification,
then turn on **Enable AI DJ**. The download stays inside Cyn; no browser or manual
file copying is required. Internet access is needed for the initial download only.

The optional file is 90,807,386 bytes (90.8 MB / 86.6 MiB). Cyn's installer does not
contain this file. The app reuses its existing ONNX Runtime, so installing AI DJ
does not install Python, PyTorch or another runtime. Allow about 182 MB of free
disk space for replacing an existing model. Inference uses additional RAM while
enabled; disabling AI DJ unloads the model after the current inference finishes.

Controls:

- **Intensity:** level of the added effects (starts at 25%). Zero adds no effects.
- **Minimum spacing:** at least 4–45 seconds between accents (starts at 18 seconds).
- **Move effects between ears:** pans the added effects while preserving the
  original audio's left/right positions. Turn this off for centred accents.
- **Remove download:** stops DJ inference and deletes its downloaded model.
  Personal Music AI and Movie AI training is preserved. DJ control preferences
  remain so reinstalling restores the controls.

AI DJ starts disabled each time Cyn opens. It supports stereo float audio from
16–192 kHz. Unsupported formats bypass it. Cyn's main power switch also bypasses
DJ processing. An enabled AI DJ works independently of Smart Listening.

## What the AI does

MIT's pretrained 86.6-million-parameter Audio Spectrogram Transformer (AST), in
Xenova's INT8 ONNX conversion, analyses an eight-second rolling mono window on a
background CPU thread, then waits 2.5 seconds before its next analysis. Its 527
AudioSet logits are interpreted with sigmoid probabilities. Music is class 137;
speech, conversation and narration are classes 0, 4 and 5 in the pinned config.

Fresh YAMNet recognition permits accents when music confidence is at least 0.65,
speech is below 0.25 and silence below 0.6. Timely positive AST context lowers the
music threshold to 0.35. Slow AST inference no longer prevents the live DJ from
working. A fast local onset detector schedules short synthesised sweeps, echo
accents and shimmers. Sustained music can also receive an accent after 1.5 times
the chosen minimum spacing, preventing compressed tracks from waiting forever.
The classifier is pretrained audio recognition, **not** a pretrained DJ decision
model or a generative-music model. Effects are synthesised locally, not generated
by a language model. No Mixxx code or executable is included.

The realtime path has fixed buffers and atomic parameters. Model loading,
downloads, spectrograms and inference never execute on the playback callback.
Effect energy is bounded to available output headroom; the dry audio is not
normalised or repanned. Scene permission expires after 40 seconds of processed
audio without a fresh result. Repeated accents respect the chosen spacing.
The existing, faster YAMNet analysis also vetoes speech and silence; if its
results stop arriving, that permission expires within three seconds of audio.
AST inference taking more than 30 seconds cannot lower the live music threshold.
The fast classifier still schedules accents independently; missing, stale or
non-musical live analysis continues to pause effects.

## Storage, provenance and privacy

Storage: `%APPDATA%\Cyn\AI-DJ\ast-audioset-int8.onnx`.
Interrupted downloads use a `.part` file and are removed on cancellation/error;
retry starts a new download. The final filename is installed only after size and
SHA-256 verification. Damaged installed files are rejected before inference.

- Original model: https://huggingface.co/MIT/ast-finetuned-audioset-10-10-0.4593
- Conversion: https://huggingface.co/Xenova/ast-finetuned-audioset-10-10-0.4593
- Pinned revision: `249a1fbf0286b40e7f1ed687a8ae396997bf7dc6`
- File: `onnx/model_quantized.onnx`
- SHA-256: `807d244b58a30eaa89f0a721fb841619c696857aabe4eac93769bd0b61497f61`
- Original AST license: BSD-3-Clause, Copyright (c) 2021 Yuan Gong.
- Original code: https://github.com/YuanGongND/ast

The download goes directly to Hugging Face over HTTPS. Captured audio is held in
a volatile analysis ring; this feature does not save or upload it. No account,
subscription, API key or telemetry is required.

## Experimental limitations

Live audio cannot reliably announce an unseen drop before it happens. Effects
react to detected onsets; this is not beat-grid synchronisation. The eight-second
classification window can lag a scene change. Voice suppression is best effort,
not a guarantee that dialogue, footsteps or important game sounds are protected.
Music is the intended first use. The current effects policy is hand-authored;
it does not claim to be trained on professional DJ performances.

Native tests check bypass, output bounds, spacing, speech gating, stale results,
formats and capture. `scripts/verify_ast.py` compares native features against
the reference Hugging Face frontend and executes the actual quantised model.
Numeric fixture checks are not a listening-quality benchmark.

## Checking activity (2.0.23)

The larger AST model provides optional slower background context. Fresh YAMNet
results must report music >= 0.65 (0.35 with positive context), speech < 0.25
and silence < 0.6. Without fresh results, no accents are allowed.
The status shows which gate is waiting; background context is no longer a
prerequisite. Generated sweeps use bounded current stereo energy, including
opposite-phase stereo material.

Watch **accents this session** to confirm automatic effects have triggered.
Spacing is a minimum rather than a guaranteed interval; audible music and fresh
recognition are required. Try 4-second spacing for testing, then restore your preferred
setting. Pausing playback stops capture; the gathering-audio message is expected
until more audio arrives. You do not need to download the model again after an
app update. Re-enable AI DJ after restarting Cyn.

`Cyn.exe --status` includes `ai_dj` with enabled state, status, actual accent
count, last effect and completed background analysis count. Opening Misc or
enabling the switch does not increment the counters.
