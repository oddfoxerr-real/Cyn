# CYN Windows 2.0.75 — APEX 10M Experimental Spatial DSP

**Experimental prerelease.** This build adds a real-time, opt-in stereo spatial
DSP preview to the native Windows CYN audio processing path while keeping the
existing FxSound equalizer, neural APEX 6M, YAMNet, AI Hub, themes, and driver.

### Fixed in this preview
- APEX 10M is listed directly in the AI Hub alongside APEX 6M and saved Music AI profiles.
- Selecting APEX 10M switches the active primary AI rather than stacking it on another selected engine.
- Spatial intensity is configured in AI Hub, not Audio Settings.
- Smart Listening master off or leaving APEX 10M stops its experimental spatial processor.

### Enable
Open **CYN → AI Hub → CYN APEX 10M · Experimental spatial** and select it. The AI Hub's 0–100 slider controls the intensity (default 40%). The setting persists locally.

The spatial processor operates after existing FX and DJ processing on 32-bit
float stereo PCM, with a high-pass, side-only ambience boost. It leaves the
center and mono sum unchanged unless the output limiter activates, limits
new spatial gain in anti-correlated material, and does not boost low-frequency
side effects. Mono and unsupported formats bypass the spatial stage.
Switching CYN power off disables this experimental stage.

### Critical technical limitations
- This is **not trained APEX 10M neural inference**. The 10M architecture
  exists separately; the additional 3.69M neural parameters remain untrained.
- No HRTF, 3D scene/object decoding, surround-channel routing or head tracking.
- Stereo enhancement can emphasize existing directions, but cannot reconstruct
  object positions from a stereo stream.
- This is a real-time deterministic DSP experiment, OFF by default. Sound
  improvements depend on material and listening setup; no quality guarantee.
- Does not remove or replace the underlying FxSound DSP/driver.
- Install only after exiting the current CYN/FxSound audio host. As with
  other CYN builds, only one virtual-driver host can be active.
- This experimental build is not marked latest or stable; rollback to the
  existing stable CYN release remains possible.

### Validation
Release CI compiles native spatial safety tests, builds CYN's Windows x64
program and installation package, and checks SHA-256 before publishing.
Testing includes exact disabled bypass, mono preservation, anti-phase
guard, stereo widening on an existing high-frequency side signal, and
unsupported-format safety. This is not a perceptual evaluation.

### Licensing and attribution
Original FxSound DSP code, virtual driver and relevant components remain
credited to FxSound LLC and original contributors. CYN modifications are
unofficial community work and are not endorsed by FxSound. Source and
redistribution notices continue under AGPL-3.0-or-later; warranties are
disclaimed under that license. No separate third-party neural weights were
added to this build.
