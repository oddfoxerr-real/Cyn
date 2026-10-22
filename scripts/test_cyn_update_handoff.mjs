import assert from "node:assert/strict";
import { readFileSync } from "node:fs";

const read = (path) => readFileSync(new URL("../" + path, import.meta.url), "utf8");
const updater = read("CynUpdater.ps1");
const installer = read("Installer/Cyn/Cyn.iss");

assert(!updater.includes("-ArgumentList '/VERYSILENT'"), "Do not hide installer prompts.");
assert(!updater.includes("-ArgumentList '/SUPPRESSMSGBOXES'"), "Never auto-abort a recoverable setup error.");
assert(updater.includes("$arguments=@('/SILENT','/NORESTART'"), "Keep visible setup progress and allow error retry.");
assert(updater.includes("'/LOG="), "Inno setup must write a diagnostic log.");
assert(updater.includes("$script:installLogPath"), "Log path must persist for the update session.");
assert(updater.includes("Start-Process -FilePath $script:installerPath -Verb RunAs -ArgumentList $arguments -PassThru"), "Launch existing verified installer with UAC.");
assert(updater.includes("if ($code -eq 5)"), "Handle Inno abort/cancel specifically.");
assert(updater.includes("$progressText.Text='Installation stopped'"), "Clear stale installation activity on failure.");
assert(updater.includes("$downloadProgress.Value=0"), "Clear stale full progress after failure.");
assert(updater.includes("$laterButton.Content='View install log'"), "Make the log accessible after failure.");
assert(updater.includes("Get-Process -Name FxSound"), "Check for the original audio app before install.");
assert(installer.includes("CloseApplications=no"), "Do not forcibly terminate audio applications.");
assert(installer.includes("AppVersion=2.0.79"), "Require a new release rather than overwriting an old asset.");

console.log("PASS: recoverable installer, persistent logging, error-state UI and driver-safe process policy");
