# Stage ONLY the official APEX ONNX checkpoint from its model release.
# No training, arbitrary code execution or silent user-side downloads.
[CmdletBinding()]
param([string]$Destination)
$ErrorActionPreference = 'Stop'
if (!$Destination) { $Destination=Join-Path $PSScriptRoot '.build-tools\apex-package' }
New-Item -Path $Destination -ItemType Directory -Force | Out-Null
$model=Join-Path $Destination 'apex6m-225.onnx'
$manifestPath=Join-Path $Destination 'apex6m-manifest.json'
$base='https://github.com/oddfoxerr-real/Cyn/releases/download/apex-6m-v1.0.0/'
if (!(Test-Path -LiteralPath $manifestPath)) {
    Invoke-WebRequest -Uri ($base+'apex6m-manifest.json') -OutFile $manifestPath
}
$manifest=Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.parameter_count -ne 6307477 -or $manifest.checkpoint_training_steps -ne 225 -or
    $manifest.new_training_steps -ne 0 -or
    $manifest.sha256 -notmatch '^[a-fA-F0-9]{64}$') {
    throw 'Unexpected APEX manifest; refusing native activation.'
}
if (!(Test-Path -LiteralPath $model)) {
    Invoke-WebRequest -Uri ($base+'apex6m-225.onnx') -OutFile $model
}
$hash=(Get-FileHash -LiteralPath $model -Algorithm SHA256).Hash
if ($hash -ine $manifest.sha256) {
    Remove-Item -LiteralPath $model -Force -ErrorAction SilentlyContinue
    throw 'APEX full ONNX model SHA-256 mismatch.'
}
if ((Get-Item $model).Length -lt 20000000) { throw 'The full 6.3M APEX model is missing.' }
Write-Host 'Verified APEX 6,307,477 parameter ONNX model, existing 225-step weights. No training.'
