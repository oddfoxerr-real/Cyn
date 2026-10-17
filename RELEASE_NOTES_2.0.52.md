# CYN v2.0.52 — DJ Touch Studio and 31/35/39-Band EQ

## DJ Touch Studio (independent from color themes)

Open **Settings → Theme Studio**. Beneath the familiar theme carousel, choose **AI TOUCH / DJ HAND** separately from the background palette:

- **None** — default, zero overlay drawing.
- **DJ Cursor** — translucent native cursor over real controls.
- **Animated DJ Hand** — stylized finger/hand reaches toward real knobs.
- **Ghost Finger** — translucent glowing fingertip silhouette.
- **Flower Touch** — six-petal touch indicator.
- **Custom PNG** — choose a local transparent PNG (at most 2 MB, 1024×1024). Cyn copies it to the user's private Cyn TouchAssets folder. Image selection never needs cloud hosting and is not uploaded.

**Apply studio** saves the theme and touch style independently. Reducing motion disables touch animations. CYN uses measured differences between applied and saved EQ/effect gains before moving toward bass, clarity, dynamics, or EQ controls. The transparent overlay receives no clicks and never writes back to audio, neural weights, sliders or DSP. When AI adjustments stop, the hand fades. When style is None there is no gesture rendering.

## Real graphic EQ choices

- **31 bands** (new default).
- **35 bands** (higher-resolution opt-in).
- **39 bands** (maximum supported; may consume more CPU than 31).
- The original FxSound DSP backend used 31 maximum SOS sections, so this release enlarges its section storage, allocates the full 40-section capacity before playback, and actually activates 31, 35 or 39 filter sections.
- Preset EQ gain remapping and one-based arrays are widened to handle 39 without overflow. Legacy 5/10/15/20 saved selections migrate to the 31-band default.
- Extra bands are NOT fake visuals. Source code tests verify backend allocation; Windows CI compiles the actual native DSP implementation.

## APEX protection

Same 6,307,477 ONNX checkpoint and existing neural music profile, no new training, and no changes to APEX's audio inference. The AI touch itself is purely cosmetic. Higher band counts add DSP work, so start with **31** on weaker PCs. Windows compilation, shipped weights and native ONNX inference are release gates.
