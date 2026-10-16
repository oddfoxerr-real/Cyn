# Cyn v2.0.39 — Kick/punch restoration, volume-aware safety and updater repairs

This update addresses feedback that the new AI reacted to volume changes, that the Bass preset lost the punch present in earlier builds, and that update prompts repeatedly offered the installed version.

## Audio
- Feed **level-neutral spectral features** into Cyn's learned music preference model and balance-role recognizer. Lowering the playback level no longer changes those feature inputs; the raw waveform level remains available for **silence and clipping safeguards**.
- Use a musical kick-shaped EQ accent centered near **95 Hz**, derived from percussion/bass-drum confidence, across **General and Bass** presets. This preserves existing sub-bass instead of replacing a punchy kick with rumble.
- Keep the user's manually boosted low-frequency preset bands rather than automatically cutting them merely because the song has strong bass, except when the source has very little headroom.
- Make music bass and dynamic effects more responsive within their existing 0–10 control limits. The user's Sensitivity control remains authoritative. Do not auto-adjust system volume or master gain.
- Add native regression tests for lowered-volume feature equivalence, transient/bass EQ behavior and peak-risk protection. The increase is deliberately limited to avoid risky unverified output amplification.

## Update reliability
- Fix **the actual native app version** and updater mismatch: previous build metadata still identified Cyn as **2.0.29**. All Windows build configurations now identify **2.0.39** and the updater passes the running version dynamically.
- Add a single-instance updater mutex so automatic checks and manual checks cannot stack up duplicate download windows and workers.
- Verify app, installer and updater version contracts in CI.

## Limitations
A sudden upstream volume change can still alter the raw peak measurement and the short-term crescendo detector. Exact loudness-normalized music perception across all playback environments needs further listening/driver testing; YAMNet's pretrained classifier is not volume-agnostic at arbitrarily quiet or clipped input. Improved kick presence is still user- and headphone-dependent and should be confirmed on Bass/General presets at safe volume.
