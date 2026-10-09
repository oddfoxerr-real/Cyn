$ErrorActionPreference = 'Stop'
$updater = Join-Path $PSScriptRoot '..\CynUpdater.ps1'
$tokens = $null
$parseErrors = $null
[void][System.Management.Automation.Language.Parser]::ParseFile(
  (Resolve-Path $updater).Path, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count -gt 0) {
  throw ('PowerShell updater syntax errors: ' + (($parseErrors | ForEach-Object { $_.Message }) -join '; '))
}
Write-Host 'Updater PowerShell syntax parsed successfully.'
$exe = "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe"
$arguments = '-NoProfile -NonInteractive -STA -ExecutionPolicy Bypass -File "' + (Resolve-Path $updater).Path + '" -CurrentVersion 2.0.6 -SmokeTest'
$process = Start-Process -FilePath $exe -ArgumentList $arguments -PassThru
if (-not $process.WaitForExit(30000)) {
  Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
  throw 'Cyn updater visual smoke test exceeded 30 seconds.'
}
if ($process.ExitCode -ne 0) {
  throw "Cyn updater visual smoke test exited with code $($process.ExitCode)."
}
Write-Host 'Updater WPF construction, theme assets, and animation initialization passed.'
