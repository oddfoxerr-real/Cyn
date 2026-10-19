param([switch]$DebugBuild, [switch]$Offline)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
# Use environment configuration first; this fallback is the verified toolchain on this PC.
$bundled = 'C:\Users\wiese\Documents\Codex\2026-10-05\design-m\work\android-build'
if (-not $env:JAVA_HOME -and (Test-Path "$bundled\jdk\jdk-17.0.20.1+1")) { $env:JAVA_HOME = "$bundled\jdk\jdk-17.0.20.1+1" }
if (-not $env:ANDROID_HOME -and (Test-Path "$bundled\sdk")) { $env:ANDROID_HOME = "$bundled\sdk" }
if (-not $env:JAVA_HOME -or -not $env:ANDROID_HOME) { throw 'Install JDK 17 and Android SDK platform 35/build tools 35.0.0, NDK 27.2.12479018 and CMake 3.22.1, then set JAVA_HOME and ANDROID_HOME.' }
if (-not $env:GRADLE_USER_HOME -and (Test-Path "$bundled\gradle-home")) { $env:GRADLE_USER_HOME = "$bundled\gradle-home" }
$env:ANDROID_USER_HOME = Join-Path $root '.gradle\android-user'
New-Item -ItemType Directory -Force -Path $env:ANDROID_USER_HOME | Out-Null
$env:ANDROID_SDK_ROOT = $env:ANDROID_HOME
$env:PATH = "$env:JAVA_HOME\bin;$env:PATH"
$tasks = @(':app:assembleDebug', ':app:lintDebug')
if (-not $DebugBuild) {
    $signing = Join-Path $root '.signing'
    New-Item -ItemType Directory -Force -Path $signing | Out-Null
    $key = Join-Path $signing 'cyn-android.jks'
    $secret = Join-Path $signing 'password.dpapi'
    if (-not (Test-Path $key) -or -not (Test-Path $secret)) {
        throw 'Restore the ORIGINAL .signing/cyn-android.jks and password.dpapi from the original Windows account. A new key cannot update existing CYN installs. Use -DebugBuild only for development.'
    }
    $secure = Get-Content -LiteralPath $secret | ConvertTo-SecureString
    $env:CYN_KEY_PASSWORD = [System.Net.NetworkCredential]::new('', $secure).Password
    $env:CYN_KEYSTORE = $key
    $tasks = @(':app:assembleRelease', ':app:lintRelease')
}
$gradle = Join-Path $root 'gradlew.bat'
if (Test-Path "$bundled\gradle\gradle-8.11.1\bin\gradle.bat") { $gradle = "$bundled\gradle\gradle-8.11.1\bin\gradle.bat" }
$options = @('--no-daemon', '--console=plain')
if ($Offline) { $options += '--offline' }
Push-Location $root
try {
    $modelArgs = @((Join-Path $root 'scripts\prepare_models.py'))
    if ($Offline) { $modelArgs += '--offline' }
    & python @modelArgs
    if ($LASTEXITCODE -ne 0) { throw 'Model preparation failed. Python 3.11+ and both verified desktop model files are required.' }
    & $gradle @tasks @options
    if ($LASTEXITCODE -ne 0) { throw "Android build failed ($LASTEXITCODE)." }
    if (-not $DebugBuild) {
        $apk = Join-Path $root 'app\build\outputs\apk\release\app-release.apk'
        $certificate = & "$env:ANDROID_HOME\build-tools\35.0.0\apksigner.bat" verify --print-certs $apk
        if ($LASTEXITCODE -ne 0 -or -not ($certificate -match 'certificate SHA-256 digest: 20698db6b762137a5e5199c0c5a6c728e8b90374d9dc76973c6ea21c005ee13d')) {
            throw 'Release signature does not match the original CYN Android app. Do not distribute this APK as an update.'
        }
    }
    $variant = if ($DebugBuild) { 'debug' } else { 'release' }
    Write-Output "APK: $root\app\build\outputs\apk\$variant\app-$variant.apk"
} finally {
    Pop-Location
    Remove-Item Env:CYN_KEY_PASSWORD -ErrorAction SilentlyContinue
    Remove-Item Env:CYN_KEYSTORE -ErrorAction SilentlyContinue
}
