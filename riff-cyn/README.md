# CYN on Riff — Windows desktop migration

Riff is the **only user-facing CYN Windows interface**. The one combined CYN installer places the Riff-based Electron UI at `CYN-Interface/CYN Audio.exe` and the original FxSound DSP/AI host at `Cyn.exe`. The Riff app starts the host with `--engine-host`, controls it using its typed native command interface, and renders verified native status. The old JUCE CYN application window is never constructed, even by a legacy `--classic` launch. No Riff DSP engine or nested old CYN UI is involved.

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

On an installed release, **CYN** shortcuts launch `CYN-Interface/CYN Audio.exe` directly. The interface starts the bundled native FxSound host headlessly; if that host fails it displays diagnostics and offers Repair install. The standalone development mode may locate a manually installed `Cyn.exe` for testing.

The production installer is a single CYN package. The old Classic shortcut, second native UI, manual engine selection and Start/Connect Engine buttons have been removed. One power button controls native FxSound processing and the audio service starts and recovers automatically. Advanced sound, AI, EQ, Presets, Settings, and updater controls remain routed to the native CYN/FxSound controller. The deprecated standalone preview should not be installed alongside the combined product.

## Security

Electron renderer uses sandbox=true, contextIsolation=true, nodeIntegration=false, preload IPC allowlisting, process identity verification, full validation against live native status, direct execFile calls with no shell invocation, navigation blocking and no HTTP control server.

## Remaining migration

BrainScan neural activity, AI training, AI DJ, live spectrum, DSP controls and presets remain connected to the native FxController. When a live signal is unavailable, CYN reports it instead of generating fabricated data.

The Riff-first refactor is validated on a separate Windows CI branch before a stable signed release. Windows update checks, verified installer downloads and existing presets remain with the native CYN updater. A physical Windows PC playback/output-device test is still required before shipping this restructuring.

## Licenses

Riff by rootscripts: MIT, included at renderer/vendor/Riff-LICENSE.txt at packaging. CYN/FxSound native: AGPL-3.0-or-later, with existing source and JUCE attributions retained. Distributors must preserve applicable upstream notices and source availability.

## One app and one power control

The combined Windows installer exposes only **CYN Audio.exe** as the user-facing application. The bundled **Cyn.exe** is an implementation detail: the headless FxSound DSP/driver and AI host, not a second interface. CYN starts it automatically and uses rate-limited recovery after transient failures. **Turn on / Turn off** maps to the original native `--power=1` / `--power=0` command and does not spawn or kill audio services, delete presets or reset the selected device. Legacy taskbar shortcuts redirect into the new interface. Packaged CYN will never silently adopt a stale native executable from an older installation; driver repair remains available from the official installer.

One installed app does not mean one operating-system process: Electron presents the single window while the original Windows audio host runs headlessly for driver compatibility.
