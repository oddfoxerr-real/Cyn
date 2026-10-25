# CYN Windows 2.0.87 — Restore AI functionality in Material desktop

CYN 2.0.87 brings native CYN features back to the redesigned Riff-derived Material 3 desktop. This is an **in-place update** through the existing CYN Windows installer and verified updater. It retains the original CYN audio driver, APEX/AUDONIC models, installed user profiles, presets, license notices and existing Windows app identity.

## Restored or improved
- **Live BrainScan and native neural activity**: real 11→12→5 neural inference signal, weighted connections, and AUDONIC creative channel measurements drive the BrainScan/neural graph. No fabricated activity or second audio engine.
- **AI-controlled sound visuals**: animated curve and markers reflect EQ and effects actually applied by native CYN rather than only manual slider positions; stale activity clears.
- **Smart Listening**: Music/Movie modes, headroom cap, automatic preset cycling, AUDONIC intensity, training progress and recent-audio More/Less feedback.
- **AI DJ**: native model download/cancel/remove, enablement, intensity, stereo control and remix spacing.
- **Sound Studio**: the 31/35/39-band EQ selector, balance, master gain, volume leveling and filter Q return as live native controls.
- **Settings**: native automatic update checks, launch with Windows and global keyboard shortcut toggles.
- **Updater reliability**: verified CYN installer remains the update path. The new UI can run the updater even if the native sound engine is offline by reading installed version metadata, and reports Material UI relaunch errors instead of silently opening CYN Classic. Existing app and data are preserved.

## Remaining limitations
- The redesigned interface still lacks complete community AI Hub browsing, protected model import/export, some training starter/reset and timeline tools, per-action keyboard shortcut remapping, and parts of the old advanced Theme Studio/settings experience.
- **CYN (Classic)** stays installed for these advanced features and troubleshooting until migration is complete.
- Automated Windows build/UI/DSP tests do not substitute for direct listening and device-routing verification on each user's audio hardware.

## Upgrading
Use **Settings → Check for Updates** in CYN, or the official Windows release installer attached here. It uses the same Inno Setup AppId and installation path as previous CYN releases; no uninstall, audio-driver reset, model removal or separate preview installation is needed.

This release updates **Windows desktop only**, not the Android APK.

Riff © rootscripts (MIT). FxSound/CYN native DSP © respective authors (AGPL-3.0-or-later). Preserve the bundled license and source notices.
