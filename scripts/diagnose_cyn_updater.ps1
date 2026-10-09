$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$headers = @{'User-Agent'='Cyn-Updater-Diagnostic';'Accept'='application/vnd.github+json'}
$releases = @(Invoke-RestMethod -Uri 'https://api.github.com/repos/oddfoxerr-real/Cyn/releases?per_page=50' -Headers $headers -TimeoutSec 25)
Write-Host ('Outer collection count: ' + $releases.Count)
foreach ($release in $releases) {
  Write-Host ('Release iteration type: ' + $release.GetType().FullName)
  Write-Host ('Release tag: ' + ($release.tag_name | Out-String))
  if ($release.draft -or !$release.published_at -or $release.tag_name -notmatch '^v\d+\.\d+\.\d+$') {
    Write-Host 'SKIPPED: draft/date/tag condition'; continue
  }
  $version = [version]$release.tag_name.Substring(1)
  if ($version -le [version]'2.0.2') { Write-Host 'SKIPPED: version'; continue }
  $name = "Cyn-Setup-$($version.ToString(3))-x64.exe"
  $installer = @($release.assets | Where-Object {$_.name -ceq $name -and $_.state -eq 'uploaded'} | Select-Object -First 1)
  $checksum = @($release.assets | Where-Object {$_.name -ceq "$name.sha256" -and $_.state -eq 'uploaded'} | Select-Object -First 1)
  Write-Host ('Candidate: '+$name+' installer='+$installer.Count+' checksum='+$checksum.Count)
}
