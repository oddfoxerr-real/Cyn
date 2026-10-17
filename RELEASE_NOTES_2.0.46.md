# Cyn v2.0.46 — APEX AI Hub Visibility Fix

## Fixed
- AI Hub now displays **CYN APEX-6M FlowSense (Research)** after a community refresh, alongside Cyn Starter, Cyn Cinema AI and Cyn Real Audio 13h.
- Fixed a C++ escape-sequence error in the research URL validation. The prior validator accidentally treated ordinary letters such as `t`, `r` and `n` as forbidden, silently removing valid research entries.
- Fixed literal backslash-n sequences in the model description panel; text now renders as proper multiline paragraphs.
- Added an automated regression test verifying the exact released APEX URL survives native validation.

## How to find APEX
1. Install Cyn v2.0.46 or later, then open **Smart listening → AI Hub**.
2. Click **Refresh community** (the new catalogue is also included with this installer).
3. Select **CYN APEX-6M FlowSense (Research)**.
4. Click **View research files** to open the downloadable experimental neural research checkpoint and demonstrations.

## Technical limitation
APEX is a separate 6,307,477-parameter PyTorch offline denoiser and FlowSense analyzer. It does **not** run inside CYN's existing 209-parameter Smart Listening model importer, and this patch does not install a Windows live audio inference backend. We intentionally never label the button **Activate selected AI** for APEX.

## Preserved
Cyn's active neural profiles, personal learning, current presets, optional auto preset cycling, native audio driver, themes, update controls, and AI DJ behavior remain unchanged.
