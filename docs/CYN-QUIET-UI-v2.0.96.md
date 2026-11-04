# CYN Windows v2.0.96 — Quiet UI

CYN's Riff/Electron Windows interface is intentionally simpler without changing audio processing, model selection, native output routing, or the updater.

## What changed
- A flat charcoal design, with fewer gradients, tighter typography and consistent whitespace.
- Sound Studio shows Bass, Clarity and Surround first, with Ambience and Dynamic Boost available under More sound controls. The five native FxSound effect IDs, slider values and actual DSP command callbacks remain unchanged.
- The live native EQ curve and spectrum remain visible. Engine diagnostic actions collapse when processing is healthy and show automatically on routing faults or disconnection.
- Smart Listening prioritizes adaptive processing, the active native model and BrainScan. Learning feedback, headroom, AUDONIC and optional AI DJ remain available through a keyboard-accessible Advanced AI tools disclosure.
- Desktop/small-screen layouts use the same minimal presentation, improved focus outlines and reduced-motion rules.

## Verification
Riff native DSP, updater, device selection and model controls must remain connected to the original FxSound host. Quiet UI regression tests cover layout structure, engine diagnostic visibility, all five live DSP effect controls and keyboard disclosure sections. Actual Windows visual inspection and audio playback remain necessary for release acceptance.

## Distribution
Installer and native Windows build version are synchronized to 2.0.96. GitHub Windows release job should publish Cyn-Setup-2.0.96-x64.exe and Cyn-Windows-x64.zip only after successful tests and compilation. Existing CYN installation remains upgradeable; no new DSP engine or separate UI is added.
