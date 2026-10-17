[CmdletBinding()]
param([switch]$WithMcp, [switch]$CheckOnly, [switch]$Incremental, [ValidateRange(1,8)][int]$BuildWorkers=2)
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
& $msbuild $solution "/m:$BuildWorkers" /nr:false $target /p:Configuration=Release /p:Platform=x64 /p:PreferredToolArchitecture=x64 /nologo "/bl:$logs\cyn-release.binlog"
if ($LASTEXITCODE -ne 0) { throw "Release build failed ($LASTEXITCODE). See build-logs\cyn-release.binlog." }
$exe = Join-Path $root 'fxsound\Project\x64\Release\App\FxSound.exe'
if (!(Test-Path -LiteralPath $exe)) { throw 'MSBuild did not produce FxSound.exe.' }
$dist = Join-Path $root 'dist\Cyn'
New-Item -ItemType Directory -Force -Path $dist | Out-Null
Copy-Item -LiteralPath $exe -Destination (Join-Path $dist 'Cyn.exe') -Force
Copy-Item -LiteralPath (Join-Path $root 'CynUpdater.ps1'),(Join-Path $root 'CynUpdaterWorker.ps1'),(Join-Path $root 'CynUpdateWindow.xaml'),(Join-Path $root 'CynAudioRepair.ps1') -Destination $dist -Force
Copy-Item -LiteralPath (Join-Path $root 'branding') -Destination $dist -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'models\cyn-starter.json'),(Join-Path $root 'models\MODEL_CARD.md') -Destination $dist -Force
Copy-Item -LiteralPath (Join-Path $root 'models\BALANCE_MODEL_CARD.md'),(Join-Path $root 'models\balance-evaluation.json') -Destination $dist -Force
# Built-in AI catalogue is native app data, not a website or hosted service.
$aiProfiles = Join-Path $dist 'CynAiProfiles'
New-Item -ItemType Directory -Path $aiProfiles -Force | Out-Null
Copy-Item -Path (Join-Path $root 'ai-hub\community\*') -Destination $aiProfiles -Force
Copy-Item -LiteralPath (Join-Path $root 'bin\x64\Factsoft') -Destination $dist -Recurse -Force
Get-ChildItem -LiteralPath (Join-Path $root 'fxsound\Fonts') -File | Copy-Item -Destination $dist -Force
Copy-Item -LiteralPath (Join-Path $root 'LICENSE'),(Join-Path $root 'README_CYN.md'),(Join-Path $root 'VALIDATION.md'),(Join-Path $root 'RELEASE_CHECKS.md'),(Join-Path $root 'CYN_CONTROL.ps1'),(Join-Path $root 'LAUNCH_CYN.ps1'),(Join-Path $root 'INSTALL_CYN.ps1') -Destination $dist -Force
Copy-Item -LiteralPath (Join-Path $root 'third_party\JUCE\LICENSE.md') -Destination (Join-Path $dist 'JUCE-LICENSE.md') -Force
Copy-Item -LiteralPath (Join-Path $root 'docs\AI-DJ.md') -Destination (Join-Path $dist 'AI-DJ.md') -Force
# Remove obsolete documentation when reusing an older staged payload.
$obsoleteSpotifyDoc = Join-Path $dist 'SPOTIFY.md'
if (Test-Path -LiteralPath $obsoleteSpotifyDoc) { Remove-Item -LiteralPath $obsoleteSpotifyDoc }
Copy-Item -LiteralPath (Join-Path $root 'third_party\ast\LICENSE.txt') -Destination (Join-Path $dist 'AST-LICENSE.txt') -Force
# Optional AI DJ weights are user-downloaded only, never installer payload.
if (Get-ChildItem $dist -Recurse -File -Filter '*ast*onnx') {throw 'AI DJ model must not be bundled.'}
foreach ($json in Get-ChildItem $dist -Recurse -File -Filter '*.json') {
    if ((Get-Content $json.FullName -Raw) -match '"(access_token|refresh_token|client_secret)"\s*:') {throw 'Private Spotify credentials must not be bundled.'}
}
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
# Require genuine native ONNX model packaging for Cyn 2.0.18+ releases.
$manifest=Get-Content (Join-Path $root 'Installer/Cyn/Cyn.iss') -Raw
$published=[regex]::Match($manifest,'(?m)^AppVersion=([0-9]+\.[0-9]+\.[0-9]+)').Groups[1].Value
if ([version]$published -ge [version]'2.0.18') {
    $onnxStage=Join-Path $root '.build-tools\yamnet-package'
    & (Join-Path $root 'PREPARE_CYN_YAMNET.ps1') -Destination $onnxStage
    foreach ($f in @('onnxruntime.dll','yamnet-16k-0p975s.onnx','yamnet-manifest.json')) {
        Copy-Item -LiteralPath (Join-Path $onnxStage $f) -Destination $dist -Force
        if (!(Test-Path (Join-Path $dist $f))) { throw "Native YAMNet package missing: $f" }
    }
    Copy-Item -LiteralPath (Join-Path $root 'third_party/onnxruntime/NOTICE.md') -Destination (Join-Path $dist 'ONNXRUNTIME-NOTICE.md') -Force
    Copy-Item -LiteralPath (Join-Path $root 'third_party/onnxruntime/LICENSE.txt') -Destination (Join-Path $dist 'ONNXRUNTIME-LICENSE.txt') -Force
    Copy-Item -LiteralPath (Join-Path $root 'third_party/yamnet/LICENSE.txt') -Destination (Join-Path $dist 'YAMNET-LICENSE.txt') -Force
}

# Native APEX 6M preview loads its separately exported full ONNX model.
# Never substitute the old 209-parameter .cynai, nor silently skip missing weights.
if ([version]$published -ge [version]'2.0.50') {
    $apexStage=Join-Path $root '.build-tools\apex-package'
    & (Join-Path $root 'PREPARE_CYN_APEX6M.ps1') -Destination $apexStage
    foreach($name in @('apex6m-225.onnx','apex6m-manifest.json')) {
        Copy-Item -LiteralPath (Join-Path $apexStage $name) -Destination $dist -Force
        if (!(Test-Path (Join-Path $dist $name))) { throw "Full APEX ONNX package missing: $name" }
    }
}

Compress-Archive -Path (Join-Path $dist '*') -DestinationPath (Join-Path $root 'dist\Cyn-Windows-x64.zip') -Force
Write-Host "Built: $dist\Cyn.exe"
Write-Host 'Close the original FxSound, then run dist\Cyn\LAUNCH_CYN.ps1. Keep the installed FxSound driver.'
