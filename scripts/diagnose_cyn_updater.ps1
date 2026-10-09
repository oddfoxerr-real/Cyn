$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$updater = Join-Path $PSScriptRoot '..\CynUpdater.ps1'
if (!(Test-Path -LiteralPath $updater)) { throw 'CynUpdater.ps1 is missing.' }
$result = @(& $updater -CurrentVersion '2.0.2' -Probe)
if ($result.Count -ne 1) { throw "Expected exactly one detected version. Actual: $($result -join ', ')" }
$version = [version]$result[0]
if ($version -le [version]'2.0.2') { throw "Updater did not find a newer release: $version" }
Write-Host "SUCCESS: Cyn 2.0.2 detects GitHub Release v$version"
