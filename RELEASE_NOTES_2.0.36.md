# Cyn v2.0.36 — Restore the original Cyn interface

The recent Crescendo Studio / Live Stage interface experiments have been withdrawn at the project owner's request.

This release **restores the complete native Cyn interface exactly as it was in v2.0.31**, before the redesigned dashboards and effect knobs. This is a source-level restoration of the original JUCE UI, not a theme that only makes the newer layout resemble the old one.

- Restore the original Sound Studio layout, effect sliders, editable equalizer, sidebar/navigation and appearance.
- Restore the original 20-theme catalog (including Graphite's original palette), Theme Studio, and original theme animations.
- Remove the dedicated Crescendo/Live Stage graphical renderer, radial visualizations, bottom performance dock, and redesign-only source/test additions.
- **Keep** the v2.0.30 trained starter model, beat-aware bass, vocal clarity improvements, and v2.0.31 crescendo-aware music DSP.
- Keep Music AI, Movie AI, AI Hub, AI DJ, output routing, optional audio driver repairs, personal presets and user settings as implemented in v2.0.31.
- Retain the automatic updater; publish a new version rather than reusing an older number.

The withdrawn redesign releases v2.0.32 through v2.0.35 and their GitHub release tags will be deleted **only after** this restored release is published successfully. Git commit history remains intact: removing public release assets and release tags does not erase historical commits or external copies.

Build validation covers native Windows compilation, existing AI/driver/DSP regression checks, installer packaging and SHA-256 verification.
