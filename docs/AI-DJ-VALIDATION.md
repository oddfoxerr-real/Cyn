# AI DJ validation â€” Cyn 2.0.22

Validation applies to the optional AST classifier and Cyn's new effects layer.
The model weights remain separate from the application installer.

## Completed automated checks

- Native C++ DSP regressions: exact dry bypass while disabled, at zero intensity,
  and with main power off; real nonzero effects; bounded output; minimum event
  spacing; speech veto; stale analysis; unsupported formats; and ring-buffer
  capture/reset. The fast YAMNet veto is independent of the larger AST classifier.
- Native production worker lifecycle using an isolated test directory and
  synthetic capture: actual ONNX Runtime DLL loading and inference; disable,
  unload and remove; cancelled HTTPS download; successful retry of the full
  90,807,386-byte file; SHA-256 match; and rejection of a deliberately corrupted
  full-size model. This runs the same worker/download code used by the UI.
- Native AST frontend compared with Hugging Face Transformers 4.46.3's reference
  audio utilities for silence, tone, noise and synthetic rhythm. Maximum feature
  difference: 1.1920928955078125e-7. The actual ONNX model returns 527 finite logits.
  Maximum sigmoid probability difference: 0.0057417154; top class and DJ gating
  decisions match for all four fixtures. INT8 dynamic activation quantisation
  amplifies tiny input rounding differences in the noise fixture; raw logits are
  not claimed to be bit-exact. Full numbers: `tests/data/dj/ast-validation.json`.
- Existing AI Hub, native YAMNet wiring and updater XAML checks.
- Final Windows x64 release build: succeeded with zero errors (98 upstream and
  conversion/deprecation warnings). Inno Setup installer compilation succeeded.
  Packaged executable file/product version is 2.0.22.0. Portable payload inspection
  confirms the existing YAMNet/runtime and AST license, without AST model weights.

Interactive visual inspection was interrupted while the installed older Cyn was
  active; the new interface has not had a completed visual or listening check.

## Performance and limits

The Python fixture run while compilation was active measured 8.6â€“20.7 seconds per
inference; these are not unloaded-system benchmarks. The 2.0.22 release discarded
decisions taking over six seconds, reports that analysis is behind playback,
and leaves accents paused. Inference has two CPU threads and never executes in
the playback callback. The independently running existing YAMNet classifier
vetoes speech/silence; its permission expires if telemetry stops arriving.

Tests do not establish subjective musical quality, perfect beat alignment,
perfect speech protection, game-event understanding, or compatibility with every
audio device. No clean-machine driver installation or human listening study is
claimed. The native lifecycle test uses synthetic capture, not the user's audio.
The original FxSound driver and its binary payload are unchanged.

## Reproducing

Run `scripts/test_dj.ps1` with Visual Studio C++ tools. For model parity, install
the development-only requirements described by `scripts/verify_ast.py`, place
the pinned model and config in `.build-tools/dj`, then run that script.
`scripts/test_dj_lifecycle.ps1` additionally requires the pinned native runtime
from `PREPARE_CYN_YAMNET.ps1` and internet access. Its storage stays under
`.build-tools/dj/lifecycle-storage`. No test dependencies ship with the app.

## 2.0.23 regression fix

Live inspection of 2.0.22 confirmed an installed/enabled model with zero accents
and “Analysis is behind playback - effects paused” during actual Spotify playback.
Windows endpoint diagnostics confirmed audio through FxSound and Cyn to the JBL
headphones. Paused playback separately explained the gathering-audio messages.

2.0.23 allows 8–21-second analysis (the measured range above) as background context,
with a 30-second maximum and 40-second playback lease. Fresh YAMNet must positively
detect current music as well as pass voice/silence checks. Native regressions cover
these duration boundaries, missing/nonmusic/speech veto, actual accents after the
old 12-second lease, and expiry at the new boundary while live music remains fresh.
The model/frontend/download bytes are unchanged. No live listening success is
claimed until the updated application is exercised during playback.

## 2.0.28 validation

- Native DSP tests passed: exact bypass, nonzero added audio, headroom, spacing,
  speech and stale-analysis suppression, power and formats, sustained music,
  opposite-phase stereo, and silence after continuous music.
- Production model lifecycle passed using the actual pinned AST ONNX and native
  runtime: current music permits effects while context warms; inference completes;
  disable/unload/remove, HTTPS cancel/retry/download, hash and corruption rejection
  pass. The initial sandbox network failure was repeated successfully with access.
- Spotify native tests passed with injected transport and synthetic credentials:
  EMBER and Spotipy JSON shapes, refresh/rotation, preserved fields, controls,
  track/podcast display, missing playback, 429 handling and persisted disconnect.
- Regression checks for current theme atmospheres, applied AI controls, audio
  repair, AI Hub and YAMNet passed after merging the latest 2.0.27 source.
- Live Spotify account, physical-device listening and native visual QA have not
  been performed in this session. The user has supplied EMBER source references
  but not the private Spotify session JSON.

The current live music threshold is 0.65 without timely positive AST context,
0.35 with it. Slow AST no longer blocks accents. Continuous music may receive
an accent after 1.5 times minimum spacing. Previous 2.0.23 notes above describe
an unreleased intermediate fix, not the published theme release of that number.
