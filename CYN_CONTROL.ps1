[CmdletBinding()]
param(
    [ValidateSet('Status','Power','Preset','Effects','Eq','Output','Smart','Sensitivity','PinOutput','Leveling','Theme','Motion')]
    [string]$Action = 'Status',
    [string]$Value = ''
)
$ErrorActionPreference = 'Stop'
$exe = Join-Path $PSScriptRoot 'Cyn.exe'
if (!(Test-Path -LiteralPath $exe)) { $exe = Join-Path $PSScriptRoot 'dist\Cyn\Cyn.exe' }
if (!(Test-Path -LiteralPath $exe)) { throw 'Build CYN first with BUILD_CYN.ps1.' }
$running = @(Get-Process FxSound,Cyn -ErrorAction SilentlyContinue)
if (!$running) { throw 'Start CYN with LAUNCH_CYN.ps1 first.' }
foreach ($process in $running) {
    if ($process.Path -ne $exe) { throw 'The running FxSound is not this CYN build. Close it and use LAUNCH_CYN.ps1.' }
}
if ($Value -match '["\r\n\x00]') { throw 'Quotes, line breaks and NUL are not allowed in values.' }
$flags = @{ Power='power'; Preset='preset'; Effects='set_effect'; Eq='set_band_gain'; Output='output'; Smart='smart_listening'; Sensitivity='smart_sensitivity'; PinOutput='pin_output'; Leveling='volume_leveling'; Theme='theme'; Motion='reduce_motion' }
if ($Action -ne 'Status') {
    if (!$Value.Length) { throw 'A Value is required for control actions.' }
    if ($Action -in @('Power','Smart','PinOutput','Motion') -and $Value -notin @('0','1')) { throw 'Value must be 0 or 1.' }
    if ($Action -eq 'Theme' -and $Value -notin @('Graphite','Pearl','Aurora','Dusk','Copper','Midnight')) { throw 'Choose Graphite, Pearl, Aurora, Dusk, Copper or Midnight.' }
    if ($Action -eq 'Sensitivity' -and ($Value -notmatch '^\d{1,3}$' -or [int]$Value -gt 100)) { throw 'Sensitivity must be an integer from 0 to 100.' }
    # Values are passed to the existing CLI parser; no shell is invoked.
    $argument = '--' + $flags[$Action] + '="' + $Value + '"'
    $child = Start-Process -FilePath $exe -ArgumentList $argument -PassThru -WindowStyle Hidden
    if (!$child.WaitForExit(10000)) { throw 'FxSound command timed out.' }
}
$status = Join-Path $env:APPDATA 'Cyn\status.json'
$before = if (Test-Path -LiteralPath $status) { (Get-Item -LiteralPath $status).LastWriteTimeUtc } else { [datetime]::MinValue }
$child = Start-Process -FilePath $exe -ArgumentList '--status' -PassThru -WindowStyle Hidden
if (!$child.WaitForExit(10000)) { throw 'FxSound status request timed out.' }
$deadline = [datetime]::UtcNow.AddSeconds(5)
while ([datetime]::UtcNow -lt $deadline) {
    if ((Test-Path -LiteralPath $status) -and (Get-Item -LiteralPath $status).LastWriteTimeUtc -gt $before) {
        try {
            $json = Get-Content -LiteralPath $status -Raw
            $parsed = $json | ConvertFrom-Json
            if ($parsed.cyn_build -ne 'identity-2') { throw 'Status is from a different FxSound build.' }
            $json
            return
        } catch { $lastError = $_ }
    }
    Start-Sleep -Milliseconds 100
}
throw "No fresh CYN status arrived. $lastError"
