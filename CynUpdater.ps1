# Cyn Update Experience v2.0.6 — native WPF, real transfer progress and safe in-place install.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][ValidatePattern('^\d+\.\d+\.\d+$')][string]$CurrentVersion,
    [switch]$Interactive,
    [switch]$Probe,
    [switch]$SmokeTest,
    [ValidatePattern('^#[0-9a-fA-F]{6}$')][string]$AccentHex = '#91B7E2',
    [switch]$ReducedMotion
)
$ErrorActionPreference = 'Stop'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Add-Type -AssemblyName System.Net.Http
$repository = 'oddfoxerr-real/Cyn'
$script:window = $null
$script:cancelRequested = $false
$script:installing = $false
$script:updating = $false
$script:latest = $null
$script:network = [System.Net.Http.HttpClient]::new()
$script:network.Timeout = [TimeSpan]::FromMinutes(15)
$script:network.DefaultRequestHeaders.UserAgent.ParseAdd('Cyn-Updater/2.0')
$script:network.DefaultRequestHeaders.Accept.ParseAdd('application/vnd.github+json')

function Pump-CynDispatcher {
    if ($null -eq $script:window) { return }
    $frame = [System.Windows.Threading.DispatcherFrame]::new()
    $callback = [Action] { $frame.Continue = $false }
    [void]$script:window.Dispatcher.BeginInvoke(
        [System.Windows.Threading.DispatcherPriority]::Background, $callback)
    [System.Windows.Threading.Dispatcher]::PushFrame($frame)
}
function Await-CynTask($task) {
    while (-not $task.IsCompleted) {
        if ($script:cancelRequested -and -not $script:installing) {
            throw [OperationCanceledException]::new('Update cancelled. No installed files were changed.')
        }
        Pump-CynDispatcher
        [Threading.Thread]::Sleep(14)
    }
    return $task.GetAwaiter().GetResult()
}
function Get-CynUpdateRelease {
    $url = "https://api.github.com/repos/$repository/releases?per_page=50"
    $json = Await-CynTask ($script:network.GetStringAsync($url))
    $releases = ConvertFrom-Json -InputObject $json
    $candidates = @(
        foreach ($release in $releases) {
            if ($release.draft -or !$release.published_at -or $release.tag_name -notmatch '^v\d+\.\d+\.\d+$') { continue }
            $version = [version]$release.tag_name.Substring(1)
            if ($version -le [version]$CurrentVersion) { continue }
            $name = "Cyn-Setup-$($version.ToString(3))-x64.exe"
            $exe = @($release.assets | Where-Object { $_.name -ceq $name -and $_.state -eq 'uploaded' } | Select-Object -First 1)
            $sha = @($release.assets | Where-Object { $_.name -ceq "$name.sha256" -and $_.state -eq 'uploaded' } | Select-Object -First 1)
            if ($exe.Count -ne 1 -or $sha.Count -ne 1) { continue }
            $base = "https://github.com/$repository/releases/download/v$($version.ToString(3))/"
            if ($exe[0].browser_download_url -cne ($base+$name) -or $sha[0].browser_download_url -cne ($base+$name+'.sha256')) { continue }
            [pscustomobject]@{ Version=$version; Name=$name; Uri=$exe[0].browser_download_url; ShaUri=$sha[0].browser_download_url }
        }
    )
    return ($candidates | Sort-Object Version -Descending | Select-Object -First 1)
}
# Preserve the command-line regression probe used in CI. No GUI or install for probes.
if ($Probe) {
    $found = Get-CynUpdateRelease
    if ($null -eq $found) { throw "Updater probe found no newer published version after $CurrentVersion." }
    Write-Output $found.Version.ToString(3)
    $script:network.Dispose()
    return
}

# Automatic daily checks only show UI when there really is a new release.
if (-not $Interactive -and -not $SmokeTest) {
    try { $script:latest = Get-CynUpdateRelease }
    catch {
        $folder = Join-Path $env:LOCALAPPDATA 'Cyn'
        New-Item -Path $folder -ItemType Directory -Force | Out-Null
        Add-Content -LiteralPath (Join-Path $folder 'updater.log') -Value "$(Get-Date -Format o) $($_.Exception.Message)"
        $script:network.Dispose()
        exit 1
    }
    if ($null -eq $script:latest) { $script:network.Dispose(); exit 0 }
}

