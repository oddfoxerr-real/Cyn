# Cyn Audio Repair: only discover/download matching audio driver updates through Windows Update.
# Never installs, rolls back, uninstalls or enables a driver. Caller must authorize a download.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][ValidateSet('Scan','Download')][string]$Action,
    [Parameter(Mandatory=$true)][string]$ResultPath,
    [string]$UpdateId = '',
    [int]$Revision = -1,
    [string]$DeviceId = '',
    [string]$TargetDeviceId = ''
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
function Finish([string]$state, [string]$message, [object]$info = $null) {
    $result = @{ status=$state; message=$message }
    if ($null -ne $info) {
        foreach ($key in $info.Keys) { $result[$key] = $info[$key] }
    }
    $json = ConvertTo-Json -InputObject $result -Depth 4 -Compress
    $folder = Split-Path -Parent $ResultPath
    if (-not (Test-Path -LiteralPath $folder)) { New-Item -ItemType Directory -Path $folder -Force | Out-Null }
    [IO.File]::WriteAllText($ResultPath, $json, [Text.UTF8Encoding]::new($false))
    exit 0
}
try {
    if ($Action -eq 'Download' -and (($UpdateId -notmatch '^[0-9a-fA-F-]{36}$') -or $Revision -lt 0 -or [string]::IsNullOrWhiteSpace($DeviceId))) {
        Finish 'error' 'Invalid driver update identity.'
    }

    # PnP MEDIA devices represent real audio driver hardware; an AudioEndpoint is
    # a logical endpoint and should never be treated as a driver download target.
    $devices = @(Get-PnpDevice -Class 'MEDIA' -PresentOnly -ErrorAction Stop)
    $hardware = @{}
    foreach ($device in $devices) {
        if ($Action -eq 'Scan' -and $TargetDeviceId -ne '' -and $device.InstanceId -ne $TargetDeviceId) { continue }
        try {
            $props = Get-PnpDeviceProperty -InstanceId $device.InstanceId -KeyName 'DEVPKEY_Device_HardwareIds' -ErrorAction Stop
            foreach ($id in @($props.Data)) {
                if (-not [string]::IsNullOrWhiteSpace([string]$id)) {
                    $hardware[[string]$id] = [string]$device.InstanceId
                }
            }
        } catch {
            # Not every device exposes hardware IDs. Never guess its identity.
        }
    }
    if ($hardware.Count -eq 0) {
        Finish 'none' 'No eligible audio hardware identifiers were found. Use Windows Update > Optional updates if needed.'
    }

    $session = New-Object -ComObject Microsoft.Update.Session
    $session.ClientApplicationID = 'Cyn Audio Repair'
    $searcher = $session.CreateUpdateSearcher()
    $results = $searcher.Search("IsInstalled=0 and IsHidden=0 and Type='Driver'")
    $candidates = @()
    for ($i=0; $i -lt $results.Updates.Count; $i++) {
        $update = $results.Updates.Item($i)
        $matchingHardware = [string]$update.DriverHardwareID
        if (-not $hardware.ContainsKey($matchingHardware)) { continue }
        $matchingDevice = [string]$hardware[$matchingHardware]
        if ($Action -eq 'Download' -and ($update.Identity.UpdateID -ne $UpdateId -or
            $update.Identity.RevisionNumber -ne $Revision -or $matchingDevice -ne $DeviceId)) { continue }
        $candidates += [pscustomobject]@{
            update = $update
            deviceId = $matchingDevice
        }
    }
    if ($candidates.Count -eq 0) {
        Finish 'none' 'Windows Update has no matching audio driver fix available.'
    }

    $choice = $candidates[0]
    $candidate = $choice.update
    $info = @{
        title = [string]$candidate.Title
        updateId = [string]$candidate.Identity.UpdateID
        revision = [int]$candidate.Identity.RevisionNumber
        deviceId = [string]$choice.deviceId
        source = 'Windows Update'
    }
    if ($Action -eq 'Scan') {
        Finish 'available' 'A matching audio driver update is available from Windows Update.' $info
    }

    # Re-check exact update ID, revision, and device hardware ID immediately before download.
    # WUA itself supplies the package; no executable, URL, or third-party mirror is accepted.
    if (-not $candidate.EulaAccepted) {
        Finish 'eula' 'Windows Update requires additional license acceptance; open Optional updates to continue.' $info
    }
    $updates = New-Object -ComObject Microsoft.Update.UpdateColl
    [void]$updates.Add($candidate)
    $downloader = $session.CreateUpdateDownloader()
    $downloader.Updates = $updates
    $download = $downloader.Download()
    if ([int]$download.ResultCode -ne 2) {
        Finish 'error' "Windows Update download was not fully successful (code $($download.ResultCode))." $info
    }
    Finish 'downloaded' 'Verified Windows Update driver payload downloaded. Not installed; install via Windows Update when you choose.' $info
} catch {
    $safeMessage = [string]$_.Exception.Message
    Finish 'error' ('Windows Update could not complete the request: ' + $safeMessage)
}
