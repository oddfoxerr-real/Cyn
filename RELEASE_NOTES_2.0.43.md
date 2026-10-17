# Cyn v2.0.43 — Full-range Music AI sensitivity

You asked why 100% sensitivity still made small adjustments. The previous music controller multiplied its model scores by small fixed positive and negative ranges (e.g. Ambience/Surround +1.4), then smoothed the changes. Consequently, 100% was *maximum of those small margins*, not maximum access to the existing DSP.

## New behavior
- **0%** keeps the manual preset unchanged.
- **0–55%** maintains Cyn's previous, restrained Music AI response.
- **55–100%** gradually increases Music AI control over the real effect range.
- **100%** allows scene/model-driven settings to reach the existing **0–10 DSP range** when the model's preference is strong enough. It does *not* mean every effect stays at 10.
- Music AI changes respond faster at high sensitivity, and temporary signed EQ changes can be more pronounced while preserving your manual baseline.
- The neural model, YAMNet worker, trained profiles, and track recognition remain unchanged.

## Important safety and boundaries
- Additional positive Clarity (above 55%), Bass, and Dynamic Boost changes remain scaled by **measured pre-effect headroom**. This is an estimate, not a proof of output true-peak safety.
- Automatic upper-treble EQ boost is capped to reduce sibilance; all effect and EQ values remain within the DSP's existing legal ranges.
- **Movie & TV AI stays restrained**, preserving its dialogue-specific bass/dynamic limits.
- Changes are temporary: stopping Smart Listening, switching modes, or saving a preset still restores the manual baseline. No automatic system-volume/master-gain changes.

## Verification
The Windows release pipeline now runs a native C++ regression testing full-range authority, low-sensitivity continuity, neutral preference, movie-mode limits, finite inputs, headroom restrictions, and smooth bounded rates. UI regression tests also confirm that the actual DSP—not a visual animation—is being driven by the new sensitivity mapping.

This is an experimental DSP responsiveness improvement: subjective sound quality, clipping behavior on particular devices, and listening fatigue still require testing on headphones at a sensible starting volume.