Add-Type -AssemblyName PresentationFramework, PresentationCore, WindowsBase
$xaml = @'
<Window xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        Title="Cyn Updates" Width="720" Height="518" MinWidth="720" MinHeight="518"
        ResizeMode="NoResize" WindowStartupLocation="CenterScreen" Background="#11151C"
        FontFamily="Segoe UI" Foreground="#F2F5FC" ShowInTaskbar="True">
    <Window.Resources>
        <Style TargetType="Button">
            <Setter Property="Foreground" Value="#FFFFFF"/>
            <Setter Property="FontSize" Value="13"/>
            <Setter Property="FontWeight" Value="SemiBold"/>
            <Setter Property="Cursor" Value="Hand"/>
            <Setter Property="Padding" Value="18,10"/>
            <Setter Property="BorderThickness" Value="0"/>
        </Style>
    </Window.Resources>
    <Grid Margin="22">
        <Border CornerRadius="20" Background="#191E28" BorderBrush="#3B4554" BorderThickness="1"/>
        <Grid Margin="27,20">
            <Grid.RowDefinitions>
                <RowDefinition Height="Auto"/>
                <RowDefinition Height="212"/>
                <RowDefinition Height="Auto"/>
                <RowDefinition Height="Auto"/>
                <RowDefinition Height="Auto"/>
                <RowDefinition Height="*"/>
            </Grid.RowDefinitions>
            <DockPanel Grid.Row="0">
                <TextBlock Text="CYN" FontSize="19" FontWeight="Bold" DockPanel.Dock="Left"/>
                <TextBlock Text="SYSTEM UPDATE" FontSize="11" Foreground="#B0BECE"
                           VerticalAlignment="Center" HorizontalAlignment="Right"/>
            </DockPanel>
            <Grid Grid.Row="1" Width="300" Height="204" HorizontalAlignment="Center">
                <Ellipse Width="177" Height="177" Opacity=".20">
                    <Ellipse.Fill>
                        <RadialGradientBrush>
                            <GradientStop Color="#8DBDFF" Offset="0"/>
                            <GradientStop Color="#191E28" Offset="1"/>
                        </RadialGradientBrush>
                    </Ellipse.Fill>
                </Ellipse>
                <Ellipse Width="168" Height="168" Stroke="#5C7391" StrokeThickness="1"
                         StrokeDashArray="4,7" Opacity=".72">
                    <Ellipse.RenderTransform>
                        <RotateTransform x:Name="RingOuter" CenterX="84" CenterY="84"/>
                    </Ellipse.RenderTransform>
                </Ellipse>
                <Ellipse Width="132" Height="132" Stroke="#9DBDE5" StrokeThickness="2"
                         StrokeDashArray="12,7" Opacity=".8">
                    <Ellipse.RenderTransform>
                        <RotateTransform x:Name="RingInner" CenterX="66" CenterY="66"/>
                    </Ellipse.RenderTransform>
                </Ellipse>
                <Ellipse x:Name="CoreGlow" Width="100" Height="100" Fill="#8DBDFF" Opacity=".14"/>
                <Border Width="72" Height="72" Background="#242E3E" CornerRadius="21"
                        BorderBrush="#8097B8" BorderThickness="1">
                    <Grid>
                        <Image x:Name="CynIcon" Width="50" Height="50" Stretch="Uniform"/>
                        <TextBlock x:Name="FallbackLogo" Text="C" FontSize="43" FontWeight="Bold"
                                   HorizontalAlignment="Center" VerticalAlignment="Center"/>
                    </Grid>
                </Border>
                <Canvas x:Name="Particles" Width="300" Height="204" IsHitTestVisible="False"/>
            </Grid>
            <TextBlock x:Name="StageTitle" Grid.Row="2" Text="Checking for updates"
                       FontSize="23" FontWeight="SemiBold" HorizontalAlignment="Center" Margin="0,0,0,8"/>
            <TextBlock x:Name="StageDescription" Grid.Row="3" Text="Connecting securely to GitHub Releases"
                       FontSize="12" Foreground="#B5C0D0" HorizontalAlignment="Center" TextAlignment="Center"
                       TextWrapping="Wrap" Margin="0,0,0,19"/>
            <StackPanel Grid.Row="4" Margin="4,0">
                <ProgressBar x:Name="Progress" Height="7" Minimum="0" Maximum="100"
                             Value="0" Background="#313A48" Foreground="#9DBDE5" BorderThickness="0"/>
                <DockPanel Margin="1,9,0,0">
                    <TextBlock x:Name="StageLabel" Text="CHECKING" FontSize="10"
                               Foreground="#A1B1C6" DockPanel.Dock="Left"/>
                    <TextBlock x:Name="Percentage" Text="" FontSize="11"
                               Foreground="#D9E4F4" HorizontalAlignment="Right"/>
                </DockPanel>
            </StackPanel>
            <DockPanel Grid.Row="5" VerticalAlignment="Bottom" LastChildFill="False" Margin="0,17,0,0">
                <TextBlock Text="Verified GitHub release · Existing preferences retained"
                           FontSize="10" Foreground="#97A8BB" VerticalAlignment="Center"
                           DockPanel.Dock="Left"/>
                <Button x:Name="CancelButton" Content="Close" MinWidth="96" Margin="8,0,0,0"
                        Background="#354051" DockPanel.Dock="Right"/>
                <Button x:Name="ActionButton" Content="Update now" MinWidth="126"
                        Background="#365C92" DockPanel.Dock="Right" Visibility="Collapsed"/>
            </DockPanel>
        </Grid>
    </Grid>
