# CYN Studio Android 0.2.12 preview — APEX 10M Spatial Experiment

This Android prerelease adds an optional APEX 10M experimental stereo spatial
processing mode to the **native PCM audio path for music played inside CYN's
library**. The same experimental spatial concept is available for Windows via
the separate APEX 10M desktop spatial WAV preview release.

## Using the spatial preview
Open CYN Studio → Studio → **APEX 10M · Experimental spatial**.
Enable **Experimental stereo enhancement** and adjust **Spatial intensity**
(start at 40%). Listen to a track using CYN's **Library** tab.

The effect selectively increases existing high-frequency stereo side detail;
it does not widen low bass and preserves the summed center. The processing
automatically reduces widening when a signal is strongly out of phase or
already especially wide. An off switch stops adding enhancement. Settings
persist locally on-device.

## What this build does NOT do
- **No trained APEX 10M model is active.** APEX 10M's 10,001,469-parameter
  model architecture (6,307,477 existing + 3,693,992 new trainable spatial
  head) is research source only. Its new head has no trained checkpoint.
- This is a **real-time DSP experimental preview**, not binaural HRTF,
  object-audio decoding, Dolby Atmos, DTS:X, multichannel rendering,
  head tracking, or source separation.
- Third-party apps use Android's device-dependent session effects. Their
  audio is **not** routed through this preview's native PCM processor.
- Stereo content with little spatial information cannot be turned reliably
  into real positional audio.
- No user audio is uploaded.

## Install and compatibility
APK uses the existing `io.github.oddfoxerr.cyn.studio` identity,
Android versionCode 16 and versionName 0.2.12-preview. A verified match
to the existing CYN Studio signing certificate is required before publishing.
The APK is released with a SHA-256 checksum and source archive.

Experimental mode is OFF by default, and existing AI, EQ, Library, Themes,
device connection and update settings remain available. This builds on the
existing Android Studio 0.2.11 source branch without changing Windows stable.

## License
CYN source retains the AGPL-3.0-or-later license and applicable original
FxSound notices. The new experimental spatial DSP code is original CYN work.
