// Prevent the native ONNX Runtime DLL from silently failing with missing VC++ x64 dependencies.
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const read=path=>readFileSync(new URL("../"+path,import.meta.url),"utf8");
const manifest=read("Installer/Cyn/Cyn.iss");
const build=read("BUILD_INSTALLER.ps1");
const stage=read("PREPARE_CYN_VCRUNTIME.ps1");
const notes=read("Installer/Cyn/SETUP-NOTES.txt");
const worker=read("fxsound/Source/GUI/CynApex6MWorker.h");
assert(manifest.includes('Name: vcruntime;') &&
       manifest.includes('Name: repairvcruntime;') &&
       manifest.includes('Flags: unchecked; Check: HasVcRuntime') &&
       manifest.includes('Check: NeedsVcRuntime'),
       "Missing/installed VC++ runtime must have explicit, optional installer choices");
assert(manifest.includes('RegQueryDWordValue(HKLM64') &&
       manifest.includes("Microsoft\\VisualStudio\\14.0\\VC\\Runtimes\\x64"),
       "Use x64 VC++ runtime registration, not the existing x86 driver DLL");
assert(manifest.includes('ExtractTemporaryFile(\'VC_redist.x64.exe\')') &&
       manifest.includes('WizardIsTaskSelected(\'vcruntime\')') &&
       manifest.includes('WizardIsTaskSelected(\'repairvcruntime\')') &&
       manifest.includes("'/repair /quiet /norestart'") &&
       manifest.includes("'/install /quiet /norestart'") &&
       manifest.includes('ExitCode = 3010'),
       "Selected x64 runtime install/repair must execute after confirmation and surface restart/errors");
assert(manifest.includes('VC_redist.x64.exe"; Flags: dontcopy') &&
       build.includes("PREPARE_CYN_VCRUNTIME.ps1"),
       "The installer must embed the verified official runtime so end-user PC requires no network");
assert(stage.includes('https://aka.ms/vs/17/release/vc_redist.x64.exe') &&
       stage.includes('Get-AuthenticodeSignature') &&
       stage.includes("CN=Microsoft Corporation") &&
       stage.includes("signature.Status -ne 'Valid'"),
       "Runtime must be signed by Microsoft and verified before packaging");
assert(notes.includes('Windows error 126') && notes.includes('runtime is embedded'),
       "Installation notes should explain cross-PC APEX compatibility");
assert(worker.includes('GetLastError()') &&
       worker.includes('Microsoft Visual C++ x64 runtime'),
       "Still expose runtime errors if machine has other broken dependencies");
console.log("PASS: explicit x64 VC++ install/repair, official signed package, APEX 126 compatibility");
