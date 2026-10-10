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
# One updater per login session, including download and install phases.
$updateMutex=[Threading.Mutex]::new($false,'Local\Cyn.Update')
$ownsMutex=$false
try { $ownsMutex=$updateMutex.WaitOne(0) } catch [Threading.AbandonedMutexException] { $ownsMutex=$true }
if(!$ownsMutex) {
    if($Interactive) { Add-Type -AssemblyName PresentationFramework; [Windows.MessageBox]::Show('A Cyn update is already open. Finish or close that update first.','Cyn Updates') | Out-Null }
    $updateMutex.Dispose(); exit 0
}
try {
. (Join-Path $PSScriptRoot 'CynUpdatePolicy.ps1')
$registered=@(Get-CynRegisteredDirectories)
$installDirectory=Resolve-CynInstallDirectory $PSScriptRoot $registered
$CurrentVersion=(Get-CynVersionFloor ([version]$CurrentVersion) (@($PSScriptRoot)+$registered)).ToString(3)
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

# Automatic checks are silent when no release is available. If one is found,
# the updater opens the real animated download view without requiring another action.
if (!$Interactive) {
    $found = & "$PSHOME\powershell.exe" -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $workerPath -CurrentVersion $CurrentVersion -Probe 2>$null
    if (!$found -or $LASTEXITCODE -ne 0) { exit 0 }
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
    $laterButton.Visibility='Collapsed'
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
    if ($script:phase -eq 'stuck') {
        # Explicit user action, limited to the exact processes seen at timeout.
        try {
            foreach($old in $script:stuckProcesses) {
                $process=Get-Process -Id $old.Id -ErrorAction SilentlyContinue
                if($process -and $process.ProcessName -eq 'Cyn' -and $process.StartTime -eq $old.StartTime -and $process.SessionId -eq $old.SessionId) {
                    Stop-Process -InputObject $process -ErrorAction Stop
                }
            }
            $script:phase='closing'; $script:exitRequestedAt=[datetime]::UtcNow
            $actionButton.IsEnabled=$false; $laterButton.Visibility='Collapsed'
        } catch { Show-Error ('Unable to close Cyn: '+$_.Exception.Message) }
        return
    }
    if ($script:phase -eq 'ready') {
        $script:installRequested=$true
        $script:phase='closing'
        $script:exitRequestedAt=[datetime]::UtcNow
        $actionButton.IsEnabled=$false
        $laterButton.Visibility='Collapsed'
        Update-Label 'Preparing installation' 'Safely saving audio settings and closing Cyn...'
        $cyn=Join-Path $installDirectory 'Cyn.exe'
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
    $script:allowedToClose=$true
    $window.Close()
})
$window.Add_Closing({
    if ($script:phase -eq 'installing' -or $script:phase -eq 'closing') {
        $_.Cancel=$true
    } else {
        if ($worker -and !$worker.HasExited) { try { $worker.Kill() } catch {} }
    }
})

$clock=New-Object Windows.Threading.DispatcherTimer
$clock.Interval=[TimeSpan]::FromMilliseconds(140)
$clock.Add_Tick({
    if ($script:phase -eq 'closing') {
        $running=@(Get-CynSessionProcesses)
        if ($running.Count -eq 0) {
            try {
                $floor=Get-CynVersionFloor ([version]$CurrentVersion) @($installDirectory)
                if([version]$script:newVersion -lt $floor) { throw 'A newer version is already installed. This update was cancelled.' }
                $script:installProcess=Start-Process -FilePath $script:installerPath -Verb RunAs -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',('/DIR="'+$installDirectory+'"') -PassThru
                $script:phase='installing'
                Update-Label 'Installing Cyn' 'Windows is applying the verified update...'
                $progressText.Text='Installation in progress'
                $percentText.Text=''
            } catch {
                Show-Error ("Installer could not start: "+$_.Exception.Message)
            }
        } elseif (([datetime]::UtcNow - $script:exitRequestedAt).TotalSeconds -gt 25) {
            $script:stuckProcesses=@($running | Select-Object Id,StartTime,SessionId)
            $script:phase='stuck'
            Update-Label 'Cyn is still running' 'The audio app did not respond to Exit. You can close the stuck Cyn process here; unsaved settings may be lost.'
            $actionButton.Content='Close stuck Cyn & retry'; $actionButton.IsEnabled=$true
            $laterButton.Content='Cancel update'; $laterButton.Visibility='Visible'
        }
        return
    }
    if ($script:phase -eq 'installing') {
        if ($script:installProcess -and $script:installProcess.HasExited) {
            if ($script:installProcess.ExitCode -ne 0) {
                Show-Error ("Installer returned error "+$script:installProcess.ExitCode+". Your previous installation remains available.")
                return
            }
            try { Assert-CynInstalledVersion $installDirectory ([version]$script:newVersion) }
            catch { Show-Error $_.Exception.Message; return }
            try { Repair-CynUserLaunchers $installDirectory } catch { $script:launcherWarning=$_.Exception.Message }
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
                Assert-CynInstalledVersion $installDirectory ([version]$script:newVersion)
                Start-Process -FilePath (Join-Path $installDirectory 'Cyn.exe') -WorkingDirectory $installDirectory | Out-Null
            } catch { Show-Error ('Updated app could not launch: '+$_.Exception.Message); return }
            $script:allowedToClose=$true
            $window.Close()
        }
        return
    }
    if ($script:phase -in @('ready','error','current','stuck')) { return }
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

} finally { if($ownsMutex){$updateMutex.ReleaseMutex()}; $updateMutex.Dispose() }
