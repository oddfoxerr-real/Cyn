# Cyn 2.0.33 — Full Theme Studio support and readable Crescendo UI

Restores the full native theme system to the Crescendo Studio redesign and fixes the cramped EQ visible in v2.0.32.

- **Preserves all 20 original themes** with the same saved IDs. Restores the original Graphite palette that v2.0.32 accidentally replaced.
- Adds **Crescendo** as a separate, 21st optional theme using the #5 concept's muted midnight/violet color scheme and its own atmosphere.
- Adds a permanent **Themes** sidebar button that opens the existing native Theme Studio, including previews, previous/next arrows, Apply/Close, theme transitions, and persisted selections.
- Every Crescendo Studio card, label, waveform, activity meter and five rotary DSP controls now derives its colors from the active theme rather than forcing purple. Light themes retain their legible foreground/background colors.
- Replaces the compressed 5/10/15/20/31-band EQ controls on the small dashboard with a **read-only curve of actual applied DSP EQ**. **Edit full EQ** opens the existing editable EQ over the full-width workspace. Its back button returns to the studio. No preset value or band count is changed.
- Corrects the overpaint/overlap at the enhancement panel title and keeps the bottom Smart Listening control separate from the status text.
- Existing Music AI, Movie AI, Smart Listening training, AI Hub, AI DJ, driver repair, output selection, device pins, EQ shortcuts, and lower-power reduced-motion settings remain in place.

**Limitations:** This is a native JUCE patch, not a pixel-perfect implementation of a 1600 × 960 browser design. CI validates source wiring, DSP regressions, native compilation and installer checks. A manual installed-Windows visual pass is still needed, especially when testing multiple Windows DPI scale factors and all 21 theme palettes.
