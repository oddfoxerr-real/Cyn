[CmdletBinding()]
param([string]$Python='python', [int]$Epochs=300)
$ErrorActionPreference='Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual Studio C++ tools required' }
New-Item -ItemType Directory -Force build-tests,training/generated/balance | Out-Null
$envScript=Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
$command='call "'+$envScript+'" >NUL && cl /nologo /O2 /EHsc /std:c++17 /I dsp\include training\GenerateBalanceData.cpp /Fo:build-tests\GenerateBalanceData.obj /Fe:build-tests\GenerateBalanceData.exe'
cmd.exe /D /C $command
if ($LASTEXITCODE -ne 0) { throw 'Feature generator compilation failed' }
foreach ($split in @(@('train',18031,6000),@('validation',81047,1500),@('test',99371,1500))) {
    & .\build-tests\GenerateBalanceData.exe "training/generated/balance/$($split[0]).csv" $split[1] $split[2]
    if ($LASTEXITCODE -ne 0) { throw 'Dataset generation failed' }
}
& $Python training/train_balance.py --epochs $Epochs
if ($LASTEXITCODE -ne 0) { throw 'Training/evaluation gate failed' }
$command='call "'+$envScript+'" >NUL && cl /nologo /O2 /EHsc /std:c++17 /I dsp\include /I fxsound\Source\GUI tests\cyn_balance_test.cpp /Fo:build-tests\cyn_balance_test.obj /Fe:build-tests\cyn_balance_test.exe && build-tests\cyn_balance_test.exe tests\data\balance-parity.csv'
cmd.exe /D /C $command
if ($LASTEXITCODE -ne 0) { throw 'Native model parity / behavior tests failed' }
