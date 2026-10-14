# Cyn 2.0.35 — Live Stage: a genuinely new Sound Studio layout

Completely changes the native JUCE Sound Studio composition instead of recoloring the same four dashboard cards.

- **Immersive radial stage:** a large, circular music visualization dominates the workspace. All 72 radial ticks and the smaller recent-audio strip derive from actual pre-effect 100-ms RMS samples, not generated/dummy music.
- **Asymmetric composition:** audio visualization is central, with a much narrower right-hand AI/EQ console and a single low-profile performance rack spanning the bottom. No stacked four equal-sized cards.
- **Live AI sidecar:** actual YAMNet music, percussion and speech confidences, and the actual crescendo rise metric. Idle states are truthful when no scene is available.
- **Five interactive audio rings** stay in the bottom rack, with real DSP setters and applied-versus-preset indicators. Manual keyboard controls and AI automation remain intact.
- **Full theme compatibility:** all 20 existing themes and the separate Crescendo theme remain in the native Theme Studio. Stage gradients, radial graphics, text and effect rings follow active theme colors and light/dark contrast.
- **Real full-band EQ:** right-side preview plots the true applied DSP equalizer. Edit full EQ still opens an accessible native full-width 5-to-31 band editor.
- **Smart Listening** remains directly accessible on the main stage, moved away from the bottom rail. Other pages and device/preset selectors remain intact.

**Scope:** this is a real Windows GUI layout change in native JUCE rather than a PNG or an embedded web mockup; the DSP itself is unchanged. The energy percentage represents a normalized input display scale, not LUFS or a measured acoustic loudness percentage. Continuous audio and human Windows visual/DPI testing remain recommended after CI compilation.
