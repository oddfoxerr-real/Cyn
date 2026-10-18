# CYN v2.0.53 — Two DJ Hands and Independent Flower Touches

## AI Touch Studio: two independent panel performers

CYN v2.0.52 used only one shared pointer/hand target. That meant the strongest single change on the screen monopolized all AI gesture feedback. This update gives the **Effects** and **Equalizer** panels their own measured, moving gesture layer.

Open **Settings → Theme Studio → AI Touch / DJ Performers**:

- Choose **Effects** touch style independently: None, DJ Cursor, Animated DJ Hand, Ghost Finger, Flower Touch, or Custom PNG.
- Choose **EQ panel** touch style independently using the same options.
- When using hands in both panels, **two distinct DJ hands** work simultaneously; the EQ hand is mirrored to approach from the opposite side.
- The **Flowers** option supports **2, 3, or 4 independently aimed flowers per panel** (up to eight active blossoms across the two panels). Each flower targets a DIFFERENT effect knob or EQ band with an actual measured DSP adjustment. If fewer independent controls are moving, only those flowers appear.
- The same selection can produce **two independent cursors** (one per panel). Each panel may instead have its own style (e.g., hand for effects, flowers for EQ).
- The saved styles are separate from the selected background theme. Older single-touch selections automatically become the starting style for both panels.
- Custom PNG remains local, size-limited and follows independent per-panel coordinates. An imported PNG can be used on either/both panels.
- Reduced motion, None, absent audio processing and leaving Sound Studio disable this cosmetic overlay.
- No hand, cursor or flower intercepts mouse input, runs on the audio thread, issues neural inference, or changes the underlying DSP settings.

## Stability / sound preservation

- The real 31/35/39-band graphic EQ implementation and 31-band default remain unchanged.
- The APEX 6M ONNX weights, inference cadence, effect/headroom logic, profile backups, sound signature and preset cycling remain unchanged.
- This release only changes cosmetic UI and its saved appearance settings.
- All CYN x64 and ONNX runtime verification checks remain release gates.

### Verification

Select two Animated DJ Hands, play a track with Smart Listening on, and observe separate hands in the Effects and EQ panels. Then select Flowers for both panels, choose four flowers per panel, and watch independently moving EQ bands/effect knobs. Unrelated or unchanged controls must not generate looping animations.
