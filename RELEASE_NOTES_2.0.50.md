# CYN v2.0.50 — Activatable APEX 6M live neural-EQ preview

**This release integrates the genuine 6,307,477-parameter APEX ONNX model into the live CYN Windows audio processing workflow. No additional training has been performed.**

## Activate

1. Install CYN v2.0.50, exit any older Cyn from the tray, and open **Smart Listening → Music AI → AI Hub**.
2. Select **CYN APEX 6M (Full Neural Preview)**. The button now reads **Activate selected AI** once the full pretrained ONNX weights are installed.
3. Click **Activate selected AI**, then turn on Smart Listening and play music through the existing CYN/FxSound virtual device. Full APEX neural inference reads *real captured full-bandwidth 48 kHz mono/stereo audio* asynchronously and sends the resulting spectral mask statistics into CYN's native EQ control. The current preset's EQ is preserved as baseline; APEX corrections are capped at -1.35/+1.15 dB and follow the existing sensitivity control.
4. Check the Smart Listening AI summary for **APEX 6M / ... / Neural EQ ACTIVE** once enough audio has arrived. Switch back to My saved AI or another profile to return to the original model.

## Architecture and limits

- **Real full model**: uses the exact SHA-256-verified existing APEX checkpoint exported to ONNX with 6,307,477 parameters and the existing 225 synthetic denoising training steps.
- **No new training**: this release only adds inference and activation.
- **Asynchronous**: ONNX inference runs outside the live audio callback, retaining native CYN/FxSound DSP processing and the user's previous trained profile. No extra audio streaming buffer delay is introduced by this *EQ control* preview.
- **Actual limitation**: this first live preview transforms the model's frequency mask into **bounded eight-band EQ corrections** on real playback. It does **not** yet apply the full per-sample spectral-denoising mask to the live waveform; the standalone APEX WAV application still provides full spectral-mask audio reconstruction.
- The current 6M checkpoint was trained to denoise synthetic audio, not to understand musical emotion or perceptual mastering. Inference may not improve your music. No blinded listening study or hardware device matrix has been completed.
- Native APEX requires a **48 kHz, 32-bit float, mono/stereo** source format and the packed ONNX model. With other source formats, missing model, no audio or a stopped neural runtime it silently falls back toward the manual EQ baseline rather than inferring fake results.
- Presets, CYN AI Hub community models, Movie AI, and personal learning remain intact.

## Tests

GitHub validates Windows compilation, full pretrained model checksum, native ONNX API loading with the bundled runtime, real 1x513x189 inference and AI Hub activation selection. The other CYN audio and UI regression suites remain enforced. Listening and driver round-trip tests on personal Windows hardware are still pending.

Official model: https://github.com/oddfoxerr-real/Cyn/releases/tag/apex-6m-v1.0.0

No automatic checkpoint downloads on a customer's computer, no new user accounts, and no private audio uploads.
