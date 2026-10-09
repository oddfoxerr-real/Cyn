[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
Add-Type -AssemblyName PresentationFramework,PresentationCore,WindowsBase
$root=Split-Path $PSScriptRoot -Parent
$path=Join-Path $root 'CynUpdateWindow.xaml'
[xml]$markup=Get-Content -LiteralPath $path -Raw
$reader=New-Object System.Xml.XmlNodeReader($markup)
$window=[System.Windows.Markup.XamlReader]::Load($reader)
foreach ($id in @('StageTitle','StageDetail','DownloadProgress','ProgressText','PercentText','LaterButton','ActionButton',
                 'OrbitA','OrbitB','CorePulse','ParticleA','ParticleB','ParticleC','ParticleD')) {
    if (!$window.FindName($id)) { throw "Required Cyn updater animation element '$id' not found." }
}
if ($window.Resources['CynMotion'] -eq $null) { throw 'Missing particle/orbit animation storyboard.' }
$window.Close()
Write-Host 'PASS: Update window loads, all progress controls, particles and rings found.'
