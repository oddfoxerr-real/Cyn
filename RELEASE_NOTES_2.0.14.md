# Cyn 2.0.14 — Neural Activity and All-Audio Support

Cyn now includes the animated neural-activity display on Smart Listening, with gold neuron pulses in training mode, gentler idle activity, and a short completion highlight. The graph illustrates the real 11-input, 12-hidden, 5-output architecture. Pulses illustrate training state and **are not live individual neuron readouts**.

Cyn can already extract and learn from audio played through its selected Windows output. On-screen wording now explicitly welcomes **movies, TV episodes, music, podcasts, dialogue, and games**, using the existing More/Less feedback and manual controls.

- Neural activity runs at a bounded frame rate and respects reduced-motion settings.
- Layout places the graph in the unused Smart Listening sidebar, not over its learning controls.
- Builds on Cyn 2.0.13's opaque AI buttons and rounded tooltips.
- Does not claim to classify film versus music, understand dialogue, identify tracks, or introduce independently trained dialogue-specific model heads.
- Does **not** include the ongoing hour-trained neural base weights. Those must pass validation and will ship in the next release, not be silently combined with this UI release.

## Compatibility
Personal models and labels are unchanged, and the updater continues to use GitHub Releases and installer checksums.
