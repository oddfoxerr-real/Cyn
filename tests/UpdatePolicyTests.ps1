$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
. (Join-Path $root 'CynUpdatePolicy.ps1')
foreach($name in @('CynUpdater.ps1','CynUpdaterWorker.ps1','CynUpdatePolicy.ps1')) {
    $tokens=$null; $errors=$null
    [void][Management.Automation.Language.Parser]::ParseFile((Join-Path $root $name),[ref]$tokens,[ref]$errors)
    if($errors.Count) { throw ($errors | Out-String) }
}
# Simulate an updater started in an old extraction while a newer app is installed.
$script:versions=@{
    'C:\Old extracted\Cyn.exe'=[version]'2.0.1'
    'C:\Program Files\Cyn\Cyn.exe'=[version]'2.0.16'
}
function Get-CynBinaryVersion([string]$Path) { $script:versions[$Path] }
$target=Resolve-CynInstallDirectory 'C:\Old extracted' @('C:\missing','C:\Program Files\Cyn\')
if($target -ne 'C:\Program Files\Cyn') { throw 'Old extraction selected instead of installed app' }
if((Get-CynVersionFloor ([version]'2.0.1') @('C:\Old extracted',$target)) -ne [version]'2.0.16') { throw 'Downgrade floor failed' }
try { Assert-CynInstalledVersion $target ([version]'2.0.17'); throw 'Accepted stale installed binary' }
catch { if($_.Exception.Message -eq 'Accepted stale installed binary') { throw } }
$script:versions['C:\Program Files\Cyn\Cyn.exe']=[version]'2.0.17'
Assert-CynInstalledVersion $target ([version]'2.0.17')
if((Get-CynVersionFloor ([version]'2.0.20') @($target)) -ne [version]'2.0.20') { throw 'Newer running copy was ignored' }
if((Resolve-CynInstallDirectory 'C:\Old extracted' @()) -ne 'C:\Old extracted') { throw 'Portable fallback failed' }
$script:versions['C:\User installation\Cyn.exe']=[version]'2.0.20'
if((Resolve-CynInstallDirectory 'C:\Old extracted' @($target,'C:\User installation')) -ne 'C:\User installation') { throw 'Newest registered installation ignored' }
try { Resolve-CynInstallDirectory 'C:\' @(); throw 'Accepted drive root' }
catch { if($_.Exception.Message -eq 'Accepted drive root') { throw } }
Write-Host 'PASS: script syntax; installed path wins; missing registrations ignored; downgrade floors; post-install version verification; portable fallback; invalid target rejected.'
