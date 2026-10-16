[CmdletBinding()]
param([string]$CompilerPath, [switch]$RebuildApp)
$ErrorActionPreference = 'Stop'
$payload = Join-Path $PSScriptRoot 'dist\Cyn'
if ($RebuildApp -or !(Test-Path (Join-Path $payload 'Cyn.exe'))) {
    & (Join-Path $PSScriptRoot 'BUILD_CYN.ps1')
}
if (!(Test-Path (Join-Path $payload 'Cyn.exe'))) { throw 'Cyn build is missing.' }
if (!$CompilerPath) {
    $toolRoot = Join-Path $PSScriptRoot '.build-tools\InnoSetup'
    $CompilerPath = Join-Path $toolRoot 'ISCC.exe'
    if (!(Test-Path -LiteralPath $CompilerPath)) {
        New-Item -ItemType Directory -Path $toolRoot -Force | Out-Null
        $download = Join-Path $PSScriptRoot '.build-tools\innosetup-6.7.3.exe'
        Invoke-WebRequest -Uri 'https://github.com/jrsoftware/issrc/releases/download/is-6_7_3/innosetup-6.7.3.exe' -OutFile $download
        $signature = Get-AuthenticodeSignature -LiteralPath $download
        if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notlike '*Pyrsys B.V.*') {
            throw 'Inno Setup compiler download failed signature validation.'
        }
        $process = Start-Process -FilePath $download -WindowStyle Hidden -Wait -PassThru -ArgumentList @(
            '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/CURRENTUSER',
            '/NOICONS', '/TASKS=', '/PORTABLE=1', ('/DIR="' + $toolRoot + '"'))
        if ($process.ExitCode -ne 0) { throw "Compiler setup failed: $($process.ExitCode)" }
    }
}
if (!(Test-Path -LiteralPath $CompilerPath)) { throw 'ISCC.exe was not found.' }
$manifest = Get-Content (Join-Path $PSScriptRoot 'Installer\Cyn\driver-sha256.json') -Raw | ConvertFrom-Json
foreach ($file in $manifest) {
    $path = Join-Path $PSScriptRoot $file.path
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $file.sha256) {
        throw "Bundled upstream driver file changed: $($file.path)"
    }
}
& $CompilerPath ('/DPayloadDir=' + $payload) ('/DOutputDir=' + (Join-Path $PSScriptRoot 'dist')) (Join-Path $PSScriptRoot 'Installer\Cyn\Cyn.iss')
if ($LASTEXITCODE -ne 0) { throw "Installer compilation failed: $LASTEXITCODE" }
$installer = Join-Path $PSScriptRoot 'dist\Cyn-Setup-2.0.37-x64.exe'
$hash = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText("$installer.sha256", $hash + [Environment]::NewLine, (New-Object Text.UTF8Encoding($false)))
Write-Host "SHA-256: $hash"
Write-Host "Installer ready: $installer"
