$ErrorActionPreference = 'Stop'
$exe = Join-Path $PSScriptRoot 'Cyn.exe'
if (!(Test-Path -LiteralPath $exe)) { $exe = Join-Path $PSScriptRoot 'dist\Cyn\Cyn.exe' }
if (!(Test-Path -LiteralPath $exe)) { throw 'Build the application first with BUILD_CYN.ps1.' }
foreach ($process in @(Get-Process FxSound,Cyn -ErrorAction SilentlyContinue)) {
    if ($process.Path -ne $exe) { throw 'Close the original FxSound from its tray menu before launching CYN. Both use the same audio driver.' }
}
Start-Process -FilePath $exe -ArgumentList '--view=2' -WorkingDirectory (Split-Path -Parent $exe)
