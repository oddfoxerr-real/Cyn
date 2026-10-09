[CmdletBinding()]
param(
    [string]$InstallDirectory = (Join-Path $env:LOCALAPPDATA 'Programs\Cyn'),
    [switch]$Launch
)
$ErrorActionPreference = 'Stop'
$source = $PSScriptRoot
if (!(Test-Path -LiteralPath (Join-Path $source 'Cyn.exe'))) { $source = Join-Path $source 'dist\Cyn' }
if (!(Test-Path -LiteralPath (Join-Path $source 'Cyn.exe'))) { throw 'Build Cyn first with BUILD_CYN.ps1.' }
if (@(Get-Process Cyn,FxSound -ErrorAction SilentlyContinue).Count) {
    throw 'Exit Cyn and FxSound from their tray menus before installing.'
}
$destination = [IO.Path]::GetFullPath($InstallDirectory)
if ($destination -eq [IO.Path]::GetPathRoot($destination)) { throw 'Choose an application folder, not a drive root.' }
New-Item -ItemType Directory -Path $destination -Force | Out-Null
if ([IO.Path]::GetFullPath($source).TrimEnd('\') -ne $destination.TrimEnd('\')) {
    Get-ChildItem -LiteralPath $source | Copy-Item -Destination $destination -Recurse -Force
}
$programs = [Environment]::GetFolderPath('Programs')
$shortcut = (New-Object -ComObject WScript.Shell).CreateShortcut((Join-Path $programs 'Cyn.lnk'))
$shortcut.TargetPath = Join-Path $destination 'Cyn.exe'
$shortcut.WorkingDirectory = $destination
$shortcut.IconLocation = "$destination\Cyn.exe,0"
$shortcut.Description = 'Cyn - personal audio'
$shortcut.Save()
Write-Host "Installed: $destination\Cyn.exe"
Write-Host 'Open Cyn from the Start menu. The official FxSound audio driver must remain installed.'
if ($Launch) { Start-Process -FilePath $shortcut.TargetPath -WorkingDirectory $destination -ArgumentList '--view=2' }
