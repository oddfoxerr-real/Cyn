# CYN APEX 10M Experimental v0.1.0

**Experimental standalone WAV spatial preview — not a trained neural APEX 10M release.**

This prerelease publishes the new 10,001,469-parameter APEX 10M architecture
(6,307,477 existing APEX 6M parameters + 3,693,992 trainable spatial reasoning
parameters), conservative offline stereo processing, and source/test files.

**Important:** The newly added 3,693,992 parameters have not been trained or
published as usable pretrained weights. The downloadable standalone app runs a
deterministic stereo spatial DSP preview, **not** live APEX 10M neural inference.
The earlier APEX 6M checkpoint is not bundled into this experimental EXE.

## Download and test

Download **CYN-APEX-10M-Experimental-Windows-x64.zip** or the standalone
**CYN-APEX-10M-Experimental.exe**. Launch the app, choose a stereo WAV,
select a strength (40% is a reasonable starting setting), and save the
processed WAV to a new location.

The processor can enhance *existing* high-frequency stereo side information
while keeping centered dialogue and low-frequency bass largely untouched.
It does not derive real spatial position from two channels; it does not decode
Dolby Atmos, DTS:X, spatial-object metadata or multichannel surround audio.
Mono audio is not supported by the preview GUI.

- Offline only; does not affect Spotify, system playback or CYN's DSP driver
- No network audio upload or new training
- Mono/strongly anti-phase content is not spatially widened
- For audio quality evaluation, compare at matched volume and test mono sum

If you want command-line WAV processing, use the included source in the tagged
repository: `python apex_engine/apex10m_preview.py in.wav out.wav --strength 0.4`
with NumPy, SciPy and SoundFile installed.

## Architecture and safeguards

- 8 temporal learned control channels for dialogue priority, spatial activity,
  width targeting, transient/bass/sibilance protection, motion and confidence
- 32 stereo band suggestions (untrained and disconnected from audio output)
- Gradient, parameter-count and stereo safety test suite in
  `tests/test_apex10m.py` and `tests/test_apex10m_preview.py`
- Original APEX 6M and stable CYN application remain unchanged

Next stage: train the new branch on appropriately licensed spatial audio,
benchmark CPU/latency, evaluate true quality improvements and phase behavior,
then consider a separately gated real-time integration.

## License and upstream attribution

Repository source: https://github.com/oddfoxerr-real/Cyn

CYN repository and applicable inherited portions retain their AGPL-3.0 and
upstream FxSound notices. This experimental preview is an independent CYN
addition; it does not claim affiliation with audio-format or plugin vendors.
Use the repository license and included source for redistribution obligations.

**Known limitations:** no trained 10M checkpoint, no real-time playback, no
head-tracked binaural renderer, no automatic music quality guarantee.
