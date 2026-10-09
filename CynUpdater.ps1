[CmdletBinding()]
param([Parameter(Mandatory=$true)][ValidatePattern('^\d+\.\d+\.\d+$')][string]$CurrentVersion,[switch]$Interactive)
$ErrorActionPreference = 'Stop'
$repository = 'oddfoxerr-real/Cyn'
function Show-CynDialog([string]$message,[string]$buttons='OK',[string]$icon='Information') {
  Add-Type -AssemblyName System.Windows.Forms
  [System.Windows.Forms.MessageBox]::Show($message,'Cyn Updates',[System.Windows.Forms.MessageBoxButtons]::$buttons,[System.Windows.Forms.MessageBoxIcon]::$icon)
}
try {
  [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
  $headers = @{'User-Agent'='Cyn-Updater';'Accept'='application/vnd.github+json'}
  $releases = @(Invoke-RestMethod -Uri "https://api.github.com/repos/$repository/releases?per_page=50" -Headers $headers -TimeoutSec 25)
  $available = @(
    foreach($release in $releases) {
      if ($release.draft -or !$release.published_at -or $release.tag_name -notmatch '^v\d+\.\d+\.\d+$') {continue}
      $version = [version]$release.tag_name.Substring(1)
      if ($version -le [version]$CurrentVersion) {continue}
      $name = "Cyn-Setup-$($version.ToString(3))-x64.exe"
      $installer = @($release.assets | Where-Object {$_.name -ceq $name -and $_.state -eq 'uploaded'} | Select-Object -First 1)
      $checksum = @($release.assets | Where-Object {$_.name -ceq "$name.sha256" -and $_.state -eq 'uploaded'} | Select-Object -First 1)
      if (!$installer.Count -or !$checksum.Count) {continue}
      [pscustomobject]@{Version=$version;Installer=$installer[0];Checksum=$checksum[0]}
    }
  )
  $latest = $available | Sort-Object Version -Descending | Select-Object -First 1
  if (!$latest) {
    if ($Interactive) {[void](Show-CynDialog "Cyn $CurrentVersion is up to date.")}
    exit 0
  }
  $message = "Cyn $($latest.Version.ToString(3)) is available!" + [Environment]::NewLine + [Environment]::NewLine + "Download and install? Exit Cyn using the tray icon when setup asks."
  if ((Show-CynDialog $message 'YesNo' 'Question') -ne [System.Windows.Forms.DialogResult]::Yes) {exit 0}
  $tag = "v$($latest.Version.ToString(3))"
  $name = "Cyn-Setup-$($latest.Version.ToString(3))-x64.exe"
  $base = "https://github.com/$repository/releases/download/$tag/"
  if ($latest.Installer.browser_download_url -cne ($base+$name) -or $latest.Checksum.browser_download_url -cne ($base+$name+'.sha256')) {throw 'Unexpected release asset URL.'}
  $dir = Join-Path ([IO.Path]::GetTempPath()) ("Cyn-Update-"+[guid]::NewGuid().ToString('N'))
  New-Item -ItemType Directory -Path $dir | Out-Null
  $exe = Join-Path $dir $name
  $sha = "$exe.sha256"
  Invoke-WebRequest -Uri ($base+$name+'.sha256') -OutFile $sha -UseBasicParsing -Headers @{'User-Agent'='Cyn-Updater'}
  $expected = ((Get-Content -LiteralPath $sha -Raw).Trim() -split '\s+')[0]
  if ($expected -notmatch '^[a-fA-F0-9]{64}$') {throw 'Invalid release checksum.'}
  Invoke-WebRequest -Uri ($base+$name) -OutFile $exe -UseBasicParsing -Headers @{'User-Agent'='Cyn-Updater'}
  if ((Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash -ine $expected) {throw 'Installer SHA-256 mismatch.'}
  Start-Process -FilePath $exe -Verb RunAs | Out-Null
  [void](Show-CynDialog 'Verified installer launched. Exit Cyn from its tray icon to finish setup.')
} catch {
  if ($Interactive) {[void](Show-CynDialog ("Update check failed: "+$_.Exception.Message) 'OK' 'Warning')}
  else {
    try {
      $log = Join-Path $env:LOCALAPPDATA 'Cyn\updater.log'
      New-Item -ItemType Directory -Force -Path (Split-Path $log) | Out-Null
      Add-Content -LiteralPath $log -Value ((Get-Date -Format o)+" "+$_.Exception.Message)
    } catch {}
  }
  exit 1
}
