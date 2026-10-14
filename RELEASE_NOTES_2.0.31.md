# Cyn 2.0.31 — Real Musical Crescendo Response

Music AI now follows measured quiet-to-loud energy rather than treating each moment independently.

- Compares incoming pre-effect RMS over approximately 600 milliseconds with the preceding 1.1–5.5 seconds to identify measured builds and drops.
- Preserves soft intros: when YAMNet confidently identifies music, Cyn no longer adds its own automatic quiet-passage volume leveling. The listener's saved leveling setting is preserved.
- Adds modest bass and dynamics emphasis at a detected musical rise, weighted by real YAMNet percussion and bass-drum evidence and guarded by remaining pre-effects headroom.
- No constant boost on sustained loud music. Silence, speech, stale input, device pauses and mode/preset changes reset the energy arc.
- Movie AI, manual presets, personal training and AI DJ remain independent.

This is an amplitude-history enhancement informed by existing scene AI, not predictive beat tracking, source separation or a newly trained neural model. The release adds a native regression test, run before compiling the installer. Verify with real tracks and check output peaks: pre-effects headroom is not true-peak protection.
