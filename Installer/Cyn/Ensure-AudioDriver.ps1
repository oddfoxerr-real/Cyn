[CmdletBinding()]
param([switch]$CheckOnly, [string]$ResultFile)
$ErrorActionPreference = 'Stop'

function Get-CynDriverDevices {
    # Present devices only: disconnected/stale instances must not suppress install.
    @(Get-CimInstance Win32_PnPEntity -Filter "Service='fxvad'" -ErrorAction Stop |
        Where-Object { $_.Present -eq $true })
}

function Invoke-CynDriverTool([string]$Tool, [string[]]$ToolArguments) {
    $output = & (Join-Path $PSScriptRoot $Tool) @ToolArguments 2>&1
    if ($LASTEXITCODE -ne 0) { throw "$Tool failed ($LASTEXITCODE): $output" }
    return ($output -join "`n").Trim()
}

function Save-CynDriverConfiguration([string]$Guid, [string]$Buffer) {
    $base = [Microsoft.Win32.RegistryKey]::OpenBaseKey('LocalMachine', 'Registry64')
    try {
        $key = $base.CreateSubKey('Software\DFX\23\devices')
        try {
            $key.SetValue('dfx_guid', $Guid, 'String')
            $key.SetValue('default_buffer_size', $Buffer, 'String')
        } finally { $key.Dispose() }
    } finally { $base.Dispose() }
}

function Initialize-CynAudioDriver {
    $devices = @(Get-CynDriverDevices)
    if ($devices.Count -gt 0) {
        if (@($devices | Where-Object { $_.ConfigManagerErrorCode -eq 0 }).Count -eq 0) {
            throw 'The existing FxSound audio device is disabled or reports an error. Enable or repair it in Device Manager, then retry. Setup has not replaced it.'
        }
        return 'Reused the existing healthy FxSound virtual audio driver.'
    }
    if ($CheckOnly) { throw 'FxSound virtual audio driver is not installed.' }

    # Keep the upstream signed INF/SYS/CAT unchanged. This is the same root-device
    # creation command used by Installer/DfxInstall/DfxInstall/DfxInstall.cpp.
    foreach ($file in @('fxvad.sys', 'fxvadntamd64.cat')) {
        $signature = Get-AuthenticodeSignature (Join-Path $PSScriptRoot $file)
        if ($signature.Status -ne 'Valid' -or
            $signature.SignerCertificate.Subject -notlike '*Microsoft Windows Hardware Compatibility Publisher*') {
            throw "The audio driver signature could not be verified: $file"
        }
    }
    if ($devices.Count -eq 0) {
        $output = Invoke-CynDriverTool 'fxdevcon64.exe' @('install', (Join-Path $PSScriptRoot 'fxvad.inf'))
        if ($output -notmatch 'Success') { throw "Audio driver installation failed: $output" }
    }
    $guid = ''
    for ($attempt = 0; $attempt -lt 15; $attempt++) {
        Start-Sleep -Seconds 1
        $guid = Invoke-CynDriverTool 'DfxSetupDrv.exe' @('getguid')
        if ($guid -match '^(?:\{0\.0\.0\.00000000\}\.)?\{[0-9a-fA-F-]{36}\}$') { break }
    }
    if ($guid -notmatch '^(?:\{0\.0\.0\.00000000\}\.)?\{[0-9a-fA-F-]{36}\}$') {
        throw 'The driver was added but its audio endpoint is not ready. Restart Windows and run setup again.'
    }
    $output = Invoke-CynDriverTool 'DfxSetupDrv.exe' @('setname')
    if ($output -notmatch 'Success') { throw "Could not initialize the audio endpoint: $output" }
    $buffer = Invoke-CynDriverTool 'DfxSetupDrv.exe' @('defaultbuffersize')
    if ($buffer -notmatch '^\d+$') { throw "Invalid driver buffer size: $buffer" }
    # Match upstream driver initialization in the 64-bit machine registry.
    Save-CynDriverConfiguration $guid $buffer
    $devices = @(Get-CynDriverDevices)
    if (@($devices | Where-Object { $_.ConfigManagerErrorCode -eq 0 }).Count -eq 0) {
        throw 'The driver is not ready. Restart Windows and run setup again.'
    }
    return 'Installed and initialized the FxSound virtual audio driver.'
}

# Dot sourcing exposes the functions for isolated, non-mutating tests.
if ($MyInvocation.InvocationName -ne '.') {
    try {
        $message = Initialize-CynAudioDriver
        if ($ResultFile) { Set-Content -LiteralPath $ResultFile -Value $message -Encoding UTF8 }
        Write-Output $message
        exit 0
    } catch {
        $message = $_.Exception.Message
        if ($ResultFile) { Set-Content -LiteralPath $ResultFile -Value $message -Encoding UTF8 }
        Write-Error $message
        exit 1
    }
}
