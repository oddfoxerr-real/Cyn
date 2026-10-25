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
$script:installLogPath=''
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
    Update-Label 'Update interrupted' $message
    $actionButton.Content='Close'
    $actionButton.IsEnabled=$true
    $progressText.Text='Installation stopped'
    $percentText.Text=''
    $downloadProgress.Value=0
    if ($script:installLogPath -and (Test-Path -LiteralPath $script:installLogPath)) {
        $laterButton.Content='View install log'
        $laterButton.Visibility='Visible'
    } else {
        $laterButton.Visibility='Collapsed'
    }
}
function Show-Ready([string]$version,[string]$path) {
    $script:phase='ready'
    $script:newVersion=$version
    $script:installerPath=$path
    Update-Label 'Ready to install' ("Cyn "+$version+" downloaded and verified.")
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
    $samples=@(
        @('ParticleA','X',-146),@('ParticleB','Y',128),
        @('ParticleC','X',170),@('ParticleD','Y',-118)
    )
    foreach($sample in $samples) {
        $target=$window.FindName($sample[0])
        if ($null -eq $target) { continue }
        $property=if($sample[1] -eq 'X'){[Windows.Media.TranslateTransform]::XProperty}else{[Windows.Media.TranslateTransform]::YProperty}
        $animation=New-Object Windows.Media.Animation.DoubleAnimation
        $animation.From=0.0
        $animation.To=[double]$sample[2]
        $animation.Duration=[Windows.Duration]::new([TimeSpan]::FromMilliseconds(950))
        $animation.FillBehavior=[Windows.Media.Animation.FillBehavior]::HoldEnd
        $target.BeginAnimation($property,$animation)
    }
}

$actionButton.Add_Click({
    if ($script:phase -eq 'ready') {
        $script:installRequested=$true
        $script:phase='closing'
        $script:exitRequestedAt=[datetime]::UtcNow
        $actionButton.IsEnabled=$false
        $laterButton.Visibility='Collapsed'
        Update-Label 'Preparing installation' 'Safely saving audio settings and closing Cyn...'
        # Material CYN must exit gracefully before Inno overwrites Electron files.
        # Scope the request to our installed executable, never another Electron app.
        $material=Join-Path $PSScriptRoot 'CYN-Interface\CYN Audio.exe'
        if (Test-Path -LiteralPath $material) {
            foreach($proc in @(Get-Process -Name 'CYN Audio' -ErrorAction SilentlyContinue)) {
                try {
                    if ($proc.Path -ieq $material -and $proc.MainWindowHandle -ne 0) {
                        [void]$proc.CloseMainWindow()
                    }
                } catch { Write-Verbose ('Material exit request failed: '+$_.Exception.Message) }
            }
        }
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
    if ($script:phase -eq 'error' -and $script:installLogPath -and
        (Test-Path -LiteralPath $script:installLogPath)) {
        try {
            $notepad=Join-Path $env:SystemRoot 'System32\notepad.exe'
            Start-Process -FilePath $notepad -ArgumentList ('"'+$script:installLogPath+'"') | Out-Null
        } catch {
            Update-Label 'Update interrupted' ('Cannot open install log: '+$_.Exception.Message)
        }
        return
    }
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
        $material=Join-Path $PSScriptRoot 'CYN-Interface\CYN Audio.exe'
        $materialRunning=@(Get-Process -Name 'CYN Audio' -ErrorAction SilentlyContinue | Where-Object {
            try { $_.Path -ieq $material } catch { $false }
        })
        if ($running.Count -eq 0 -and $materialRunning.Count -eq 0) {
            if (@(Get-Process -Name FxSound -ErrorAction SilentlyContinue).Count -gt 0) {
                Show-Error 'FxSound is still running. Quit it from the system tray, then retry installation.'
                return
            }
            try {
                $script:installLogPath=Join-Path $cache ("Cyn-Setup-"+$script:newVersion+"-install.log")
                # The old /SUPPRESSMSGBOXES automatically chose Abort at recoverable
                # Inno Setup errors. Show progress and allow a human Retry/Cancel.
                $arguments=@('/SILENT','/NORESTART',('/LOG="'+$script:installLogPath+'"'))
                $script:installProcess=Start-Process -FilePath $script:installerPath -Verb RunAs -ArgumentList $arguments -PassThru
                $script:phase='installing'
                Update-Label 'Installing Cyn' 'Follow the Windows installer if it needs your attention.'
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
                $code=[int]$script:installProcess.ExitCode
                if ($code -eq 5) {
                    Show-Error 'Setup was cancelled or aborted during installation. Open the install log to identify the cause.'
                } else {
                    Show-Error ("Installer failed with exit code "+$code+". Open the install log for details.")
                }
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
            try {
                $material=Join-Path $PSScriptRoot 'CYN-Interface\CYN Audio.exe'
                if (Test-Path -LiteralPath $material) {
                    Start-Process -FilePath $material -WorkingDirectory (Split-Path -Parent $material) | Out-Null
                } else {
                    Start-Process -FilePath (Join-Path $PSScriptRoot 'Cyn.exe') -ArgumentList '--view=2' -WorkingDirectory $PSScriptRoot | Out-Null
                }
            } catch {}
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
                Update-Label 'Checking for updates' $detail
                $progressText.Text='Contacting GitHub'
            } elseif ($phase -eq 'downloading') {
                $script:phase='downloading'
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
                Update-Label 'Verifying update' 'Checking the downloaded SHA-256 hash...'
                $downloadProgress.Value=100
                $progressText.Text='Integrity check'
            } elseif ($phase -eq 'ready') {
                Show-Ready ([string]$data.version) ([string]$data.installer)
            } elseif ($phase -eq 'current') {
                $script:phase='current'
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
