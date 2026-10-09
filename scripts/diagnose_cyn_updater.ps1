$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$updater = Join-Path $PSScriptRoot '..\CynUpdater.ps1'
$worker = Join-Path $PSScriptRoot '..\CynUpdaterWorker.ps1'
if (!(Test-Path -LiteralPath $updater)) { throw 'CynUpdater.ps1 is missing.' }
$result = @(& $updater -CurrentVersion '2.0.2' -Probe)
if ($result.Count -ne 1) { throw "Expected exactly one detected version. Actual: $($result -join ', ')" }
$version = [version]$result[0]
if ($version -le [version]'2.0.2') { throw "Updater did not find a newer release: $version" }
Write-Host "SUCCESS: Cyn 2.0.2 detects GitHub Release v$version"

# Keep the actual worker's release enumeration covered by CI.
$workerResult = @(& $worker -CurrentVersion '2.0.2' -Probe)
if ($workerResult.Count -ne 1 -or [version]$workerResult[0] -le [version]'2.0.2') {
    throw 'Updater worker failed to discover a valid newer GitHub release.'
}
Write-Host "PASS: background downloader discovers v$($workerResult[0])"
