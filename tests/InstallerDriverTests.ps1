$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\Installer\Cyn\Ensure-AudioDriver.ps1') -CheckOnly
function Invoke-CynDriverTool { throw 'A read-only check must never execute a driver tool.' }
function Get-CynDriverDevices { @([pscustomobject]@{ConfigManagerErrorCode=0}) }
if ((Initialize-CynAudioDriver) -notmatch 'healthy') { throw 'Healthy driver not reused.' }
function Get-CynDriverDevices { @() }
try { Initialize-CynAudioDriver; throw 'Missing driver was accepted.' }
catch { if ($_.Exception.Message -notmatch 'is not installed') { throw } }
function Get-CynDriverDevices { @([pscustomobject]@{ConfigManagerErrorCode=22}) }
try { Initialize-CynAudioDriver; throw 'Disabled driver was accepted.' }
catch { if ($_.Exception.Message -notmatch 'disabled or reports an error') { throw } }
function Get-CynDriverDevices { throw 'Enumeration unavailable' }
try { Initialize-CynAudioDriver; throw 'Enumeration error was swallowed.' }
catch { if ($_.Exception.Message -ne 'Enumeration unavailable') { throw } }
Write-Host 'PASS: healthy, absent, disabled and enumeration-failure checks; no driver mutation.'

# Exercise the installation orchestration with all OS writes and waits mocked.
$CheckOnly = $false
$script:installed = $false
$script:steps = [Collections.Generic.List[string]]::new()
function Get-CynDriverDevices {
    if ($script:installed) { @([pscustomobject]@{ConfigManagerErrorCode=0}) } else { @() }
}
function Get-AuthenticodeSignature {
    [pscustomobject]@{ Status='Valid'; SignerCertificate=[pscustomobject]@{Subject='Microsoft Windows Hardware Compatibility Publisher'} }
}
function Start-Sleep { }
function Invoke-CynDriverTool([string]$Tool, [string[]]$ToolArguments) {
    $script:steps.Add($ToolArguments[0])
    switch ($ToolArguments[0]) {
        'install' { $script:installed = $true; 'Success' }
        'getguid' { '{0.0.0.00000000}.{939d7d2d-cb17-4ca1-896b-c4319d1f9be2}' }
        'setname' { 'Success' }
        'defaultbuffersize' { '40' }
        default { throw 'Unexpected driver operation' }
    }
}
function Save-CynDriverConfiguration([string]$Guid, [string]$Buffer) {
    if ($Guid -notlike '{0.0.0.00000000}.*' -or $Buffer -ne '40') { throw 'Incorrect driver configuration' }
    $script:steps.Add('save')
}
if ((Initialize-CynAudioDriver) -notmatch 'Installed and initialized') { throw 'Install path failed' }
if (($script:steps -join ',') -ne 'install,getguid,setname,defaultbuffersize,save') { throw 'Unexpected installation order' }
$script:steps.Clear()
Initialize-CynAudioDriver | Out-Null
if ($script:steps.Count -ne 0) { throw 'Existing driver was modified' }
$script:installed = $false
function Get-AuthenticodeSignature { [pscustomobject]@{Status='HashMismatch'} }
try { Initialize-CynAudioDriver; throw 'Invalid signature was accepted' }
catch { if ($_.Exception.Message -notmatch 'signature could not be verified') { throw } }
if ($script:steps.Count -ne 0) { throw 'Invalid driver package was executed' }
Write-Host 'PASS: simulated first install, endpoint ID format, reinstall reuse and signature rejection.'
