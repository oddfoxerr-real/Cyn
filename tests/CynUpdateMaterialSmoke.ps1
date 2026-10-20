# CYN Material-inspired updater layout smoke test.
# Validates with the actual Windows WPF parser; no network, audio, or UI shown.
$ErrorActionPreference='Stop'
Add-Type -AssemblyName PresentationFramework,PresentationCore,WindowsBase
$path=Join-Path (Split-Path $PSScriptRoot -Parent) 'CynUpdateWindow.xaml'
[xml]$markup=Get-Content -LiteralPath $path -Raw
$reader=New-Object System.Xml.XmlNodeReader($markup)
$window=[Windows.Markup.XamlReader]::Load($reader)
foreach($name in @(
  'StageTitle','StageDetail','DownloadProgress','ProgressText','PercentText',
  'ActionButton','LaterButton','CurrentVersionText','LatestVersionText',
  'DragSurface','StepCheck','StepDownload','StepVerify','StepInstall','StatusPulse'
)){
  if($null -eq $window.FindName($name)){throw "Updater XAML missing binding: $name"}
}
if($window.Width -lt 620 -or $window.Height -lt 420){throw 'Updater window layout unexpectedly small'}
if($window.WindowStyle -ne [System.Windows.WindowStyle]::None){throw 'Unexpected WPF window chrome'}
$window.Close()
Write-Host 'PASS: WPF Material update screen loads and all updater bindings resolve.'
