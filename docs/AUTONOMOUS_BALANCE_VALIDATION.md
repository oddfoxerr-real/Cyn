# Autonomous Balance prototype validation

Validated locally on Windows x64, 11 October 2026, against base commit
`85e4dd69c16b6336f006d8c974577bbb1e3c753e`.

- Full Release/x64 Visual Studio application compilation and link: passed.
  Existing application/DSP compiler warnings remain; this is not a warnings-free build.
- New native Balance tests: passed. 64 Python/C++ prediction parity cases,
  signed bass correction, silence/speech/invalid-input/headroom guards,
  1,000 hostile-input bounded-control cases, and shipping feature extraction
  at 16/44.1/48/96 kHz.
- Existing flagship policy and personal audio tests: passed.
- Native YAMNet wiring and AI Hub regression scripts: passed.
- Default startup wiring: Smart listening enabled and Music AI selected for
  new installs, with saved choices and initial baseline capture preserved.
- Training: 6,000 original synthetic mixtures; validation 1,500; test 1,500.
  300 epochs, 14,100 optimizer steps. Test MSE 0.0195693 versus the constant
  training-mean baseline 0.0789423 (75.2% lower source-role estimation MSE).
  Repeated training reproduced the same dataset and weight hashes.

The application was compiled but not installed or live-auditioned. No real-music
quality score, sample-accurate drum separation, output true-peak compliance,
or independent kick/snare gain control is established by these checks.
YAMNet weights were not retrained. The added Balance network was trained.

This branch is a reviewable prototype. Production release requires licensed
real-music evaluation and output/audio-device listening checks. See
`models/BALANCE_MODEL_CARD.md` for the architecture, source research and limits.
