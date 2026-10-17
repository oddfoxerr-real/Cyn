# Cyn v2.0.45 — Optional Auto Preset Cycling

Music AI can now **morph between real installed preset settings in real time**, based on the type of music in the current passage. Enable this in **Smart Listening → Auto preset cycling**.

## What it does
- When enabled, Cyn reads the actual effect levels and equalizer curves from its existing built-in `.fac` presets. It also preserves the user's currently selected manual preset as the starting profile.
- Using the existing YAMNet scene (bass drum, percussion, vocals, soundtrack, electronic music, etc.), a lightweight selection policy compares actual preset signatures and picks a suitable profile.
- The selected profile is displayed beside the switch. Actual Bass, Clarity, Dynamics, Surround and EQ DSP values **smoothly morph** toward that profile while the selected neural model and Smart Listening continue processing normally.
- Stable candidate selection (~700 ms), minimum hold time (~1.8 s), and fade speeds protect against frantic switching and audio discontinuities. A single isolated note will not immediately trigger a hard preset reload.
- The app's visible selected preset, saved preset files, manual modifications, and auto-save system are **not changed** by cycling. Turning cycling off restores the original manual baseline.
- Preset files are read only on initial enable / baseline changes, not continuously during audio processing. No second DSP instance or disk-read on each beat is required.
- **Off by default.** Only active for Music AI; Movie/TV AI retains its dialogue behavior.

## Important limits
- This uses the available real-time scene/features to make an **estimated best fit** for musical passages. It cannot know the objectively perfect preset for an individual note, and scene detection has latency.
- It automatically considers built-in presets plus your currently selected manual preset, not every user-created preset, to avoid unexpected user-preset switches.
- Increases to Bass, Dynamic Boost, Clarity or EQ gains are constrained by the existing pre-effect headroom and extreme-input-peak safeguards. These do not guarantee output true-peak safety.
- Smooth changes are not sample-perfect crossfades; subjective listening and device testing are recommended.

## Preserved
Cyn's neural profiles, training history, Dark Theme, AI Hub, headroom cap, max sensitivity behavior, existing bass/kick tuning, updater and version checker remain intact.
