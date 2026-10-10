# Cyn update discovery/downloader. Runs separately from the WPF UI to keep animation responsive.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][ValidatePattern('^\d+\.\d+\.\d+$')][string]$CurrentVersion,
    [string]$StatusFile,
    [string]$CacheDirectory,
    [switch]$Probe
)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$repo = 'oddfoxerr-real/Cyn'
$script:lastState = @{}
$script:lastWrite = [datetime]::MinValue

function Send-State([string]$phase, [string]$message, [int]$percent=0, [long]$bytes=0, [long]$total=0, [string]$version='', [string]$installer='') {
    if ($Probe -or [string]::IsNullOrWhiteSpace($StatusFile)) { return }
    $state = @{
        phase=$phase; message=$message; percent=[math]::Max(0,[math]::Min(100,$percent))
        bytes=$bytes; total=$total; version=$version; installer=$installer
    }
    $json = ConvertTo-Json -InputObject $state -Compress
    $partial = "$StatusFile.tmp"
    [System.IO.File]::WriteAllText($partial,$json,[System.Text.Encoding]::UTF8)
    # The UI tolerates short reads while the state file is being replaced.
    Move-Item -LiteralPath $partial -Destination $StatusFile -Force
}

try {
    . (Join-Path $PSScriptRoot 'CynUpdatePolicy.ps1')
    $CurrentVersion=(Get-CynVersionFloor ([version]$CurrentVersion) (@($PSScriptRoot)+@(Get-CynRegisteredDirectories))).ToString(3)
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Send-State 'checking' 'Checking GitHub releases...'
    $headers=@{'User-Agent'='Cyn-Updater';'Accept'='application/vnd.github+json'}
    # Do not wrap Invoke-RestMethod in @(...). In Windows PowerShell 5.1
    # this creates a nested array and treats all releases as one object.
    $releases = Invoke-RestMethod -Uri "https://api.github.com/repos/$repo/releases?per_page=50" -Headers $headers -TimeoutSec 25
    $available=@(
        foreach($release in $releases) {
            if ($release.draft -or !$release.published_at -or $release.tag_name -notmatch '^v\d+\.\d+\.\d+$') { continue }
            $version=[version]$release.tag_name.Substring(1)
            if ($version -le [version]$CurrentVersion) { continue }
            $name="Cyn-Setup-$($version.ToString(3))-x64.exe"
            $exe=@($release.assets | Where-Object { $_.name -ceq $name -and $_.state -eq 'uploaded' } | Select-Object -First 1)
            $sum=@($release.assets | Where-Object { $_.name -ceq "$name.sha256" -and $_.state -eq 'uploaded' } | Select-Object -First 1)
            if (!$exe.Count -or !$sum.Count) { continue }
            $expected="https://github.com/$repo/releases/download/v$($version.ToString(3))/"
            if ($exe[0].browser_download_url -cne ($expected+$name) -or
                $sum[0].browser_download_url -cne ($expected+$name+'.sha256')) { continue }
            [pscustomobject]@{Version=$version;Exe=$exe[0];Sha=$sum[0]}
        }
    )
    $newest=$available | Sort-Object Version -Descending | Select-Object -First 1
    if ($Probe) {
        if (!$newest) { throw "Updater regression: No newer release for $CurrentVersion" }
        Write-Output $newest.Version.ToString(3)
        exit 0
    }
    if (!$newest) { Send-State 'current' "Cyn $CurrentVersion is already up to date." 100; exit 0 }
    if ([string]::IsNullOrWhiteSpace($CacheDirectory)) { throw 'Missing update cache directory.' }
    New-Item -ItemType Directory -Path $CacheDirectory -Force | Out-Null
    $version=$newest.Version.ToString(3)
    $name="Cyn-Setup-$version-x64.exe"
    $installer=Join-Path $CacheDirectory $name
    $shaPath=Join-Path $CacheDirectory "$name.sha256"
    Send-State 'downloading' "Downloading Cyn $version" 0 0 0 $version
    Invoke-WebRequest -Uri $newest.Sha.browser_download_url -OutFile $shaPath -UseBasicParsing -Headers @{'User-Agent'='Cyn-Updater'} -TimeoutSec 40
    $shaText=(Get-Content -LiteralPath $shaPath -Raw).Trim()
    $expectedHash=($shaText -split '\s+')[0]
    if ($expectedHash -notmatch '^[a-fA-F0-9]{64}$') { throw 'GitHub Release provided an invalid SHA-256 checksum.' }
    if (Test-Path -LiteralPath $installer) {
        if ((Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash -ieq $expectedHash) {
            Send-State 'ready' "Cyn $version is verified and ready to install." 100 0 0 $version $installer
            exit 0
        }
        Remove-Item -LiteralPath $installer -Force
    }
    $tmp="$installer.part"
    $request=[Net.HttpWebRequest]::Create($newest.Exe.browser_download_url)
    $request.UserAgent='Cyn-Updater'
    $request.Timeout=30000
    $request.ReadWriteTimeout=45000
    $response=$request.GetResponse()
    try {
        $total=[long]$response.ContentLength
        if ($total -gt 536870912) { throw 'Release installer exceeds 512 MB limit.' }
        $inputStream=$response.GetResponseStream()
        $outputStream=[System.IO.File]::Open($tmp,[System.IO.FileMode]::Create,[System.IO.FileAccess]::Write,[System.IO.FileShare]::None)
        try {
            $buffer=New-Object byte[] 65536
            $received=[long]0
            $last=[datetime]::UtcNow.AddSeconds(-1)
            while (($count=$inputStream.Read($buffer,0,$buffer.Length)) -gt 0) {
                $outputStream.Write($buffer,0,$count)
                $received+=$count
                if ($received -gt 536870912) { throw 'Release installer exceeds 512 MB limit.' }
                if (([datetime]::UtcNow-$last).TotalMilliseconds -ge 160) {
                    $pct=if($total -gt 0){[int][math]::Round($received*100.0/$total)}else{0}
                    Send-State 'downloading' "Downloading Cyn $version" $pct $received $total $version
                    $last=[datetime]::UtcNow
                }
            }
            if ($total -gt 0 -and $received -ne $total) { throw 'Download ended before all bytes arrived.' }
        } finally { $outputStream.Dispose(); $inputStream.Dispose() }
    } finally { $response.Dispose() }
    Send-State 'verifying' 'Verifying SHA-256 signature...' 100 0 0 $version
    $actual=(Get-FileHash -LiteralPath $tmp -Algorithm SHA256).Hash
    if ($actual -ine $expectedHash) { throw 'Downloaded installer failed SHA-256 verification.' }
    Move-Item -LiteralPath $tmp -Destination $installer -Force
    Send-State 'ready' "Cyn $version is ready to install." 100 0 0 $version $installer
} catch {
    $reason=$_.Exception.Message
    try { Send-State 'error' $reason } catch {}
    Write-Error $reason
    exit 1
}
