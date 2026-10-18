# CYN v2.0.70 — Liquid Screen Morph

- Replaces the rectangular-strip page wipe with a connected, wavy liquid mask and spray droplets revealing real destination page pixels DURING the switch.
- Main workspaces, Sound Studio focus tabs and theme changes use the same screen-compositing transition instead of stacked slide overlays.
- Captures the outgoing native JUCE view before switching and the incoming view after immediately updating the actual controls; cuts new pixels into the old image as the liquid frontier advances.
- Deterministic aerosol droplets and subtle accent edges, with 405 ms page transitions and 450 ms theme transitions.
- Preserves DSP, AI models, presets, themes, output routing and reduced-motion setting.
- Native visual verification is still recommended after the automated Windows build.
