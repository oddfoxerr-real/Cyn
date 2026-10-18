# Read-only on-device APEX diagnostics. No downloads, driver edits, or credentials.
# Run beside Cyn.exe after exiting Cyn via its notification-area icon.
[CmdletBinding()]
param([string]$CynFolder = $PSScriptRoot)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $CynFolder).Path
$report = Join-Path $env:TEMP ("Cyn-APEX-Diagnostics-{0}.txt" -f (Get-Date -Format 'yyyyMMdd-HHmmss'))
$lines = [System.Collections.Generic.List[string]]::new()
function Note([string]$message) { $lines.Add($message); Write-Host $message }
Note "CYN APEX 6M local diagnostic (read-only)"
Note "Timestamp: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')"
Note "Windows: $([Environment]::OSVersion.VersionString); 64-bit OS: $([Environment]::Is64BitOperatingSystem)"
Note "Install folder: $root"
$files = @('Cyn.exe','apex6m-225.onnx','apex6m-manifest.json','onnxruntime.dll')
foreach ($name in $files) {
    $item = Join-Path $root $name
    if (Test-Path -LiteralPath $item -PathType Leaf) {
        $f=Get-Item -LiteralPath $item
        Note "$name : present ($($f.Length) bytes)"
    } else { Note "$name : MISSING" }
}
$model = Join-Path $root 'apex6m-225.onnx'
$manifest = Join-Path $root 'apex6m-manifest.json'
if ((Test-Path -LiteralPath $model) -and (Test-Path -LiteralPath $manifest)) {
    try {
        $info=Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
        $hash=(Get-FileHash -LiteralPath $model -Algorithm SHA256).Hash
        $expected=[string]$info.sha256
        Note "APEX model manifest: $($info.parameter_count) parameters, $($info.checkpoint_training_steps) steps"
        Note "APEX SHA256: $(if ($hash -ieq $expected -and $expected -match '^[a-fA-F0-9]{64}$') {'MATCH'} else {'MISMATCH'})"
    } catch { Note "APEX model manifest check error: $($_.Exception.Message)" }
}
try {
    $redist = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64' -ErrorAction Stop
    Note "Visual C++ 2015-2022 x64 runtime installed: $([bool]($redist.Installed -eq 1))"
    Note "Visual C++ x64 runtime version: $($redist.Version)"
} catch {
    Note 'Visual C++ x64 runtime not registered. If APEX fails to load, consider installing the official Microsoft VC++ x64 redistributable.'
}
$src=@'
using System;
using System.Runtime.InteropServices;
public static class CynApexDllDiagnostic {
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    public static extern IntPtr LoadLibraryEx(string fileName, IntPtr reserved, uint flags);
    [DllImport("kernel32.dll", SetLastError=true)]
    public static extern bool FreeLibrary(IntPtr module);
}
'@
try {
    Add-Type -TypeDefinition $src -ErrorAction Stop
    $dll=Join-Path $root 'onnxruntime.dll'
    if (Test-Path -LiteralPath $dll) {
        $module=[CynApexDllDiagnostic]::LoadLibraryEx($dll,[IntPtr]::Zero,0)
        if ($module -ne [IntPtr]::Zero) {
            Note 'ONNX Runtime DLL load: SUCCESS (dependency loader succeeded)'
            [void][CynApexDllDiagnostic]::FreeLibrary($module)
        } else {
            $code=[Runtime.InteropServices.Marshal]::GetLastWin32Error()
            Note "ONNX Runtime DLL load: FAILED (Windows error $code)"
            if ($code -eq 126) { Note 'Error 126 often means a dependent DLL (for example the VC++ x64 runtime) is missing.' }
            if ($code -eq 193) { Note 'Error 193: wrong DLL architecture or invalid binary.' }
            if ($code -eq 1114) { Note 'Error 1114: dependent DLL initialization failed.' }
        }
    }
} catch {
    Note "DLL load probe unavailable: $($_.Exception.Message)"
}
Note 'This check does not initialize the ONNX model; detailed session errors appear in CYN AI Hub after a retry.'
Note 'Official VC++ x64 runtime (only if needed): https://aka.ms/vs/17/release/vc_redist.x64.exe'
$lines | Set-Content -LiteralPath $report -Encoding UTF8
Write-Host ""
Write-Host "Diagnostic report saved to: $report"
