# Cyn 2.0.16 — Live Neural AI Visualizer

Smart Listening's brain display is **no longer a static decorative animation**. It follows the active personal AI model's real forward-pass activations and the latest valid audio features.

- Real **11 input, 12 hidden, 5 output** neuron values determine node brightness.
- Lines brighten according to the **actual trained weights multiplied by their source neuron activation**; traveling particles appear on active signal paths.
- Responds to changing audio (music, TV, movies, games, podcasts or dialogue) whether or not auto adjustment is enabled, as long as Cyn is capturing audio.
- Stops activity and displays **WAITING FOR AUDIO** when no recent audio is available.
- The **Train my model** operation displays progress from **actual completed optimization epochs** (240) rather than a fabricated countdown.
- Smoothed 24 FPS component refresh with UI-side snapshots at 10 Hz and reduced-motion support; no work is added to the real-time audio callback.
- Existing personal learning, profiles, updater logic, themes and model weights remain compatible.

**Note:** The visualization displays neural inference and supervised learning progress, not song comprehension, live hardware neuron signals, or a quality score. It cannot display weight changes for the earlier cloud training run; it reflects local learning while the user runs it.
