# Cyn 2.0.30 — Beat-focused Music AI and vocal clarity

The built-in Cyn Starter and AI Hub Cyn Starter profile now use the same refreshed 209-parameter neural weights. A reproducible 2,500-step optimizer trained on 2,500 procedural spectral mixtures, selected a checkpoint on 500 separate mixtures, and evaluated once on 500 differently seeded held-out mixtures.

- Music AI uses YAMNet's real percussion and bass-drum confidence to reinforce kick-driven low end, with spectral fullness and measured pre-effects peak headroom constraining boosts.
- Vocals gain modest 2.2 kHz presence when the live music scene indicates voice, but brightness/sibilance controls suppress unwanted high-frequency sharpening.
- New synthetic preference targets favor rhythmic low end over continuous rumble instead of driving Bass to maximum continuously.
- The compiled Cyn Starter and importable AI Hub profile carry identical weights and regenerated synthetic starter labels. Existing personal preferences are preserved. To adopt the new base manually, choose Smart listening > Reset to starter.
- Native regression tests check kick confidence, vocal presence, sibilant music and overload behavior. The new evaluation report is models/starter-evaluation-v3.json and the training recipe is training/train_starter_v3.mjs.

**Scope / limitations:** All training targets are simulated soft preference heuristics, not independently rated music. This is not vocal source separation, accurate BPM tracking or a validated increase in real-world sound quality. Pre-effects headroom estimation is not a true-peak limiter. Real listening tests, device checks and post-effects clipping measurement remain necessary.
