import {readFileSync} from "node:fs";
import assert from "node:assert/strict";
const file=(path)=>readFileSync(new URL("../"+path,import.meta.url),"utf8");
const h=file("fxsound/Source/GUI/FxSettingsDialog.h");
const ui=file("fxsound/Source/GUI/FxSettingsDialog.cpp");
const controller=file("fxsound/Source/GUI/FxController.cpp");
const native=file("fxsound/Project/FxSound_App.vcxproj");
const installer=file("Installer/Cyn/Cyn.iss");
assert(h.includes('TextButton version_button_ { "Version" }'));
assert(h.includes('TextButton check_updates_button_ { "Check for updates" }'));
assert(h.includes('ToggleButton automatic_updates_toggle_ { "Automatic updates" }'));
assert(ui.includes('version_button_.onClick = []()'));
assert(ui.includes('app->getApplicationVersion()'));
assert(ui.includes('check_updates_button_.onClick = []()'));
assert(ui.includes('FxController::getInstance().checkUpdates()'));
assert(ui.includes('FxController::getInstance().setAutoUpdates(automatic_updates_toggle_.getToggleState())'));
assert(ui.includes('version_button_.setBounds') && ui.includes('check_updates_button_.setBounds'));
assert(ui.includes('Installed: Cyn v'));
assert(!ui.includes('Cyn identity-2 / DSP v'),"Ambiguous old version text returned");
assert(controller.includes('const auto currentVersion=major+"."+minor+"."+patch'));
assert.equal((native.match(/JUCE_APP_VERSION=2\.0\.55\.0/g)||[]).length,4);
assert.match(installer,/^AppVersion=2\.0\.55\s*$/m);
assert.match(installer,/^VersionInfoVersion=2\.0\.55\.0\s*$/m);
console.log("PASS: installed version UI, manual update, auto-update toggle and matching native version");

// Prevent installer publication from failing on every subsequent version bump.
const buildInstaller=file("BUILD_INSTALLER.ps1");
const outputNameMatch=installer.match(/^OutputBaseFilename=([A-Za-z0-9._-]+)\s*$/m);
assert(outputNameMatch, "Inno Setup manifest must declare an output filename");
assert.equal(outputNameMatch[1], "Cyn-Setup-2.0.55-x64");
assert(buildInstaller.includes('^OutputBaseFilename=') && buildInstaller.includes('Groups[1].Value'),
  "Installer build must compute the actual executable name from Cyn.iss");
assert(!/\$installer\s*=\s*Join-Path\s+\$PSScriptRoot\s+'dist\\Cyn-Setup-\d/.test(buildInstaller),
  "Do not hardcode last release's installer path");
console.log("PASS: installer hash generated from compiled Inno Setup output filename");

assert.match(buildInstaller,
  /^\$outputName\s*=\s*\[regex\]::Match\(\$innoScript,\s*'[^'\r\n]+'\)\s*$/m,
  "Inno output-name regex must be one complete, parseable PowerShell expression");
assert.equal((buildInstaller.match(/Get-FileHash -LiteralPath \$installer -Algorithm SHA256/g)||[]).length,1,
  "Only hash installer once, after resolving and checking its path");
assert(buildInstaller.indexOf("$installer = Join-Path") <
       buildInstaller.indexOf("Get-FileHash -LiteralPath $installer"),
  "Resolve generated installer path before calculating its hash");
