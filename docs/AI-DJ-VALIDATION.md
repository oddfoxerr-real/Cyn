# AI DJ validation — Cyn 2.0.22

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

The Python fixture run while compilation was active measured 8.6–20.7 seconds per
inference; these are not unloaded-system benchmarks. Production code discards
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
