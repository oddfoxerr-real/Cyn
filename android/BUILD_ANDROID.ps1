param([switch]$DebugBuild, [switch]$Offline)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
# Use environment configuration first; this fallback is the verified toolchain on this PC.
$bundled = 'C:\Users\wiese\Documents\Codex\2026-10-05\design-m\work\android-build'
if (-not $env:JAVA_HOME -and (Test-Path "$bundled\jdk\jdk-17.0.20.1+1")) { $env:JAVA_HOME = "$bundled\jdk\jdk-17.0.20.1+1" }
if (-not $env:ANDROID_HOME -and (Test-Path "$bundled\sdk")) { $env:ANDROID_HOME = "$bundled\sdk" }
if (-not $env:JAVA_HOME -or -not $env:ANDROID_HOME) { throw 'Install JDK 17 and Android SDK platform 35/build tools 35.0.0, then set JAVA_HOME and ANDROID_HOME.' }
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
    if (-not (Test-Path $secret)) {
        if (Test-Path $key) { throw 'Signing key exists but its encrypted password is missing. Restore the password backup; do not replace the key.' }
        $bytes = New-Object byte[] 32
        [Security.Cryptography.RandomNumberGenerator]::Create().GetBytes($bytes)
        $password = [Convert]::ToBase64String($bytes)
        ConvertTo-SecureString $password -AsPlainText -Force | ConvertFrom-SecureString | Set-Content -LiteralPath $secret
    }
    $secure = Get-Content -LiteralPath $secret | ConvertTo-SecureString
    $env:CYN_KEY_PASSWORD = [System.Net.NetworkCredential]::new('', $secure).Password
    $env:CYN_KEYSTORE = $key
    if (-not (Test-Path $key)) {
        & "$env:JAVA_HOME\bin\keytool.exe" -genkeypair -keystore $key -storetype JKS -alias cyn-android -keyalg RSA -keysize 3072 -validity 10000 -dname 'CN=Cyn Android, OU=Independent App, O=Cyn' -storepass:env CYN_KEY_PASSWORD -keypass:env CYN_KEY_PASSWORD
        if ($LASTEXITCODE -ne 0) { throw 'Signing key creation failed.' }
    }
    $tasks = @(':app:assembleRelease', ':app:lintRelease')
}
$gradle = Join-Path $root 'gradlew.bat'
if (Test-Path "$bundled\gradle\gradle-8.11.1\bin\gradle.bat") { $gradle = "$bundled\gradle\gradle-8.11.1\bin\gradle.bat" }
$options = @('--no-daemon', '--console=plain')
if ($Offline) { $options += '--offline' }
Push-Location $root
try {
    & $gradle @tasks @options
    if ($LASTEXITCODE -ne 0) { throw "Android build failed ($LASTEXITCODE)." }
    $variant = if ($DebugBuild) { 'debug' } else { 'release' }
    Write-Output "APK: $root\app\build\outputs\apk\$variant\app-$variant.apk"
} finally {
    Pop-Location
    Remove-Item Env:CYN_KEY_PASSWORD -ErrorAction SilentlyContinue
    Remove-Item Env:CYN_KEYSTORE -ErrorAction SilentlyContinue
}
