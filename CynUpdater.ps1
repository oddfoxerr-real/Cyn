# Cyn 2.0.6+ secure update experience. UI stays responsive while a worker
# checks releases, streams real download progress and validates SHA-256.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][ValidatePattern('^\d+\.\d+\.\d+$')][string]$CurrentVersion,
    [switch]$Interactive,
    [switch]$Probe
)
$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
$workerPath=Join-Path $PSScriptRoot 'CynUpdaterWorker.ps1'
$xamlPath=Join-Path $PSScriptRoot 'CynUpdateWindow.xaml'
if (!(Test-Path -LiteralPath $workerPath)) {
    if ($Interactive) { Add-Type -AssemblyName PresentationFramework; [System.Windows.MessageBox]::Show('Updater worker file is missing. Reinstall Cyn.','Cyn Updates') | Out-Null }
    exit 1
}
if ($Probe) {
    & $workerPath -CurrentVersion $CurrentVersion -Probe
    exit $LASTEXITCODE
}

# OS-owned instance lock: an update in progress must never open another
# window/worker, even when Cyn's timer and an interactive check race.
# Probe mode is intentionally outside the mutex for read-only CI diagnostics.
$script:updaterMutex=[System.Threading.Mutex]::new($false,'Local\CynUpdaterSingleWindow')
try {
    $script:ownsUpdaterMutex=$script:updaterMutex.WaitOne(0,$false)
} catch [System.Threading.AbandonedMutexException] {
    $script:ownsUpdaterMutex=$true
}
if (!$script:ownsUpdaterMutex) {
    $script:updaterMutex.Dispose()
    exit 0
}


