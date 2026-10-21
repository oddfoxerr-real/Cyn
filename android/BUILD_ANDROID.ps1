param([switch]$DebugBuild, [switch]$Offline, [string]$SigningDirectory = (Join-Path $PSScriptRoot '.signing'))
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
    $signing = $SigningDirectory
    New-Item -ItemType Directory -Force -Path $signing | Out-Null
    $key = Join-Path $signing 'cyn-studio.jks'
    $secret = Join-Path $signing 'password.dpapi'
    if (-not (Test-Path $key) -or -not (Test-Path $secret)) {
        throw 'Restore cyn-studio.jks and password.dpapi from the Studio signing backup; use -SigningDirectory. A replacement key cannot update Studio installs. Use -DebugBuild only for development.'
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
        if ($LASTEXITCODE -ne 0 -or -not ($certificate -match 'certificate SHA-256 digest: 045d9c122b2158d569c0ef8d10bbb08496a9f19e7397b702e99e9330c780bcb7')) {
            throw 'Release signature does not match the registered CYN Studio Android app. Do not distribute this APK as an update.'
        }
    }
    $variant = if ($DebugBuild) { 'debug' } else { 'release' }
    Write-Output "APK: $root\app\build\outputs\apk\$variant\app-$variant.apk"
} finally {
    Pop-Location
    Remove-Item Env:CYN_KEY_PASSWORD -ErrorAction SilentlyContinue
    Remove-Item Env:CYN_KEYSTORE -ErrorAction SilentlyContinue
}
