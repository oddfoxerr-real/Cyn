# Cyn v2.0.38 — Choose your actual AI model

The Music AI / Movie & TV AI dropdown selects a listening *mode*, not an installed model. Previously the AI Hub's Import action activated a model, but there was no way to choose among models later. This release fixes that.

- Adds a separate **Active AI Model** dropdown in Smart Listening. Choose **My saved AI**, **Cyn Starter**, **Cyn Cinema AI**, **Cyn Real Audio 13h (Experimental)** or a custom imported AI.
- The **AI Hub** button now reads **Activate selected AI**, confirms which listening mode will be affected, and provides explicit activation feedback.
- Switching saves the current model's weights and personal More/Less example history into **mode-specific local model slots**. Returning to a prior model restores its previous state, rather than forcing another import.
- The selected Music AI and Movie AI profiles remain independent and persist across restarts.
- Existing music/movie DSP, YAMNet scene analysis, output device routing, themes, presets and update features are unchanged.

No model was retrained in this release. The Real Audio 13h profile remains experimental, and no claim is made that it sounds better without listening validation. This release fixes profile selection rather than increasing automatic DSP intensity.