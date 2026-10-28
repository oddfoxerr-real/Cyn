$ErrorActionPreference = 'Stop'
# Riff CYN is the single desktop interface. It starts and controls the native
# FxSound DSP host directly; do not open the old CYN/JUCE window first.
$choices = @(
    (Join-Path $PSScriptRoot 'CYN-Interface\CYN Audio.exe'),
    (Join-Path $PSScriptRoot 'dist\Cyn\CYN-Interface\CYN Audio.exe')
)
$interface = $choices | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
if (-not $interface) {
    throw 'CYN Riff interface is missing. Build the full combined Windows application or repair the installation.'
}
$engine = Join-Path (Split-Path -Parent (Split-Path -Parent $interface)) 'Cyn.exe'
if (-not (Test-Path -LiteralPath $engine -PathType Leaf)) {
    throw 'Native FxSound host is missing. Repair the complete CYN installation.'
}
Start-Process -FilePath $interface -WorkingDirectory (Split-Path -Parent $interface)
