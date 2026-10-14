# Cyn 2.0.34 — Crescendo Theme Studio visibility fix

This release includes all fixes from v2.0.33 and corrects the final native JUCE rotary indicator rendering:

- The #5 Crescendo Studio design respects the currently selected theme throughout the interface. All 20 original themes are preserved, including the restored original Graphite. A separate 21st optional Crescendo theme is available in Theme Studio.
- The sidebar Themes entry opens the existing full Theme Studio, including preview carousel and persistent Apply theme.
- The compact dashboard EQ is a real applied-DSP read-only curve. Edit full EQ opens the full-width interactive editor for saved 5–31 band presets.
- Corrected the opacity of the native five rotary DSP rings, outlines and percentages. JUCE interprets palette uint32 RGB values without a full alpha byte; the rendered controls now explicitly set alpha=1.0 before painting.
- No changes to the audio engine, training, device management, AI DJ or automatic driver-repair permissions.

**Validation:** GitHub Actions executes theme and UI source tests, the independent applied-DSP regression, native MSVC compile, installer generation and SHA-256 verification. The screenshot/layout still warrants a human Windows visual review across light and dark themes and high DPI displays.
