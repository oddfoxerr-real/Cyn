# Cyn 2.0.18 — limit bass overreaction

The learned preference is not a bass detector. A highly confident model could keep
asking for bass during sections without strong low-frequency energy. The timeline
also displayed overall RMS loudness, making loud vocals look visually bass-heavy.

Windows Smart listening now applies a separate spectral control limit to the bass
proposal. It compares the existing pre-effect 45/75 Hz measurements with the
120/200/400 Hz measurements, independently of stereo width and level. As relative
low-bass activity falls, the controller reduces bass enhancement and positive low EQ
boosts, even if the raw model still requests more. Reduction is faster than increase.

The learned 11→12→5 network, weights, saved feedback and other effects remain intact.
This limit is ordinary control logic around the network, not a newly trained model
or vocal/instrument classifier. It runs only for a trained bass head with Smart
listening enabled; zero sensitivity retains the manual baseline, and switching Smart
listening off restores the preset. The limit is intentionally conservative for upper
bass: a 120 Hz fundamental can belong to either a voice or an instrument. It cannot
separate those sources, and deep vocals may still contain substantial low bass.

The status row reports the applied bass/clarity settings and a low/moderate/strong
low-bass activity index. Raw model proposals remain visible in the neural panel and
CLI (`raw_bass_preference`); `bass_preference` remains the smoothed control value.
The timeline explicitly labels its bars as loudness, not bass.

## Validation

C++ tests pass for quiet/loud bass, phase-inverted stereo bass, loud and quiet
200/400/1000/2200 Hz signals, a 120 Hz harmonic voice-like signal, silence, invalid
input, existing learning tests and release/attack response. These are synthetic
signal tests, not listening tests on the user's song or a real-vocal benchmark.
No audio-driver or DSP engine source was changed; this patch changes the UI-thread
controller that supplies bounded effect settings.

Windows x64 installer: `Cyn-Setup-2.0.18-x64.exe`. Android remains at 0.1.1-preview;
its different FFT feature definition needs separate tuning, so these Windows
thresholds are not copied into Android. Public Windows binary publication remains
subject to the licensing review documented in `licenses/release-review.json`.
