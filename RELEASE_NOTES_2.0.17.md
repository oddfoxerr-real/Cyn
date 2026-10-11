# Cyn 2.0.17 — Dedicated Movie & TV AI and balanced Music AI

This release introduces a **separate, locally trainable Movie & TV AI** alongside Music AI. Choose your mode in Smart Listening; the selected mode drives audio adjustments, feedback labels, model training, and live neural visualization.

- **Independent Movie and Music models**: weights, readiness flags, and personal labels are persisted separately in Cyn/preferences.json; upgrading preserves all existing Music AI preferences.
- **Movie & TV starter**: uses the bundled separately trained Cyn Cinema AI weights for Clarity and Bass (on synthetic movie-like mixtures); other heads remain untrained. If the bundled Cinema profile is missing, Cyn falls back to the published generic acoustic weights with movie heads disabled and uses a conservative midband-presence estimate. Neither model understands spoken words or separates dialogue from sound effects.
- **Fix runaway Music AI bass**: Music AI's bass adjustment is no longer favored above clarity and automatic boosts have a 6.5/10 ceiling relative to the manual setting. Movie AI uses a 4.8/10 automatic ceiling relative to the manual setting. The app does not override user-selected high manual bass settings.
- **Separate feedback/training**: More/Less and Train now act only on the selected mode, with training epoch progress.
- Switching modes restores your chosen preset baseline before applying a different model; training locks the mode selector until completion.
- **Starter reset** acts only on the selected mode. AI profile import/export affects only the selected mode.
- **Quality gate for starter promotion**: An independent comparison script requires verified human-labeled held-out movie *and* music data, and blocks replacing the shipped global starter unless the new model improves both.

**Limitations:** There is no dataset of genuinely annotated film dialogue in this release and no evidence yet of improved real-world dialogue intelligibility. The new mode provides independent learning and safer sound-shaping foundations. You can train it by labeling audio from movies and episodes.
