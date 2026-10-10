[CmdletBinding()]
param([switch]$WithMcp, [switch]$CheckOnly, [switch]$Incremental)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) {
    throw 'Visual Studio 2022 C++ Build Tools are missing. Run SETUP_BUILD_TOOLS.ps1, then BUILD_CYN.ps1.'
}
$installation = & $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1
if (!$installation) { throw 'Visual Studio 2022 with the x64 C++ toolset is required. Run SETUP_BUILD_TOOLS.ps1.' }
$msbuild = Join-Path $installation 'MSBuild\Current\Bin\MSBuild.exe'
$sdk = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Include'
if (!(Test-Path -LiteralPath $sdk)) { throw 'Windows SDK is missing. Run SETUP_BUILD_TOOLS.ps1.' }
if (!(Test-Path -LiteralPath (Join-Path $root 'third_party\JUCE\modules\juce_core\juce_core.h'))) { throw 'Bundled JUCE 6.1.6 is missing. Extract the entire Cyn source ZIP.' }
if ($CheckOnly) { Write-Host 'Visual Studio, Windows SDK and JUCE found.'; return }
$logs = Join-Path $root 'build-logs'
New-Item -ItemType Directory -Force -Path $logs | Out-Null
$solution = Join-Path $root 'fxsound\Project\FxSound.sln'
$target = if ($Incremental) { '/t:Build' } else { '/t:Rebuild' }
& $msbuild $solution /m:2 $target /p:Configuration=Release /p:Platform=x64 /p:PreferredToolArchitecture=x64 /nologo "/bl:$logs\cyn-release.binlog"
if ($LASTEXITCODE -ne 0) { throw "Release build failed ($LASTEXITCODE). See build-logs\cyn-release.binlog." }
$exe = Join-Path $root 'fxsound\Project\x64\Release\App\FxSound.exe'
if (!(Test-Path -LiteralPath $exe)) { throw 'MSBuild did not produce FxSound.exe.' }
$dist = Join-Path $root 'dist\Cyn'
New-Item -ItemType Directory -Force -Path $dist | Out-Null
Copy-Item -LiteralPath $exe -Destination (Join-Path $dist 'Cyn.exe') -Force
Copy-Item -LiteralPath (Join-Path $root 'CynUpdater.ps1'),(Join-Path $root 'CynUpdaterWorker.ps1'),(Join-Path $root 'CynUpdateWindow.xaml') -Destination $dist -Force
Copy-Item -LiteralPath (Join-Path $root 'branding') -Destination $dist -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'models\cyn-starter.json'),(Join-Path $root 'models\MODEL_CARD.md') -Destination $dist -Force
# Built-in AI catalogue is native app data, not a website or hosted service.
$aiProfiles = Join-Path $dist 'CynAiProfiles'
New-Item -ItemType Directory -Path $aiProfiles -Force | Out-Null
Copy-Item -Path (Join-Path $root 'ai-hub\community\*') -Destination $aiProfiles -Force
Copy-Item -LiteralPath (Join-Path $root 'bin\x64\Factsoft') -Destination $dist -Recurse -Force
# Cyn modification 2026-10-10: ship every language font and its license notices.
# Remove only obsolete font files from a reused staging directory.
foreach ($obsoleteFont in @('Gilroy-Regular.ttf', 'Gilroy-Semibold.ttf', 'Gilroy-Bold.ttf')) {
    $obsoletePath = Join-Path $dist $obsoleteFont
    if (Test-Path -LiteralPath $obsoletePath) { Remove-Item -LiteralPath $obsoletePath -Force }
}
Get-ChildItem -LiteralPath (Join-Path $root 'fxsound\Fonts') -File | Copy-Item -Destination $dist -Force
Get-ChildItem -LiteralPath (Join-Path $root 'Installer\Resources\Fonts') -File | Copy-Item -Destination $dist -Force
Copy-Item -LiteralPath (Join-Path $root 'licenses') -Destination $dist -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'THIRD_PARTY_NOTICES.md'),(Join-Path $root 'MODIFICATIONS.md') -Destination $dist -Force
New-Item -ItemType Directory -Force -Path (Join-Path $dist 'docs') | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'docs\LICENSING_AUDIT.md') -Destination (Join-Path $dist 'docs') -Force
Copy-Item -LiteralPath (Join-Path $root 'LICENSE'),(Join-Path $root 'README_CYN.md'),(Join-Path $root 'VALIDATION.md'),(Join-Path $root 'RELEASE_CHECKS.md'),(Join-Path $root 'CYN_CONTROL.ps1'),(Join-Path $root 'LAUNCH_CYN.ps1'),(Join-Path $root 'INSTALL_CYN.ps1') -Destination $dist -Force
Copy-Item -LiteralPath (Join-Path $root 'third_party\JUCE\LICENSE.md') -Destination (Join-Path $dist 'JUCE-LICENSE.md') -Force
if ($WithMcp) {
    if (!(Get-Command go -ErrorAction SilentlyContinue)) { throw 'MCP build requires Go 1.26.5 or newer: winget install --id GoLang.Go --exact' }
    & $msbuild (Join-Path $root 'fxdiag\fxdiag.vcxproj') /m:2 /t:Rebuild /p:Configuration=Release /p:Platform=x64 /nologo
    if ($LASTEXITCODE -ne 0) { throw 'fxdiag build failed.' }
    Push-Location (Join-Path $root 'fxmcp')
    try {
        & go test ./...
        if ($LASTEXITCODE -ne 0) { throw 'MCP tests failed.' }
        & go build -trimpath '-ldflags=-s -w' -o (Join-Path $dist 'fxmcp.exe') ./cmd/fxmcp
        if ($LASTEXITCODE -ne 0) { throw 'MCP build failed.' }
    } finally { Pop-Location }
    Copy-Item -LiteralPath (Join-Path $root 'bin\x64\fxdiag.exe') -Destination $dist -Force
}
Compress-Archive -Path (Join-Path $dist '*') -DestinationPath (Join-Path $root 'dist\Cyn-Windows-x64.zip') -Force
Write-Host "Built: $dist\Cyn.exe"
Write-Host 'Close the original FxSound, then run dist\Cyn\LAUNCH_CYN.ps1. Keep the installed FxSound driver.'
