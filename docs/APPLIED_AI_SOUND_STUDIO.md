# Sound Studio: applied-AI visual telemetry

Cyn Smart Listening uses its own baseline preset plus transient AI changes to the live DSP coefficients. Previously Sound Studio sampled the saved baseline through `FxController::getEffectValue` and `getEqBandBoostCut`, so EQ nodes and enhancement sliders stayed fixed even while the sound changed.

## Behavior

- **Sound Studio reads the real DSP-applied values** with `getAppliedEffectValue()` and `getAppliedEqBandBoostCut()` rather than the stored baseline.
- EQ points, connecting curve, fill, and the five enhancement sliders **move only when those applied values really change**. The follower eases toward the real target on the JUCE message thread and cannot overshoot it.
- **No fabricated audio-energy bobbing, sinusoids, or idle pulse.** If an AI effect or EQ band is unchanged, that control is motionless. Other processing (e.g. leveler) is summarized in the bottom strip rather than falsely moving an unrelated EQ band.
- The bottom **Music AI / Movie AI** status strip displays the actual applied differences from the saved baseline (up to two effect deltas, number of changed EQ bands, or leveling). The Smart Listening page retains its model/scene summary.
- **Manual controls are preserved.** Visual sync uses JUCE `dontSendNotification` and never writes the DSP, modifies the stored baseline, or marks a preset modified. While a user drags a slider, Cyn stops following that control until the interaction ends. The legacy Alt-drag multi-band EQ function also takes priority.
- When Smart Listening is turned **off**, GUI positions are synced directly to the restored DSP values. **Reduce motion** also snaps to the actual applied value without a transition.
- The view samples the DSP on its existing UI timer at approximately **30 FPS** while Sound Studio is visible; no new background audio thread, recorder, or high-frequency timer is introduced.

## Verification

- `node scripts/test_ai_applied_ui.mjs`: source safety/telemetry checks, keeping baseline separate from applied coefficients.
- `tests/cyn_dsp_ui_follow_test.cpp`: a standalone native C++ test of target following, steady values, monotonic upward/downward motion, reduced-motion snapping and NaN rejection.
- Native Windows build runs on PR CI.
- Hardware A/B: play music/video; enable Smart Listening; verify sliders/EQ visibly track changes without saving the AI corrections into a preset, then turn AI off and verify the controls return to preset values. Try manually dragging an effect and EQ band and confirm the audio edits work without a jumping cursor. Verify reduced motion and no-audio conditions.

The UI follower is visual only: it does **not** add gain or apply any new DSP coefficient to the audio.
