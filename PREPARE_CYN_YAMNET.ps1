# Stage the verified Google YAMNet ONNX and official Windows ONNX Runtime.
# Only used for native YAMNet-enabled releases; files do not run inside audio DSP.
[CmdletBinding()]
param([string]$Destination)
$ErrorActionPreference = 'Stop'
if (!$Destination) { $Destination=Join-Path $PSScriptRoot '.build-tools\yamnet-package' }
New-Item -ItemType Directory -Path $Destination -Force | Out-Null
$onnx=Join-Path $Destination 'yamnet-16k-0p975s.onnx'
$report=Join-Path $Destination 'conversion-evaluation.json'
if (!(Test-Path $onnx) -or !(Test-Path $report)) {
    $gh=Get-Command gh -ErrorAction SilentlyContinue
    if (!$gh) { throw 'GitHub CLI is required to fetch the tested YAMNet artifact. Install gh, authenticate, or pre-stage model and report.' }
    & gh run download 38138969013 --repo oddfoxerr-real/Cyn --name yamnet-verified-onnx-cpu --dir $Destination
    if ($LASTEXITCODE -ne 0) { throw 'Could not download validated YAMNet ONNX artifact.' }
}
if (!(Test-Path $onnx) -or !(Test-Path $report)) { throw 'Verified YAMNet artifact incomplete.' }
$tested=Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
if ($tested.scores -ne 521 -or $tested.embedding_dimensions -ne 1024 -or
    $tested.waveform_samples -ne 15600 -or
    $tested.model -ne 'https://tfhub.dev/google/yamnet/1') {
    throw 'YAMNet model test report did not match the application input contract.'
}
if ((Get-Item $onnx).Length -lt 1000000) { throw 'YAMNet ONNX model is unexpectedly small.' }
$dll=Join-Path $Destination 'onnxruntime.dll'
if (!(Test-Path $dll)) {
    $package=Join-Path $Destination 'Microsoft.ML.OnnxRuntime.1.19.2.nupkg'
    $packageZip=Join-Path $Destination 'Microsoft.ML.OnnxRuntime.1.19.2.zip'
    $unpack=Join-Path $Destination 'nuget-runtime'
    if (!(Test-Path $packageZip)) {
        Invoke-WebRequest -Uri 'https://api.nuget.org/v3-flatcontainer/microsoft.ml.onnxruntime/1.19.2/microsoft.ml.onnxruntime.1.19.2.nupkg' -OutFile $package
        Copy-Item -LiteralPath $package -Destination $packageZip -Force
    }
    Expand-Archive -LiteralPath $packageZip -DestinationPath $unpack -Force
    $candidate=Join-Path $unpack 'runtimes/win-x64/native/onnxruntime.dll'
    if (!(Test-Path $candidate)) { throw 'Official NuGet package lacks the expected Windows x64 ONNX DLL.' }
    Copy-Item -LiteralPath $candidate -Destination $dll -Force
}
if ((Get-Item $dll).Length -lt 100000) { throw 'ONNX Runtime x64 DLL is unexpectedly small.' }
$manifest=@{
    yamnet_sha256=(Get-FileHash $onnx -Algorithm SHA256).Hash.ToLowerInvariant()
    onnxruntime_sha256=(Get-FileHash $dll -Algorithm SHA256).Hash.ToLowerInvariant()
    model_origin='Official Google TF Hub YAMNet, converted & verified in Actions run 38138969013'
    runtime_origin='Microsoft.ML.OnnxRuntime 1.19.2 official NuGet'
}
$manifest | ConvertTo-Json | Set-Content -Encoding UTF8 -Path (Join-Path $Destination 'yamnet-manifest.json')
Write-Host 'Validated and staged YAMNet model and ONNX Runtime x64.'
