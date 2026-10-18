# Stage the official Microsoft Visual C++ x64 redistributable for CYN's ONNX runtime.
# Download occurs on the build machine only; user PCs need no internet access.
# Authenticode signature validation prevents shipping an untrusted executable.
[CmdletBinding()]
param([string]$Destination = (Join-Path $PSScriptRoot '.build-tools\vc-redist-x64'))
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Path $Destination -Force | Out-Null
$exe = Join-Path $Destination 'VC_redist.x64.exe'
if (-not (Test-Path -LiteralPath $exe)) {
    $tmp = Join-Path $Destination 'VC_redist.x64.exe.download'
    try {
        Invoke-WebRequest -Uri 'https://aka.ms/vs/17/release/vc_redist.x64.exe' -OutFile $tmp
        Move-Item -LiteralPath $tmp -Destination $exe -Force
    } finally {
        Remove-Item -LiteralPath $tmp -ErrorAction SilentlyContinue
    }
}
$item = Get-Item -LiteralPath $exe
if ($item.Length -lt 10000000 -or $item.Length -gt 100000000) {
    Remove-Item -LiteralPath $exe -Force
    throw 'Microsoft Visual C++ x64 redistributable size was unexpected.'
}
$signature = Get-AuthenticodeSignature -LiteralPath $exe
if ($signature.Status -ne 'Valid' -or
    $null -eq $signature.SignerCertificate -or
    $signature.SignerCertificate.Subject -notmatch 'CN=Microsoft Corporation') {
    Remove-Item -LiteralPath $exe -Force
    throw 'Microsoft VC++ x64 installer is not Authenticode signed by Microsoft.'
}
Write-Host "Verified Microsoft-signed Visual C++ x64 runtime: $($item.Length) bytes"
