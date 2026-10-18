# CYN v2.0.57 — Animation Styles & Speed Controls

Theme Studio now has an **Animations** tab, separate from Appearance. Users can personalize the close-to-tray motion independently of CYN's colour theme and AI Touch options.

## Motion controls
- Five selectable visual styles: **Vortex**, **Gravity Melt**, **Liquid Ripple**, **Soft Fade** and **None (instant)**.
- A continuous **speed slider** from **0.55×** (slow) to **1.60×** (fast), in 0.05× increments.
- Default is **0.85×**, ~541 ms total, intentionally a little slower than v2.0.56's ~460 ms.
- **Replay** gives a lightweight in-studio animation preview; the preview and real close overlay share the same normalized fluid masks.
- **Apply studio** saves the chosen style and speed in persistent local settings; **Close** discards uncommitted changes.
- Close with X still hides to system tray; CYN remains running and audio processing is uninterrupted. **Reduce Motion** or **None** bypasses the overlay.

## Performance and compatibility
- Native JUCE effect uses the existing bounded 192×108 alpha field. No expensive fluid simulation, background rendering, model changes or new DSP code.
- Existing themes, AI Touch, 31/35/39 EQ, updater and audio remain unchanged. Normal button/hover responsiveness stays fast.
- Native CI verifies source integration and Windows compilation; real-world frame pacing and visual feel still warrant manual verification.
