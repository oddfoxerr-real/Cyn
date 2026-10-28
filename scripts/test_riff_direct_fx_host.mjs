import assert from "node:assert/strict";
import {readFileSync} from "node:fs";

const read = file => readFileSync(new URL("../" + file, import.meta.url), "utf8");

const nativeMain = read("fxsound/Source/Main.cpp");
const controller = read("fxsound/Source/GUI/FxController.cpp");
const bridge = read("riff-cyn/native-bridge.cjs");
const renderer = read("riff-cyn/renderer/renderer.js");
const html = read("riff-cyn/renderer/index.html");
const electron = read("riff-cyn/main.cjs");
const preload = read("riff-cyn/preload.cjs");
const installer = read("Installer/Cyn/Cyn.iss");
const updater = read("CynUpdater.ps1");
const launcher = read("LAUNCH_CYN.ps1");

assert(nativeMain.includes('const bool classicUi = commandline.trim().equalsIgnoreCase("--classic");'),
  "Native FxSound host may only construct the legacy UI with an explicit --classic developer switch");
assert(/if\s*\(classicUi\)\s*main_window_\s*=\s*std::make_unique<FxMainWindow>\(\)/.test(nativeMain),
  "Never construct the old JUCE GUI during a normal Riff launch");
assert(nativeMain.includes("FxController::getInstance().init(main_window_.get(), system_tray_view_.get(), audio_passthru_.get())"),
  "The original FxSound controller and audio passthrough must be retained");

assert(bridge.includes('spawn(exe,["--engine-host"]'),
  "Riff must start the actual native FxSound process headlessly");
assert(!bridge.includes('spawn(exe,["--run_minimized"]'),
  "The old full CYN application launch contract must be retired");
assert(bridge.includes("commandFor(action,state.status)"),
  "DSP controls must still go to the original native controller");
assert(controller.includes('if (arg_list.containsOption("--engine-host"))'),
  "The native controller must recognize its headless host role");
assert(controller.includes('if (!main_window_) return;'),
  "Legacy window-only operations must be safe when no window is constructed");
assert(controller.includes("audio_passthru_->setDspProcessingModule(&dfx_dsp_)"),
  "Do not replace FxSound DSP with a new layer");
assert(controller.includes("apex_worker_->startIfAvailable()"),
  "Retain original CYN AI workers");

assert(installer.includes('Name: "{autoprograms}\\Cyn"; Filename: "{app}\\CYN-Interface\\CYN Audio.exe"'),
  "Start menu must point directly to Riff");
assert(installer.includes('Name: "{autodesktop}\\Cyn"; Filename: "{app}\\CYN-Interface\\CYN Audio.exe"'),
  "Desktop must point directly to Riff");
assert(!installer.includes("Cyn (Classic)"), "No duplicate user-facing CYN application");
assert(updater.includes("Start-Process -FilePath $material -WorkingDirectory (Split-Path -Parent $material)"),
  "Updater should reopen Riff first");
assert(launcher.includes("Start-Process -FilePath $interface"),
  "Manual launcher should open Riff first");
assert(controller.includes("const auto riffGui = CynMaterialShell::executable();"),
  "Windows Run must open Riff first");
assert(controller.includes('const String command = "\\\"" + entryPoint + "\\\"";'),
  "Windows startup entry must properly quote the Riff executable path");
assert(!html.includes('id="fx-engine-classic"'), "No user-facing Legacy CYN controls");
assert(!renderer.includes("window.cyn.openNative()"), "Riff must not stack a second GUI");
assert(!electron.includes('ipcMain.handle("cyn:open-native"'), "No Electron legacy UI launch IPC");
assert(!preload.includes('openNative:'), "No renderer legacy UI launch permission");

console.log("PASS: one Riff CYN UI -> original headless FxSound engine, preserving DSP/AI/updater");
