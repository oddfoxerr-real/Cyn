# CYN on Riff — Windows desktop migration

This is the Riff-first implementation. Instead of repainting old JUCE widgets to resemble Riff, it uses the actual rootscripts/riff Electron visual foundation as a standalone CYN frontend while delegating system-wide processing to existing installed native Cyn.exe.

## Implemented

- Electron desktop shell with original Riff CSS, Material surface classes, floating sidebar, pill navigation indicator, content-open animations and responsive structure.
- Pinned upstream Riff revision 398c3dac976bd7b4a7a4b4d7a6d7636ea868a0a8 with copyright and MIT license staged by scripts/vendor-riff.mjs.
- Native CYN processing control: power, presets, selected Windows output, pinned output, all five effects and 31/35/39 EQ bands.
- Smart Listening switch and sensitivity wired to the existing CYN single-instance command-line controller.
- Narrow FxController extension exposes actual active AI model and valid installed-model list; allows only recognized model selection through --select_ai.
- Overview curve derived from real CYN EQ values; it does not pretend to be a live FFT or neural signal.

The actual FxSound audio driver, Windows audio processing, DSP, inference, APEX/AUDONIC, trained models and saved presets stay in installed native Cyn.exe. No Riff media server, scraping service or separate Riff audio engine is launched.

## Windows build

Keep your current CYN installation. Use Node.js 22+.

1. Open a terminal in the riff-cyn directory.
2. Run npm install.
3. Run npm run vendor:riff to stage the exact upstream Riff stylesheet and MIT license.
4. Run npm start to preview or npm run build:win for an independent Windows installer.

The standalone CYN Riff frontend searches default CYN locations including Program Files, or you can use Settings > Locate Cyn.exe. Press Start engine to launch installed CYN minimized.

The preview installer is a separate product named CYN Riff. It does not modify native CYN installation or its audio driver. Existing v2.0.82/v2.0.83 native installations support core commands but will not advertise the new optional AI profile bridge until a later native backend update.

## Security

Electron renderer uses sandbox=true, contextIsolation=true, nodeIntegration=false, preload IPC allowlisting, process identity verification, full validation against live native status, direct execFile calls with no shell invocation, navigation blocking and no HTTP control server.

## Remaining migration

Native BrainScan activity, neural learning/training, AI DJ controls, richer telemetry and genuine FFT metering require real data/commands from FxController before the JUCE GUI can be retired. This preview intentionally does not invent live inference visualizations or simulate playback.

This preview has its own CI artifact and is not a CYN stable-release update. The existing native CYN updater remains unchanged.

## Licenses

Riff by rootscripts: MIT, included at renderer/vendor/Riff-LICENSE.txt at packaging. CYN/FxSound native: AGPL-3.0-or-later, with existing source and JUCE attributions retained. Distributors must preserve applicable upstream notices and source availability.
