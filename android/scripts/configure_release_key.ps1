# Run on the Windows account that created the registered CYN Studio Android signing key.
# Uploads only to the CYN repository's encrypted Actions secrets. Never prints the key.
param(
    [string]$SigningDirectory = (Join-Path $PSScriptRoot '..\.signing'),
    [switch]$Publish
)
$ErrorActionPreference = 'Stop'
if ($env:OS -ne 'Windows_NT') { throw 'Use the Windows account holding the Studio signing backup: its DPAPI password cannot be decrypted on another system.' }
$repo = 'oddfoxerr-real/Cyn'
$key = Join-Path $SigningDirectory 'cyn-studio.jks'
$secret = Join-Path $SigningDirectory 'password.dpapi'
if (-not (Test-Path -LiteralPath $key) -or -not (Test-Path -LiteralPath $secret)) {
    throw 'Registered cyn-studio.jks or password.dpapi is missing. Restore the Studio signing backup folder or pass -SigningDirectory. Do not create a replacement key.'
}
$gh = (Get-Command gh -ErrorAction Stop).Source
$keytool = if ($env:JAVA_HOME) { Join-Path $env:JAVA_HOME 'bin\keytool.exe' } else { (Get-Command keytool -ErrorAction Stop).Source }
& $gh auth status --hostname github.com
if ($LASTEXITCODE -ne 0) { throw 'Sign in with gh auth login using the account that manages oddfoxerr-real/Cyn.' }
$previousPassword = $env:CYN_KEY_PASSWORD
try {
    $secure = Get-Content -LiteralPath $secret | ConvertTo-SecureString
    $password = [System.Net.NetworkCredential]::new('', $secure).Password
    $env:CYN_KEY_PASSWORD = $password
    $certificate = & $keytool '-J-Duser.language=en' -list -v -keystore $key -alias cyn-studio -storepass:env CYN_KEY_PASSWORD
    if ($LASTEXITCODE -ne 0) { throw 'Cannot open the registered Studio signing key with this Windows account.' }
    $match = [regex]::Match(($certificate | Out-String), 'SHA256:\s*([0-9A-Fa-f:]{95})')
    $fingerprint = $match.Groups[1].Value.Replace(':','').ToLowerInvariant()
    if ($fingerprint -ne '045d9c122b2158d569c0ef8d10bbb08496a9f19e7397b702e99e9330c780bcb7') {
        throw 'This is not the registered CYN Studio Android signing certificate. Nothing was uploaded.'
    }
    $encoded = [Convert]::ToBase64String([IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $key)))
    $encoded | & $gh secret set CYN_STUDIO_ANDROID_KEYSTORE_BASE64 --repo $repo
    if ($LASTEXITCODE -ne 0) { throw 'Could not save the keystore secret.' }
    $password | & $gh secret set CYN_STUDIO_ANDROID_KEY_PASSWORD --repo $repo
    if ($LASTEXITCODE -ne 0) { throw 'Could not save the password secret. Rerun this script to finish both secrets.' }
    Write-Output 'Studio certificate verified. Android release signing is configured.'
    if ($Publish) {
        & $gh workflow run android-release.yml --ref codex/android --repo $repo
        if ($LASTEXITCODE -ne 0) { throw 'Signing is configured, but the workflow did not start. Rerun the Android release workflow from GitHub Actions.' }
        Write-Output 'Build, device checks and signed publication requested. See https://github.com/oddfoxerr-real/Cyn/actions/workflows/android-release.yml'
    }
} finally {
    $password = $encoded = $secure = $null
    if ($null -eq $previousPassword) { Remove-Item Env:CYN_KEY_PASSWORD -ErrorAction SilentlyContinue }
    else { $env:CYN_KEY_PASSWORD = $previousPassword }
}
