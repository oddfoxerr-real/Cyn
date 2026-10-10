# Shared update policy; loading this file performs no installation or process changes.
function Get-CynBinaryVersion([string]$Path) {
    if (!(Test-Path -LiteralPath $Path -PathType Leaf)) { return $null }
    $v=[Diagnostics.FileVersionInfo]::GetVersionInfo($Path)
    if ($v.FileMajorPart -eq 0 -and $v.FileMinorPart -eq 0 -and $v.FileBuildPart -eq 0) { return $null }
    return [version]::new($v.FileMajorPart,$v.FileMinorPart,$v.FileBuildPart)
}
function Get-CynRegisteredDirectories {
    $key='SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{3049F105-E971-49A1-876E-50FDB14EF351}_is1'
    foreach($hive in @([Microsoft.Win32.RegistryHive]::LocalMachine,[Microsoft.Win32.RegistryHive]::CurrentUser)) {
        foreach($view in @([Microsoft.Win32.RegistryView]::Registry64,[Microsoft.Win32.RegistryView]::Registry32)) {
            $base=$null; $entry=$null
            try {
                $base=[Microsoft.Win32.RegistryKey]::OpenBaseKey($hive,$view)
                $entry=$base.OpenSubKey($key)
                if($entry) { $value=[string]$entry.GetValue('InstallLocation'); if($value) { $value } }
            } finally { if($entry){$entry.Dispose()}; if($base){$base.Dispose()} }
        }
    }
}
function Resolve-CynInstallDirectory([string]$Origin,[string[]]$RegisteredDirectories) {
    $candidates=@(foreach($dir in $RegisteredDirectories) {
        if(!$dir) { continue }
        $version=Get-CynBinaryVersion (Join-Path $dir 'Cyn.exe')
        if($version) { [pscustomobject]@{Directory=$dir;Version=$version} }
    })
    $newest=$candidates | Sort-Object Version -Descending | Select-Object -First 1
    if($newest) { return [IO.Path]::GetFullPath($newest.Directory).TrimEnd('\') }
    $full=[IO.Path]::GetFullPath($Origin).TrimEnd('\')
    if($full -eq [IO.Path]::GetPathRoot($full).TrimEnd('\') -or !(Get-CynBinaryVersion (Join-Path $full 'Cyn.exe'))) {
        throw 'Cannot identify the Cyn installation. Run the downloaded installer directly.'
    }
    return $full
}
function Get-CynVersionFloor([version]$Current,[string[]]$Directories) {
    $floor=[version]$Current.ToString(3)
    foreach($dir in $Directories) {
        if(!$dir) { continue }
        $version=Get-CynBinaryVersion (Join-Path $dir 'Cyn.exe')
        if($version -and $version -gt $floor) { $floor=$version }
    }
    return $floor
}
function Assert-CynInstalledVersion([string]$Directory,[version]$Expected) {
    $actual=Get-CynBinaryVersion (Join-Path $Directory 'Cyn.exe')
    if(!$actual -or $actual -ne [version]$Expected.ToString(3)) {
        throw "The installer finished, but the app in $Directory is version '$actual', not $Expected. Cyn was not relaunched. Run the downloaded installer to repair it."
    }
}
function Get-CynSessionProcesses {
    $session=[Diagnostics.Process]::GetCurrentProcess().SessionId
    @(Get-Process -Name Cyn -ErrorAction SilentlyContinue | Where-Object { $_.SessionId -eq $session })
}
function Repair-CynUserLaunchers([string]$Directory) {
    $exe=Join-Path $Directory 'Cyn.exe'
    # Preserve the user's startup choice; only update an already enabled Cyn entry.
    $run='HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
    $old=(Get-ItemProperty -LiteralPath $run -Name Cyn -ErrorAction SilentlyContinue).Cyn
    if($old) { Set-ItemProperty -LiteralPath $run -Name Cyn -Value ('"'+$exe+'"') }
    $shell=New-Object -ComObject WScript.Shell
    foreach($folder in @([Environment]::GetFolderPath('Programs'),[Environment]::GetFolderPath('DesktopDirectory'))) {
        $path=Join-Path $folder 'Cyn.lnk'
        if(Test-Path -LiteralPath $path) {
            $link=$shell.CreateShortcut($path)
            if([IO.Path]::GetFileName($link.TargetPath) -ieq 'Cyn.exe') {
                $link.TargetPath=$exe; $link.WorkingDirectory=$Directory; $link.IconLocation=$exe; $link.Save()
            }
        }
    }
}
