# Stage the verified Google YAMNet ONNX and official Windows ONNX Runtime.
# Only used for native YAMNet-enabled releases; files do not run inside audio DSP.
[CmdletBinding()]
param([string]$Destination)
$ErrorActionPreference = 'Stop'
if (!$Destination) { $Destination=Join-Path $PSScriptRoot '.build-tools\yamnet-package' }
New-Item -ItemType Directory -Path $Destination -Force | Out-Null
$onnx=Join-Path $Destination 'yamnet-16k-0p975s.onnx'
$dll=Join-Path $Destination 'onnxruntime.dll'
$expectedModel='d3c173d26202dd94bf5532795a0f6e3090610a70adb2a773faa0ee635ec6acbe'
$expectedRuntime='14119125df2dcf9ff3e083afdba5fcc4b09b4186d8762404eb7b1fbccde3fcf2'
if (!(Test-Path $onnx) -or !(Test-Path $dll)) {
    # The original verified Actions artifact expires. Preserve the exact same
    # bytes using the permanent, checksum-pinned 2.0.21 release instead.
    $archive=Join-Path $Destination 'Cyn-runtime-2.0.21.zip'
    if (!(Test-Path $archive)) {
        Invoke-WebRequest -Uri 'https://github.com/oddfoxerr-real/Cyn/releases/download/v2.0.21/Cyn-Windows-x64.zip' -OutFile $archive
    }
    if ((Get-FileHash $archive -Algorithm SHA256).Hash -ine '4307d0a5568748d04955bae0ead47a5e0e6c937332c6a7927dbec3d01968f0f2') { throw 'Pinned runtime archive checksum mismatch.' }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=[IO.Compression.ZipFile]::OpenRead($archive)
    try {
        foreach($name in @('onnxruntime.dll','yamnet-16k-0p975s.onnx')) {
            $entries=@($zip.Entries | Where-Object { $_.Name -ceq $name })
            if($entries.Count -ne 1){throw "Missing or ambiguous runtime file: $name"}
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entries[0],(Join-Path $Destination $name),$true)
        }
    } finally {$zip.Dispose()}
}
if ((Get-FileHash $onnx -Algorithm SHA256).Hash -ine $expectedModel) {throw 'YAMNet model checksum mismatch.'}
if ((Get-FileHash $dll -Algorithm SHA256).Hash -ine $expectedRuntime) {throw 'ONNX Runtime checksum mismatch.'}
$manifest=@{
    yamnet_sha256=(Get-FileHash $onnx -Algorithm SHA256).Hash.ToLowerInvariant()
    onnxruntime_sha256=(Get-FileHash $dll -Algorithm SHA256).Hash.ToLowerInvariant()
    model_origin='Official Google TF Hub YAMNet, converted & verified in Actions run 38138969013'
    runtime_origin='Microsoft.ML.OnnxRuntime 1.19.2 official NuGet'
}
$manifest | ConvertTo-Json | Set-Content -Encoding UTF8 -Path (Join-Path $Destination 'yamnet-manifest.json')
Write-Host 'Validated and staged YAMNet model and ONNX Runtime x64.'