</Window>
'@
[xml]$xml = $xaml
$reader = [System.Xml.XmlNodeReader]::new($xml)
$script:window = [System.Windows.Markup.XamlReader]::Load($reader)
foreach ($name in @('RingOuter','RingInner','CoreGlow','CynIcon','FallbackLogo','Particles',
                    'StageTitle','StageDescription','Progress','StageLabel','Percentage','CancelButton','ActionButton')) {
    Set-Variable -Scope Script -Name $name -Value $script:window.FindName($name)
}
$accentBrush = [System.Windows.Media.BrushConverter]::new().ConvertFromString($AccentHex)
$Progress.Foreground = $accentBrush
$CoreGlow.Fill = $accentBrush
$ActionButton.Background = $accentBrush
$ActionButton.Foreground = [System.Windows.Media.Brushes]::Black
$logoPath = Join-Path $PSScriptRoot 'branding\cyn-icon.png'
if (Test-Path -LiteralPath $logoPath) {
    $bitmap = [System.Windows.Media.Imaging.BitmapImage]::new([Uri]::new($logoPath))
    $CynIcon.Source = $bitmap
    $FallbackLogo.Visibility = [System.Windows.Visibility]::Collapsed
}
if (-not $ReducedMotion) {
    foreach ($spec in @(@($RingOuter,8.0,360.0),@($RingInner,5.0,-360.0))) {
        $a = [System.Windows.Media.Animation.DoubleAnimation]::new()
        $a.From = 0.0; $a.To = [double]$spec[2]
        $a.Duration = [System.Windows.Duration]::new([TimeSpan]::FromSeconds([double]$spec[1]))
        $a.RepeatBehavior = [System.Windows.Media.Animation.RepeatBehavior]::Forever
        $spec[0].BeginAnimation([System.Windows.Media.RotateTransform]::AngleProperty, $a)
    }
    foreach ($i in 0..9) {
        $p = [System.Windows.Shapes.Ellipse]::new()
        $size = if ($i % 3 -eq 0) { 4.0 } else { 2.5 }
        $p.Width = $size; $p.Height = $size
        $p.Fill = $accentBrush; $p.Opacity = 0.7
        $theta = (2 * [Math]::PI * $i / 10.0)
        $x = 145 + [Math]::Cos($theta)*117
        $y = 100 + [Math]::Sin($theta)*82
        [System.Windows.Controls.Canvas]::SetLeft($p, $x)
        [System.Windows.Controls.Canvas]::SetTop($p, $y)
        $move = [System.Windows.Media.TranslateTransform]::new()
        $p.RenderTransform = $move
        [void]$Particles.Children.Add($p)
        $dx = [System.Windows.Media.Animation.DoubleAnimation]::new()
        $dx.From=0.0; $dx.To=(145.0-$x)*0.82
        $dx.Duration=[System.Windows.Duration]::new([TimeSpan]::FromSeconds(2.1+$i*0.07))
        $dx.RepeatBehavior=[System.Windows.Media.Animation.RepeatBehavior]::Forever
        $move.BeginAnimation([System.Windows.Media.TranslateTransform]::XProperty, $dx)
        $dy = [System.Windows.Media.Animation.DoubleAnimation]::new()
        $dy.From=0.0; $dy.To=(100.0-$y)*0.82
        $dy.Duration=$dx.Duration
        $dy.RepeatBehavior=$dx.RepeatBehavior
        $move.BeginAnimation([System.Windows.Media.TranslateTransform]::YProperty, $dy)
    }
}
function Set-CynStage([string]$title,[string]$description,[string]$stage,[double]$percent) {
    $StageTitle.Text=$title
    $StageDescription.Text=$description
    $StageLabel.Text=$stage.ToUpperInvariant()
    $Progress.Value=[Math]::Min(100,[Math]::Max(0,$percent))
    $Percentage.Text = $(if ($percent -gt 0) { "$([int]$percent)%" } else { '' })
}
function Get-CynDownloadedInstaller($release) {
    $version = $release.Version.ToString(3)
    $folder = Join-Path $env:LOCALAPPDATA "Cyn\Updates\v$version"
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $destination = Join-Path $folder $release.Name
    Set-CynStage 'Verifying release' 'Reading the published SHA-256 checksum' 'verifying' 2
    $checksumText = Await-CynTask ($script:network.GetStringAsync($release.ShaUri))
    $expected = (($checksumText.Trim()) -split '\s+')[0]
    if ($expected -notmatch '^[a-fA-F0-9]{64}$') { throw 'Invalid release checksum.' }
    if (Test-Path -LiteralPath $destination) {
        if ((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -ieq $expected) { return $destination }
        Remove-Item -LiteralPath $destination -Force
    }
    $partial = "$destination.part"
    $download = Await-CynTask ($script:network.GetAsync($release.Uri,
        [System.Net.Http.HttpCompletionOption]::ResponseHeadersRead))
    try {
        $download.EnsureSuccessStatusCode() | Out-Null
        $total = -1L
        if ($null -ne $download.Content.Headers.ContentLength) { $total = [long]$download.Content.Headers.ContentLength }
        if ($total -gt 350MB) { throw 'Installer exceeds size limit.' }
        $stream = Await-CynTask ($download.Content.ReadAsStreamAsync())
        $out = [IO.File]::Open($partial,[IO.FileMode]::Create,[IO.FileAccess]::Write,[IO.FileShare]::None)
        $readTotal = [long]0
        try {
            $buffer = [byte[]]::new(131072)
            while ($true) {
                $read = Await-CynTask ($stream.ReadAsync($buffer,0,$buffer.Length))
                if ($read -eq 0) { break }
                $out.Write($buffer,0,[int]$read)
                $readTotal += [long]$read
                if ($readTotal -gt 350MB) { throw 'Download exceeded size limit.' }
                $fraction = if ($total -gt 0) { $readTotal/[double]$total } else { 0 }
                $pct = [Math]::Min(80.0, [Math]::Round(5.0 + $fraction*75.0,1))
                $mb = [Math]::Round($readTotal/1MB,1)
                $desc = if ($total -gt 0) { "$mb / $([Math]::Round($total/1MB,1)) MB downloaded" }
                        else { "$mb MB downloaded" }
                Set-CynStage 'Downloading update' $desc 'downloading' $pct
                Pump-CynDispatcher
            }
        } finally { $out.Dispose(); $stream.Dispose() }
        if ($total -gt 0 -and $readTotal -ne $total) { throw 'Incomplete installer download.' }
    } finally { $download.Dispose() }
    Set-CynStage 'Verifying download' 'Checking all installer bytes before installation' 'verifying' 84
    if ((Get-FileHash -LiteralPath $partial -Algorithm SHA256).Hash -ine $expected) {
        Remove-Item -LiteralPath $partial -Force -ErrorAction SilentlyContinue
        throw 'Installer checksum mismatch. No changes were made to Cyn.'
    }
    Move-Item -LiteralPath $partial -Destination $destination -Force
    return $destination
}
# Update-only IPC: private Cyn message window created by FxController.
# No forced process termination or killing unrelated audio apps.
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class CynUpdateWin32 {
    [DllImport("user32.dll", CharSet=CharSet.Unicode)]
    public static extern IntPtr FindWindow(string cls, string title);
    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr wnd, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr wnd, out uint pid);
}
'@
function Stop-CynForUpdate {
    Set-CynStage 'Preparing installation' 'Saving Cyn preferences and shutting down audio safely' 'preparing' 89
    $wnd = [CynUpdateWin32]::FindWindow($null,'CynHotkeys')
    if ($wnd -eq [IntPtr]::Zero) { return }
    [uint32]$processId = 0
    [void][CynUpdateWin32]::GetWindowThreadProcessId($wnd,[ref]$processId)
    if (-not [CynUpdateWin32]::PostMessage($wnd,[uint32]0x84D2,[IntPtr]::Zero,[IntPtr]::Zero)) {
        throw 'Cyn did not accept the update shutdown request.'
    }
    $until = [DateTime]::UtcNow.AddSeconds(25)
    while ([DateTime]::UtcNow -lt $until) {
        if (-not (Get-Process -Id $processId -ErrorAction SilentlyContinue)) { return }
        Pump-CynDispatcher
        [Threading.Thread]::Sleep(90)
    }
    throw 'Cyn did not exit in time. Installation was not started.'
}
function Install-CynUpdate($installer) {
    $script:installing = $true
    $CancelButton.IsEnabled = $false
    Set-CynStage 'Installing Cyn' 'Windows may request one administrator approval' 'installing' 92
    Stop-CynForUpdate
    Set-CynStage 'Installing Cyn' 'Applying the verified update automatically' 'installing' 93
    $arguments = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-')
    $process = Start-Process -FilePath $installer -Verb RunAs -ArgumentList $arguments -PassThru
    while (-not $process.HasExited) {
        Pump-CynDispatcher
        [Threading.Thread]::Sleep(95)
        $process.Refresh()
    }
    if ($process.ExitCode -ne 0) { throw "Installer returned code $($process.ExitCode)." }
    Set-CynStage 'Update complete' 'Cyn has been updated. Starting the new version…' 'finished' 100
    $CynIcon.Opacity=1.0
    $path = Join-Path $PSScriptRoot 'Cyn.exe'
    if (-not (Test-Path -LiteralPath $path)) {
        $path = Join-Path $env:ProgramFiles 'Cyn\Cyn.exe'
    }
    if (!(Test-Path -LiteralPath $path)) { throw 'Installation completed but Cyn.exe was not found.' }
    [void](Start-Process -FilePath $path -ArgumentList '--view=2' -PassThru)
    $script:updating = $false
    $CancelButton.Content = 'Done'
    $CancelButton.IsEnabled = $true
    $script:installing = $false
}
function Start-CynUpdate {
    if ($script:updating -or $null -eq $script:latest) { return }
    $script:updating = $true
    $script:cancelRequested = $false
    $ActionButton.Visibility=[System.Windows.Visibility]::Collapsed
    $CancelButton.Content='Cancel download'
    try {
        $installer = Get-CynDownloadedInstaller $script:latest
        if ($script:cancelRequested) { throw 'Download cancelled.' }
        Install-CynUpdate $installer
    } catch {
        $script:installing = $false
        $script:updating = $false
        $CancelButton.IsEnabled=$true
        $CancelButton.Content='Close'
        Set-CynStage 'Update interrupted' $_.Exception.Message 'needs attention' 0
    }
}
$ActionButton.Add_Click({ Start-CynUpdate })
$CancelButton.Add_Click({
    if ($script:installing) { return }
    if ($script:updating) { $script:cancelRequested = $true }
    else { $script:window.Close() }
})
$script:window.Add_Closing({
    param($sender,$e)
    if ($script:installing) { $e.Cancel=$true }
    elseif ($script:updating) { $script:cancelRequested=$true; $e.Cancel=$true }
})
$script:window.Add_ContentRendered({
    if ($SmokeTest) {
        Set-CynStage 'Smoke test passed' 'WPF theme and animation elements initialized' 'ready' 100
        $script:window.Close()
        return
    }
    try {
        Set-CynStage 'Checking for updates' 'Contacting the official Cyn GitHub releases' 'checking' 0
        if ($null -eq $script:latest) { $script:latest=Get-CynUpdateRelease }
        if ($null -eq $script:latest) {
            Set-CynStage 'Cyn is up to date' "Version $CurrentVersion is already current." 'ready' 100
            $CancelButton.Content='Done'
            return
        }
        Set-CynStage "Cyn $($script:latest.Version.ToString(3)) is ready" 'One click handles download, verification, installation and relaunch.' 'ready' 0
        $ActionButton.Visibility=[System.Windows.Visibility]::Visible
    } catch {
        Set-CynStage 'Could not check for updates' $_.Exception.Message 'connection error' 0
    }
})
try { [void]$script:window.ShowDialog() }
finally {
    $script:network.Dispose()
}
