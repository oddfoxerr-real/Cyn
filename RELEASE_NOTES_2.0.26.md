# Cyn 2.0.26 — Live AI Sound Studio Controls

This release makes the Sound Studio equalizer and enhancement sliders faithfully reflect the adjustments the Music / Movie AI is **actually applying** to the audio signal.

## Real AI-applied display

- **EQ curve and band points** now glide toward the live applied DSP filter gains rather than remaining fixed at the original preset while Smart Listening is on.
- **Clarity, Ambience, Surround Sound, Dynamic Boost, and Bass Boost** sliders reflect their actual applied DSP levels with smooth interpolation on the UI thread.
- **No fake audio motion:** unchanged EQ bands and effects remain still. Animations are not random, sine-based, or tied to an unrelated bass pulse.
- The bottom Sound Studio status strip shows the selected AI mode and meaningful real differences from the preset: changed effect levels, number of affected EQ bands, or audio leveling.
- **Manual edits remain possible** and take priority over automatic visual following during a drag. Existing keyboard ±1 steps and Alt-drag multi-band EQ control remain intact.
- AI-driven display updates **do not change actual DSP settings or save temporary AI adjustments into a preset**; they only visualize the already applied changes.
- Turning Smart Listening off returns controls to the restored preset coefficients. **Reduce motion** snaps to the real DSP levels without animated sliding.
- The native UI refreshes a maximum of about **30 frames per second** while Sound Studio is visible using its existing timer. No extra background audio recorder or AI thread is added.

## Quality and safeguards

- Windows native builds, updater and autonomous balance validation, and native DSP display-follower checks passed for the pull request before release.
- A visual follow never overshoots the actual DSP value and remains still when the actual value is unchanged.
- This change does not modify the AI's sound decisions, audio pipeline, or output device/repair permissions.
- Actual appearance on individual devices has not been user-tested; CI validates compilation and expected logic.

## Downloads

- Windows x64 installer: **Cyn-Setup-2.0.26-x64.exe**.
- Portable build: **Cyn-Windows-x64.zip**.
- Installer SHA-256 checksum included.
- Builds are currently unsigned and Windows may show a security prompt.

Licensed under GNU AGPL-3.0-or-later.
