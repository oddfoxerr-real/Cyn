import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const read=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const source=read("fxsound/Source/Main.cpp");
const native=read("fxsound/Source/GUI/FxController.cpp");
const header=read("fxsound/Source/GUI/FxController.h");
const shell=read("fxsound/Source/GUI/CynMaterialShell.h");
const updater=read("CynUpdater.ps1");
const installer=read("Installer/Cyn/Cyn.iss");
const renderer=read("riff-cyn/main.cjs");
const workflow=read(".github/workflows/windows-release.yml");

assert.equal((source.match(/commandline\.trim\(\)\.isEmpty\(\) && CynMaterialShell::launch\(\)/g)||[]).length,2,
  "Both first native launch and subsequent old shortcut invocations must open Material CYN");
assert(source.includes('#include "GUI/CynMaterialShell.h"'),"Native entry point must include Material launcher");
assert(shell.includes('getSiblingFile("CYN-Interface")') && shell.includes('getChildFile("CYN Audio.exe")'),
  "Native launcher must point to the bundled Material executable");
assert(shell.includes("::ShellExecuteW"),"Use Windows shell launch instead of replacing the audio engine");
assert(header.includes("classic_ui_requested_ = false"),"Classic session must be explicit and nonpersistent");
assert(native.includes('arg_list.containsOption("--classic") || arg_list.containsOption("--view")'),
  "Preserve explicit native Classic support");
assert(native.includes('else if (arg_list.containsOption("--classic") || view.isNotEmpty())'),
  "Native status/EQ commands must never display the old JUCE window");
assert(native.includes('!classic_ui_requested_ && CynMaterialShell::launch()'),
  "Native tray and Open actions must route to the Material UI by default");
assert(!native.includes('settings_.setString("version", app_version);\n\t\t\tsettings_.setBool("run_minimized", false);'),
  "Native version upgrades must not override minimized launch requests");
assert(renderer.includes("app.requestSingleInstanceLock()"),
  "Tray/menu shortcuts should focus one Material window instead of opening duplicates");

assert(updater.includes("CYN-Interface\\CYN Audio.exe"),"Updater must know the bundled Material executable");
assert(updater.includes("CloseMainWindow()"),"Request graceful shutdown; do not kill the audio/UI processes");
assert(updater.includes("$running.Count -eq 0 -and $materialRunning.Count -eq 0"),
  "Inno installation must wait for both native DSP and Electron GUI to close");
assert(updater.includes("Start-Process -FilePath $material -WorkingDirectory"),
  "After an in-place update, restart the actual Material UI, not old JUCE");
assert(installer.includes('Name: "{autoprograms}\\Cyn"; Filename: "{app}\\CYN-Interface\\CYN Audio.exe"'));
assert(installer.includes('Name: "{autoprograms}\\Cyn (Classic)"'));
assert(installer.includes("AppVersion=2.0.86"),"Bump version for new distributable instead of overwriting release");
assert(workflow.includes('run: .\\BUILD_INSTALLER.ps1'));
console.log("PASS: v2.0.86 Material launch and update flow, explicit Classic fallback, protected native DSP");
