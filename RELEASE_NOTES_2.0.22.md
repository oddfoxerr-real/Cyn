# Cyn 2.0.22 - Optional AI DJ Experience

**Misc → Download AI DJ** installs a separate 91 MB pretrained audio model inside
Cyn, with progress, cancel/retry, checksum verification and removal. The main
installer contains no AI DJ model weights. No browser, account or Python install
is needed.

Enable AI DJ to add occasional stereo sweeps, echo accents and shimmers to music.
Adjust intensity, minimum spacing and left/right movement. Original stereo audio
keeps its position. Music AI and Movie AI remain independent.

The optional model is the 86.6M-parameter Audio Spectrogram Transformer trained
on AudioSet, running locally through ONNX Runtime. Background scene recognition
gates a realtime onset/effects engine; it is not a trained generative DJ.

AI DJ starts off, unloads its model when disabled, expires stale scene decisions,
suppresses accents when speech confidence is high, and bypasses unsupported audio
formats. **Timing and scene recognition remain experimental**, especially for
games/dialogue. Initial recognition needs eight seconds of audio; sudden scene
changes can take time to recognise.

See `docs/AI-DJ.md` and `docs/AI-DJ-VALIDATION.md` for storage, model provenance,
checks, and the limits of validation. The Windows app/installer are unsigned
community builds; the existing signed FxSound driver is preserved.
