[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
if (!(Get-Command winget -ErrorAction SilentlyContinue)) { throw 'Install Microsoft App Installer (winget), then rerun this script.' }
& winget install --id Microsoft.VisualStudio.2022.BuildTools --exact --source winget --accept-package-agreements --accept-source-agreements --override '--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended'
if ($LASTEXITCODE -ne 0) { throw "Build Tools installation returned $LASTEXITCODE. Complete the installer, restart if requested, then run BUILD_CYN.ps1." }
Write-Host 'Build Tools setup finished. Run: powershell -ExecutionPolicy Bypass -File .\BUILD_CYN.ps1'
