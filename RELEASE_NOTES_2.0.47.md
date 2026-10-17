# CYN v2.0.47 — First Activatable APEX Flow Hybrid (Experimental)

**This is a semi-working, opt-in live music-processing preview.**

Unlike the previous APEX-6M research download, **CYN APEX Flow Hybrid (Live Preview)** can be selected in the native AI Hub, and the new flow controller directly influences applied audio DSP.

## What is actually combined

1. **Pretrained YAMNet, already shipped with CYN**, detects musical scenes, rhythm-related audio events, percussion, bass drums and vocals. Its pre-trained weights remain in the included YAMNet ONNX model.
2. **CYN Real Audio 13h (Experimental)**, the previously trained 209-parameter Bass/Clarity preference model. APEX Flow Hybrid bundles this exact trained profile (not freshly simulated training).
3. **APEX Flow Hybrid**, a new time-aware C++ controller that compares music input energy over ~0.5–4.5 seconds and uses YAMNet evidence to distinguish gradual builds, percussive impacts and quieter breakdowns. It modifies live Clarity, Dynamics, Bass and low EQ goals only when the selected experimental profile is active.
4. The existing **CYN Smart Range**, smoothing, max sensitivity, headroom cap and real FXSound DSP continue to apply to final audio.

No audio is sent to a server. Neural inference remains on the existing asynchronous YAMNet worker; Flow Hybrid is light and runs on the JUCE control timer, not in the audio callback.

## What it is NOT

- It **does not** yet run the distinct 6,307,477-parameter APEX-6M PyTorch denoiser in native Windows audio.
- It is **not** a merged 30-model neural checkpoint or trained to understand music emotion.
- A measurable response to test ramps/drops is **not proof of superior sound** or comparison with paid commercial audio processors.
- The pretrained YAMNet class outputs update at scene/window cadence; this is not sample-accurate onset detection, source separation, beat prediction or mastering.

## How to use

Install CYN **v2.0.47 or later**, select **Music AI** in Smart Listening, enable Smart Listening, then open **AI Hub**, select **CYN APEX Flow Hybrid (Live Preview)** and choose **Activate selected AI**. Alternatively choose the model from the Music AI neural-model dropdown. Enable sufficient sensitivity to hear dynamic differences. Disable Smart Listening or switch back to a previous AI at any time. Existing preferences are backed up through the normal AI profile selection mechanism.

The AI Hub additionally retains **CYN APEX-6M FlowSense (Research)** as a **View research files** entry, with its offline checkpoint and synthetic benchmark.

## Reliability gates

The tests cover rising loudness, musical impacts, fallbacks for invalid/speech scenes, pause/stale resets and safe limits when input peaks approach full scale. GitHub Actions compiles both the pure C++ tracker and the actual Windows app and checks that the hybrid profile is included in the installer package.

**Known limitations:** fast music onsets can be delayed; input peak estimation is not output true-peak metering; no real human A/B test, licensed musical benchmark, or external hardware driver round-trip validation has been completed. Revert if the audio feels worse.
