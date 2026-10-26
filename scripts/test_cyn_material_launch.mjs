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

assert.equal((source.match(/CynMaterialShell::isUiLaunchRequest\(commandline\)/g)||[]).length,2,
  "Both initial launch and running-instance IPC must redirect old pinned shortcuts");
assert(source.includes('#include "GUI/CynMaterialShell.h"'),"Native entry point must include Material launcher");
assert(shell.includes('getSiblingFile("CYN-Interface")') && shell.includes('getChildFile("CYN Audio.exe")'),
  "Native launcher must point to the bundled Material executable");
assert(shell.includes("::ShellExecuteW"),"Use Windows shell launch instead of replacing the audio engine");
for(const legacy of ["--view=1","--view=2","--view 1","--view 2"]){
  assert(shell.includes('equalsIgnoreCase("'+legacy+'")'),
    "Old taskbar launch must open the Material UI: "+legacy);
}
assert(source.includes("Reinstall the complete CYN update."),
  "Broken Material installations must explain repair, not display an unrelated old UI");
assert(source.includes('launchMaterial ? String("--run_minimized") : commandline'),
  "Normal UI launch must keep the real DSP host alive and hidden rather than quitting it");
assert(source.indexOf('FxController::getInstance().init(main_window_.get()')<
       source.indexOf('if (launchMaterial && !CynMaterialShell::launch())'),
  "Launch Material only after native DSP initialization to avoid a startup race");
assert(shell.includes('inline bool isUiLaunchRequest('));
assert(!shell.includes('trimmed.contains("--run_minimized")'),
  "DSP background invocation must never redirect into a visible shell");
assert(header.includes("classic_ui_requested_ = false"),"Classic session must be explicit and nonpersistent");
assert.equal((native.match(/if \(arg_list\.containsOption\("--classic"\)\)/g)||[]).length,3,
  "Native startup, IPC and explicit classic-view handling must all require --classic");
assert(!native.includes('arg_list.containsOption("--classic") || arg_list.containsOption("--view")'),
  "Legacy --view is normal CYN UI navigation, not permission to show JUCE");
assert(native.includes('else if (arg_list.containsOption("--classic"))'),
  "Native IPC commands must not display the old JUCE window");
assert(native.includes('if (!classic_ui_requested_)') && native.includes('if (!CynMaterialShell::launch())'),
  "Native tray opens must try the Material UI and never silently fall through to JUCE");
assert(!native.includes('settings_.setString("version", app_version);\n\t\t\tsettings_.setBool("run_minimized", false);'),
  "Native version upgrades must not override minimized launch requests");
assert(renderer.includes("app.requestSingleInstanceLock()"),
  "Tray/menu shortcuts should focus one Material window instead of opening duplicates");

assert(updater.includes("CYN-Interface\\CYN Audio.exe"),"Updater must know the bundled Material executable");
assert(updater.includes("CloseMainWindow()"),"Request graceful shutdown; do not kill the audio/UI processes");
assert(updater.includes("$running.Count -eq 0 -and $materialRunning.Count -eq 0"),
  "Inno installation must wait for both native DSP and Electron GUI to close");
assert(updater.includes("Start-Process -FilePath $native -WorkingDirectory $PSScriptRoot"),
  "After an in-place update, native CYN must initialize audio before the Material interface opens");
assert(installer.includes('Name: "{autoprograms}\\Cyn"; Filename: "{app}\\Cyn.exe"'),"Normal shortcut must start original CYN before Material");
assert(installer.includes('Name: "{autoprograms}\\Cyn (Classic)"'));
assert(installer.includes('Parameters: "--classic"'),"Original CYN must remain accessible for audio routing and driver repair");
assert(installer.includes("AppVersion=2.0.93"),"Bump version for new distributable instead of overwriting release");
assert(workflow.includes('run: .\\BUILD_INSTALLER.ps1'));
console.log("PASS: v2.0.93 Material launch and update flow, explicit Classic fallback, protected native DSP");
