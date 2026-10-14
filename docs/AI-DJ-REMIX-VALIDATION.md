# Live remix validation (2.0.29)

The installed 2.0.28 status reported 26 triggered accents and ten background
analyses during investigation. The user clarified that the feature should remix
the song itself for fun. Source-derived effects replace the old additive noises
and tones; this is a behavior change, not a claim that the old counter was zero.

`scripts/test_dj.ps1` runs the original callback regression and the new
`tests/cyn_dj_remix_test.cpp` native suite. The latter processes a synthetic
rhythmic melody through the production DSP at 44.1, 48, 96 and 192 kHz.
All three effects measurably alter the source at 25% intensity: change-to-input
RMS ratios were approximately 0.30 for repeats, 0.13 for echoes and 0.064-0.067
for sweeps, including gaps between effects. These numbers are fixture-specific,
not loudness guarantees for arbitrary music.

The suite checks opposite-phase stereo preservation, no generated sound in an
empty channel, full-intensity output headroom, speech suppression and exact
disabled bypass. Existing tests cover spacing, stale inference, unsupported
formats and capture. Generated original/processed WAV fixtures are available
locally under outputs/cyn-2.0.29; they contain synthesized test music, not captured
user audio. No listening-quality study or exact beat alignment is claimed.

Tempo is an onset-interval estimate with a 120 BPM initial value. Effects run
for two estimated beats, respecting minimum spacing. Three seconds of stereo
history at the maximum supported sample rate use about 4.4 MiB of fixed memory.
No audio callback allocation, model inference, network or file operations were
added. The existing pretrained recognition gates and optional model are reused.

Spotify UI, session discovery, refresh, playback worker, export helper and their
tests are deleted. The installer removes obsolete SPOTIFY.md documentation;
personal credential files are neither opened nor deleted by the new app.
