# CYN v2.0.62 — Native Dark UI Polish

The Horizon × Depth redesign in v2.0.61 introduced visible UI layout regressions on smaller Windows screens. This update fixes the real JUCE screens, without changing audio processing.

## UI corrections
- Dropdown labels (active profile, output device, AI enhancement) now have their **own line** above the actual selectors, eliminating clipped/overlapping text.
- The workspace tab bar sits below the selector cards, rather than passing through the card labels.
- Sound Studio, Smart Listening, Devices, AI Hub and AI DJ use corrected vertical spacing for the toolbar, card headings and working controls.
- Real 31/35/39-band EQ slider heights adapt to the smaller studio panels instead of clipping the bottom of the bands and frequency labels.
- AI DJ now separates its real enable/download controls from a second factual, live engine-state panel, and aligns intensity, spacing and stereo settings.
- The existing native title bar retains CYN branding; the sidebar instead identifies **Workspaces**, avoiding duplicated CYN branding.
- Increased muted-text contrast, corrected status indicators and replaced a UTF-8 separator that showed as scrambled text on Windows.
- The Windows binary resource still identified itself as 2.0.29.0 even when the installer was v2.0.61. Now the compiled resources, version control and installer agree on **2.0.62**.

## Compatibility
No changes to native APEX or YAMNet models, trained profiles, DSP algorithms, saved presets, output drivers or permissions. Dark Theme, liquid button effects, EQ band choices and system-tray behavior are preserved.

Native regression tests verify the source layout contracts and Windows CI compiles the real application and installer. Physical appearance at different display-scaling factors and custom font settings still requires a Windows screenshot review.
