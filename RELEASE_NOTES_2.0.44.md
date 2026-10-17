# Cyn v2.0.44 — Music AI Headroom Cap + more expressive neural control

Music AI's previous 100% sensitivity setting was still subdued, because neural preferences often clustered near 0.5 and the pre-effect headroom estimator scaled even strong bass/dynamics requests down.

## New control: Headroom Cap
Open **Smart Listening**. Alongside **Sensitivity**, use the new **Headroom Cap** slider (0–100%).

- **0%**: extra-conservative use of pre-effect headroom.
- **50% (default)**: previous headroom envelope, preserving existing listening settings.
- **100%**: maximum permitted headroom use for Music AI. It can request stronger effects than the previous conservative cap, particularly for bass-heavy music, but **cannot override the independent very-high-input-peak gate**.
- The setting is saved and restored per user. Movie/TV AI retains its existing conservative dialogue-focused controls, and the Headroom Cap slider is disabled in Movie mode.

## More expressive model decisions
- Music neural scores are progressively expanded *away* from the neutral 0.5 midpoint when Sensitivity exceeds 55%; weaker ambiguous model scores remain neutral, and 100% allows more contrast.
- This controls **real DSP effect values**, not the neural animation. The existing 0–10 effect ranges, headroom-aware EQ and temporary manual-preset baseline all stay intact.
- With both Sensitivity and Headroom Cap at 100%, Music AI has significantly more room to react to drums, bass, transients and crescendos.

## Safety / limitations
A high Headroom Cap is deliberately more permissive, **not a guarantee against clipping**. This is based on measured *pre-effect* peak/RMS, not output true peak. Avoid running extremely high volume during testing; some effects, headphones and source material can still distort. The near-clipping input gate is always active, even at 100%. We do not automatically modify system volume or master gain.

## Preserved
- Bass-preset kick tuning and real audio/YAMNet AI profile selection.
- Dark Theme's black glass, soft blue gradients and liquid web.
- Version control, update installer, and independent Movie AI behavior.
- All existing AI labels, weights, imported profiles and training data.

The Windows pipeline includes a native test for headroom slider behavior, more expressive neural decisions, peak gating, backwards compatibility and Movie mode, plus UI wiring and actual DSP regression checks.
