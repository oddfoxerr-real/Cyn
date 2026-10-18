# CYN v2.0.60 — Button Studio & Liquid Interaction Polish

## New Theme Studio → Buttons
Pick any combination of five native button motion styles:
**Classic**, **Quick Pop**, **Soft Float**, **Ripple Press**, **Liquid Morph**.

Mix those independently with five highlight materials:
**Subtle**, **Neon Edge**, **Soft Glow**, **Liquid Edge**, **Glass Sweep**.

Adjust **button response speed** between **0.60× and 1.60×** and
**highlight intensity** between **0% and 100%**.
Use an interactive hover-and-click sample in Theme Studio to preview choices
before pressing **Apply studio**. Settings persist between app launches.
Existing theme colours remain independent of animation options.

## Close-button swallow improvements
The real X now expands to cover the whole window with a readable
ease-in/out, a brief full-window pause, then the chosen liquid effect.
At the default 0.85× speed: around **354ms expansion, 66ms full-cover hold,
404ms dissolution** (~824ms total). The previous expansion was about 150ms,
often too fast to distinguish from instantly hiding the window.
The old slider remains available for individual speed adjustment.

Additional close styles: **Stardust** and **Liquid Curtain**, complementing
Vortex, Gravity Melt, Liquid Ripple, Soft Fade and None.
Reduced Motion skips nonessential motion and decorative highlights.

## Performance and scope
All button effects use small fixed native JUCE paths/gradients tied to real
hover/pressed state. No perpetual liquid simulation or audio-thread rendering.
Changes to appearance do not modify ONNX models, APEX weights, DSP, audio
devices, presets or driver setup.

The Windows build and native regressions can confirm integration and compilation.
A hands-on Windows playback/UI test is still needed to verify real frame pacing,
overlay z-order, and the user's preferred visual feel.
