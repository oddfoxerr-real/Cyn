# CYN v2.0.48 — Publish full APEX 6M in AI Hub

## Published through the native **Share my AI → Publish AI** issue route

- Official [Publish AI submission #32](https://github.com/oddfoxerr-real/Cyn/issues/32).
- The AI Hub research entry now displays **CYN APEX 6M (Full Neural Preview)** and opens the [official full-model Windows release](https://github.com/oddfoxerr-real/Cyn/releases/tag/apex-6m-v1.0.0).
- Updated the native allowlist so the verified `apex-6m-v1.0.0` tag is accepted rather than silently filtered out, preserving the original research URL prefix for compatibility.
- Updated automated regression tests and bundled catalogue.

## Safety and compatibility

- The full APEX release really contains **6,307,477 ONNX neural parameters** and a local WAV processor.
- The **live Cyn AI Hub 209-parameter importer still cannot activate that ONNX model**. The APEX listing deliberately uses **View research files**; the executable must be downloaded and launched manually for testing.
- No new training, model downloads or background installations happen when AI Hub refreshes.
- The existing CYN APEX Flow Hybrid (Live Preview), personal AI profiles, themes, presets, drivers and current playback remain unchanged.

### Test
Install v2.0.48, open **Smart Listening → AI Hub**, choose **Refresh community**, select **CYN APEX 6M (Full Neural Preview)**, then choose **View research files** to open the full-model download. Do not click **Activate selected AI** for the full ONNX model, because it is not supported as a standard AI Hub profile.
