# CYN Desktop native visual overhaul — full-width Riff workbench

This is an actual JUCE layout and painting change, not a version bump or a test-only release.

## What changed
- The Sound Studio Overview now gives the full upper workbench to the live 31/35/39-band equalizer instead of splitting it into a cramped EQ and an empty right-side spectrum box.
- The real spectrum/visualizer stays in the EQ heading region. The native effect rack and AI engine status sit in a compact lower row.
- A single selector deck replaces three redundant top-level cards. Dark Material workspaces use subtle, layered native colour gradients and quieter borders rather than stacking black panels.
- Native navigation buttons gain destination captions alongside the existing vector icons, preserving tooltips and keyboard focus.
- The live equalizer paints gain reference guides and a more readable 2.2 px response stroke; its actual sliders, mouse behavior and FxController DSP write paths remain unchanged.
- Effects controls now share the parent's panel surface and dynamically center their slider rows within the available height.
- Other CYN pages keep their functional bounds but inherit the toned-down material-surface treatment.
- Explicit source/layout regression expectations and 15 SVG geometry previews are updated to reflect the new actual geometry.

## Scope and non-goals
This is native JUCE code; no Electron, fake audio meters, changed DSP presets, modified AI models, or user preference reset. Retain the existing Riff MIT copyright notice and FxSound AGPL notices.

## Verification
CI must pass native MSVC Windows compilation, APEX/AUDONIC and EQ tests, geometry source checks, and motion regressions. Review the actual interface on Windows afterward because SVG geometry is not a true JUCE screenshot.

Release only after CI passes.