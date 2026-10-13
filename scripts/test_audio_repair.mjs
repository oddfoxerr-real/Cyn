import assert from "node:assert/strict";
import { readFileSync } from "node:fs";

const read = path => readFileSync(path, "utf8");
const agent = read("fxsound/Source/GUI/CynAudioRepair.h");
const settings = read("fxsound/Source/GUI/FxSettingsDialog.cpp");
const controller = read("fxsound/Source/GUI/FxController.cpp");
const header = read("fxsound/Source/GUI/FxController.h");
const worker = read("CynAudioRepair.ps1");
const build = read("BUILD_CYN.ps1");

assert(agent.includes('getBool("cyn_repair_enabled", false)'), "Monitoring must be OFF on new installs");
assert(agent.includes('getBool("cyn_repair_minor_recovery", false)'), "Recovery must be OFF by default");
assert(agent.includes('getInt("cyn_repair_download_permission", 0)'), "Default must always be Ask Every Time");
assert(agent.includes("AskEveryTime = 0, AlwaysAllow = 1, NeverDownload = 2"), "Three permissions required");
assert(agent.includes("permission == Permission::NeverDownload && stage_ == Stage::Downloading"), "Revoking permission must stop a pending worker");
assert(agent.includes("if (!isEnabled() || isBusy()) return"), "Disabled mode must never launch a download");
assert(agent.includes("if (getPermission() == Permission::NeverDownload)"), "Never-download must block queued downloads");
assert(agent.includes("juce::AlertWindow::showOkCancelBox"), "Ask mode must request explicit approval");
assert(agent.includes("beginWorker(Stage::Downloading, candidate)"), "Only user-approved or always-allowed scans may download");
assert(agent.includes("SetupDiGetClassDevsW") && agent.includes("CM_Get_DevNode_Status"), "Diagnose PnP status rather than guessing from playback volume");
assert(agent.includes("CREATE_NO_WINDOW"), "No console flashing in background checks");
assert(agent.includes("stopMonitoring();") && agent.includes("TerminateProcess(workerProcess_, 1)"), "Opting out stops the local checker");
assert(controller.includes("audioRepair_->startIfEnabled()"), "Independent monitor must start at application startup");
assert(controller.includes("audioRepair_.reset();"), "Repair worker must shut down with Cyn");
assert(header.includes("getAudioRepair()"), "Native UI must access the running monitor");
assert(settings.includes('make_unique<SettingsButton>("Misc")'), "Misc tab must be accessible");
for (const label of ["Enable Auto Repair", "Ask every time (recommended)", "Always allow verified downloads",
                     "Never download drivers", "Diagnose now", "Check Windows Update",
                     "Open Windows Optional Updates", "Reset permissions to Ask"]) {
  assert(settings.includes(label), "Missing native repair control: " + label);
}
assert(build.includes("'CynAudioRepair.ps1'"), "Portable and installer payload must contain the worker");
assert(worker.includes("Microsoft.Update.Session") && worker.includes("Type='Driver'"), "Official driver category only");
assert(worker.includes("DEVPKEY_Device_HardwareIds") && worker.includes("DriverHardwareID"), "Reject non-matching devices");
assert(worker.includes("Identity.UpdateID") && worker.includes("Identity.RevisionNumber"), "Download must re-check update identity");
assert(worker.includes("$Action -eq 'Scan'") && worker.includes("$Action -eq 'Download'"), "Scan and download must remain separate");
assert(worker.includes("'downloaded'") && worker.includes("Not installed"), "Package download does not imply driver installation");
for (const unsafe of [/pnputil\s+\/add-driver/i, /Restart-Service\s/i, /Remove-PnpDevice\s/i,
                      /\.\s*Install\s*\(/i, /\$installer\s*=\s*\$session\.CreateUpdateInstaller/i,
                      /Invoke-WebRequest/i, /Start-BitsTransfer/i]) {
  assert(!unsafe.test(worker), "Worker may not install/change drivers or fetch arbitrary files: " + unsafe);
}
console.log("PASS: Cyn Auto Repair opt-in and three-mode permission contracts");
console.log("PASS: matching Windows Update only; download without driver installation");
console.log("PASS: native Misc settings, process lifecycle and packaged PowerShell worker");
