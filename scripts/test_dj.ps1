$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    New-Item -ItemType Directory -Force build-tests | Out-Null
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $vc=Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
    $line='call "'+$vc+'" >NUL && cl /nologo /O2 /EHsc /std:c++17 /I dsp\include /I fxsound\Source\GUI tests\cyn_dj_test.cpp /Fe:build-tests\cyn-dj-test.exe /Fo:build-tests\cyn-dj-test.obj && build-tests\cyn-dj-test.exe'
    & cmd.exe /d /c $line
    if($LASTEXITCODE -ne 0){throw 'Native AI DJ tests failed'}
    $remix='call "'+$vc+'" >NUL && cl /nologo /O2 /EHsc /std:c++17 /I dsp\include tests\cyn_dj_remix_test.cpp /Fe:build-tests\cyn-dj-remix-test.exe /Fo:build-tests\cyn-dj-remix-test.obj && build-tests\cyn-dj-remix-test.exe'
    & cmd.exe /d /c $remix
    if($LASTEXITCODE -ne 0){throw 'Native song remix tests failed'}
} finally {Pop-Location}
