$ErrorActionPreference='Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $vc=Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
    New-Item -ItemType Directory -Force build-tests/spotify-obj | Out-Null
    $line='call "'+$vc+'" >NUL && cl /nologo /O2 /EHsc /std:c++17 /MD /D JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 /D JUCE_STANDALONE_APPLICATION=1 /D JUCE_USE_CURL=0 /I tests\dj /I third_party\JUCE\modules /I fxsound\Source\GUI tests\cyn_spotify_test.cpp third_party\JUCE\modules\juce_core\juce_core.cpp third_party\JUCE\modules\juce_cryptography\juce_cryptography.cpp /Fe:build-tests\cyn-spotify-test.exe /Fo:build-tests\spotify-obj\ /link Shell32.lib Ole32.lib'
    & cmd.exe /d /c $line
    if($LASTEXITCODE -ne 0){throw 'Spotify tests failed to compile'}
    & ./build-tests/cyn-spotify-test.exe "$PWD/build-tests/spotify-fixture"
    if($LASTEXITCODE -ne 0){throw 'Spotify tests failed'}
} finally {Pop-Location}