# Automatic checks are silent when no release is available. If one is found,
# the updater opens the real animated download view without requiring another action.
if (!$Interactive) {
    $found = & "$PSHOME\powershell.exe" -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $workerPath -CurrentVersion $CurrentVersion -Probe 2>$null
    if (!$found -or $LASTEXITCODE -ne 0) { exit 0 }
    # Respect Later for six hours, but notify immediately about a different release.
    $offeredVersion=([string]($found | Select-Object -First 1)).Trim()
    $deferredFile=Join-Path $env:LOCALAPPDATA 'Cyn\Updates\deferred-update.json'
    if (Test-Path -LiteralPath $deferredFile) {
        try {
            $deferred=Get-Content -LiteralPath $deferredFile -Raw | ConvertFrom-Json
            $nowUnix=[DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
            if ([string]$deferred.version -eq $offeredVersion -and
                [long]$deferred.untilUnix -gt $nowUnix) { exit 0 }
        } catch {
            # Malformed snooze metadata must never prevent an update check.
        }
    }
}
Add-Type -AssemblyName PresentationFramework,PresentationCore,WindowsBase
if (!(Test-Path -LiteralPath $xamlPath)) {
    [System.Windows.MessageBox]::Show('Update animation layout is missing. Reinstall Cyn.','Cyn Updates') | Out-Null
    exit 1
}
[xml]$markup=Get-Content -LiteralPath $xamlPath -Raw
$reader=New-Object System.Xml.XmlNodeReader($markup)
$window=[Windows.Markup.XamlReader]::Load($reader)
$stageTitle=$window.FindName('StageTitle')
$stageDetail=$window.FindName('StageDetail')
$downloadProgress=$window.FindName('DownloadProgress')
$progressText=$window.FindName('ProgressText')
$percentText=$window.FindName('PercentText')
$actionButton=$window.FindName('ActionButton')
$laterButton=$window.FindName('LaterButton')
$currentVersionText=$window.FindName('CurrentVersionText')
$latestVersionText=$window.FindName('LatestVersionText')
$dragSurface=$window.FindName('DragSurface')
$progressSteps=@('StepCheck','StepDownload','StepVerify','StepInstall') |
    ForEach-Object { $window.FindName($_) }
$script:activeStepBrush=New-Object Windows.Media.SolidColorBrush (
    [Windows.Media.ColorConverter]::ConvertFromString('#31565A'))
$script:inactiveStepBrush=New-Object Windows.Media.SolidColorBrush (
    [Windows.Media.ColorConverter]::ConvertFromString('#273846'))
$currentVersionText.Text='v'+$CurrentVersion
$dragSurface.Add_MouseLeftButtonDown({
    if ($_.LeftButton -eq [Windows.Input.MouseButtonState]::Pressed) {
        $window.DragMove()
    }
})
function Set-StageIndex([int]$index) {
    for($i=0;$i -lt $progressSteps.Count;$i++) {
        $progressSteps[$i].Background=if($i -eq $index) {
            $script:activeStepBrush
        } else {
            $script:inactiveStepBrush
        }
    }
}

$cache=Join-Path $env:LOCALAPPDATA 'Cyn\Updates'
New-Item -ItemType Directory -Path $cache -Force | Out-Null
$status=Join-Path ([IO.Path]::GetTempPath()) ("cyn-update-"+[guid]::NewGuid().ToString('N')+".json")
$psi=New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName=Join-Path $PSHOME 'powershell.exe'
$psi.Arguments='-NoProfile -NonInteractive -ExecutionPolicy Bypass -File "'+$workerPath+'" -CurrentVersion '+$CurrentVersion+' -StatusFile "'+$status+'" -CacheDirectory "'+$cache+'"'
$psi.CreateNoWindow=$true
$psi.UseShellExecute=$false
$worker=[System.Diagnostics.Process]::Start($psi)

$script:phase='checking'
$script:statusLast=''
$script:installRequested=$false
$script:installerPath=''
$script:newVersion=''
$script:installProcess=$null
$script:exitRequestedAt=[datetime]::MinValue
$script:finishedAt=[datetime]::MinValue
$script:allowedToClose=$false

function Update-Label([string]$title,[string]$detail) {
    $stageTitle.Text=$title
    $stageDetail.Text=$detail
}
function Show-Error([string]$message) {
    $script:phase='error'
    $downloadProgress.IsIndeterminate=$false
    $progressText.Text='Update could not be completed'
    Update-Label 'Update interrupted' $message
    $actionButton.Content='Close'
    $actionButton.IsEnabled=$true
    $laterButton.Visibility='Collapsed'
}
function Show-Ready([string]$version,[string]$path) {
    $script:phase='ready'
    $script:newVersion=$version
    $script:installerPath=$path
    Update-Label 'Ready to install' ("Cyn "+$version+" downloaded and verified.")
    $latestVersionText.Text='v'+$version
    $downloadProgress.IsIndeterminate=$false
    $downloadProgress.Value=100
    Set-StageIndex 3
    $actionButton.Content='Install & restart'
    $actionButton.IsEnabled=$true
    $laterButton.Visibility='Visible'
    $percentText.Text='100%'
    $progressText.Text='SHA-256 verified'
}
function Defer-CurrentUpdate {
    if ([string]::IsNullOrWhiteSpace($script:newVersion)) { return }
    try {
        $folder=Join-Path $env:LOCALAPPDATA 'Cyn\Updates'
        New-Item -ItemType Directory -Path $folder -Force | Out-Null
        $data=@{
            version=$script:newVersion
            untilUnix=[DateTimeOffset]::UtcNow.AddHours(6).ToUnixTimeSeconds()
        } | ConvertTo-Json -Compress
        [IO.File]::WriteAllText((Join-Path $folder 'deferred-update.json'),
            $data,(New-Object Text.UTF8Encoding($false)))
    } catch {
        # A read-only profile must not block closing the update window.
    }
}
function Spark-Reveal {
    # The last step is a genuine completed installer, not a simulated animation.
    Set-StageIndex 3
    $downloadProgress.IsIndeterminate=$false
    $downloadProgress.Value=100
}

$actionButton.Add_Click({
    if ($script:phase -eq 'ready') {
        $script:installRequested=$true
        $script:phase='closing'
        $script:exitRequestedAt=[datetime]::UtcNow
        $actionButton.IsEnabled=$false
        $laterButton.Visibility='Collapsed'
        Set-StageIndex 3
        Update-Label 'Preparing installation' 'Safely saving audio settings and closing Cyn...'
        $cyn=Join-Path $PSScriptRoot 'Cyn.exe'
        if (Test-Path -LiteralPath $cyn) {
            try {
                # JUCE single-instance IPC forwards this to the active Cyn process.
                Start-Process -FilePath $cyn -ArgumentList '--cyn-exit-for-update' -WindowStyle Hidden | Out-Null
            } catch { Show-Error ("Unable to close Cyn: "+$_.Exception.Message) }
        } else { Show-Error 'Cyn.exe is missing from the installed application folder.' }
        return
    }
    if ($script:phase -eq 'installing') { return }
    if ($worker -and !$worker.HasExited) { try { $worker.Kill() } catch {} }
    $script:allowedToClose=$true
    $window.Close()
})
$laterButton.Add_Click({
    Defer-CurrentUpdate
    $script:allowedToClose=$true
    $window.Close()
})
$window.Add_Closing({
    if ($script:phase -eq 'installing' -or $script:phase -eq 'closing') {
        $_.Cancel=$true
    } else {
        if ($script:phase -eq 'ready') { Defer-CurrentUpdate }
        if ($worker -and !$worker.HasExited) { try { $worker.Kill() } catch {} }
    }
})

$clock=New-Object Windows.Threading.DispatcherTimer
$clock.Interval=[TimeSpan]::FromMilliseconds(140)
$clock.Add_Tick({
    if ($script:phase -eq 'closing') {
        $running=@(Get-Process -Name Cyn -ErrorAction SilentlyContinue | Where-Object {
            try { $_.Path -ieq (Join-Path $PSScriptRoot 'Cyn.exe') } catch { $false }
        })
        if ($running.Count -eq 0) {
            try {
                $script:installProcess=Start-Process -FilePath $script:installerPath -Verb RunAs -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART' -PassThru
                $script:phase='installing'
                Set-StageIndex 3
                $downloadProgress.IsIndeterminate=$true
                Update-Label 'Installing Cyn' 'Windows is applying the verified update...'
                $progressText.Text='Installation in progress'
                $percentText.Text=''
            } catch {
                Show-Error ("Installer could not start: "+$_.Exception.Message)
            }
        } elseif (([datetime]::UtcNow - $script:exitRequestedAt).TotalSeconds -gt 25) {
            Show-Error 'Cyn did not exit cleanly. Try again after closing other audio apps.'
        }
        return
    }
    if ($script:phase -eq 'installing') {
        if ($script:installProcess -and $script:installProcess.HasExited) {
            if ($script:installProcess.ExitCode -ne 0) {
                Show-Error ("Installer returned error "+$script:installProcess.ExitCode+". Your previous installation remains available.")
                return
            }
            $script:phase='complete'
            $script:finishedAt=[datetime]::UtcNow
            Update-Label 'Update complete' ("Cyn "+$script:newVersion+" is ready. Launching...")
            $progressText.Text='Verified • installed • complete'
            $percentText.Text='100%'
            Spark-Reveal
        }
        return
    }
    if ($script:phase -eq 'complete') {
        if (([datetime]::UtcNow-$script:finishedAt).TotalMilliseconds -gt 1500) {
            try { Start-Process -FilePath (Join-Path $PSScriptRoot 'Cyn.exe') -WorkingDirectory $PSScriptRoot | Out-Null } catch {}
            $script:allowedToClose=$true
            $window.Close()
        }
        return
    }
    if ($script:phase -in @('ready','error','current')) { return }
    if (Test-Path -LiteralPath $status) {
        try {
            $data=Get-Content -LiteralPath $status -Raw | ConvertFrom-Json
            $phase=[string]$data.phase
            $detail=[string]$data.message
            if ($phase -eq 'checking') {
                Set-StageIndex 0
                $downloadProgress.IsIndeterminate=$true
                Update-Label 'Checking for updates' $detail
                $progressText.Text='Contacting GitHub'
            } elseif ($phase -eq 'downloading') {
                $script:phase='downloading'
                Set-StageIndex 1
                $downloadProgress.IsIndeterminate=$false
                if([string]$data.version) { $latestVersionText.Text='v'+[string]$data.version }
                Update-Label 'Downloading update' $detail
                $pct=[int]$data.percent
                $downloadProgress.Value=$pct
                $percentText.Text=if([long]$data.total -gt 0){"$pct%"}else{'Downloading'}
                if ([long]$data.total -gt 0) {
                    $mb=[math]::Round([double]$data.bytes/1MB,1)
                    $totalMb=[math]::Round([double]$data.total/1MB,1)
                    $progressText.Text="$mb / $totalMb MB"
                } else { $progressText.Text='Receiving installer bytes...' }
            } elseif ($phase -eq 'verifying') {
                $script:phase='verifying'
                Set-StageIndex 2
                $downloadProgress.IsIndeterminate=$true
                if([string]$data.version) { $latestVersionText.Text='v'+[string]$data.version }
                Update-Label 'Verifying update' 'Checking the downloaded SHA-256 hash...'
                $progressText.Text='Integrity check'
            } elseif ($phase -eq 'ready') {
                Show-Ready ([string]$data.version) ([string]$data.installer)
            } elseif ($phase -eq 'current') {
                $script:phase='current'
                Set-StageIndex 0
                $downloadProgress.IsIndeterminate=$false
                $latestVersionText.Text='Up to date'
                Update-Label 'Already up to date' $detail
                $progressText.Text='No update required'
                $actionButton.Content='Close'
                $downloadProgress.Value=100
            } elseif ($phase -eq 'error') { Show-Error $detail }
        } catch {
            # The worker may be replacing its small JSON status file.
        }
    }
    if ($worker.HasExited -and $script:phase -in @('checking','downloading','verifying')) {
        $worker.Refresh()
        if ($worker.HasExited) { Show-Error 'The update download stopped. Check your connection and try again.' }
    }
})
$clock.Start()
try {
    [void]$window.ShowDialog()
} finally {
    $clock.Stop()
    if ($worker -and !$worker.HasExited) { try { $worker.Kill() } catch {} }
    Remove-Item -LiteralPath $status -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath "$status.tmp" -Force -ErrorAction SilentlyContinue
}
