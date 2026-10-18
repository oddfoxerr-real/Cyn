# CYN v2.0.56 — Vortex Motion Polish

This focused native UI release is guided by the responsive visual feel of the DroidDeck reference, while keeping CYN's own design, audio architecture and themes.

## Close-to-tray: the correct direction
- Clicking the window **X** starts at the **actual button bounds**, then the button expands to cover the entire CYN window (~129 ms).
- The resulting full-window surface holds for ~41 ms, then liquefies into a flowing vortex until it disappears (~290 ms).
- The original app stays visually present during expansion, then stops rendering once the button has covered it, avoiding a reappearing-window flash during the dissolve.
- The vortex appearance is inspired by the offline PR #42 simulations, with a bounded **192 × 108 CPU mask** to avoid a heavyweight live fluid solver. The experimental fluid frame pack is **not** bundled.
- Closing to tray never turns off, interrupts or modifies the underlying audio processor. Reopening cancels the overlay; Reduce Motion bypasses it.

## Responsive controls
- Buttons, hover states and toggle motion now use elapsed-time-aware interpolation rather than fixed per-frame steps, helping maintain similar responsiveness on slower PCs.

## Quality notes
- The CYN layout, AI inference/training, model weights, existing themes, 31/35/39-band EQ, DSP processing and device-management features are unchanged.
- Native compiler and regression checks verify the implementation, but subjective animation quality and lower-end GPU/CPU frame times still require a hands-on Windows visual review.
