$ErrorActionPreference='Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $vc=Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
    New-Item -ItemType Directory -Force build-tests/lifecycle-obj | Out-Null
    $line='call "'+$vc+'" >NUL && cl /nologo /O2 /EHsc /std:c++17 /MD /D NDEBUG /D CYN_DJ_LIFECYCLE /D JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 /D JUCE_STANDALONE_APPLICATION=1 /D JUCE_USE_CURL=0 /I tests\dj /I third_party\JUCE\modules /I dsp\include /I fxsound\Source\GUI tests\cyn_dj_lifecycle.cpp third_party\JUCE\modules\juce_core\juce_core.cpp third_party\JUCE\modules\juce_cryptography\juce_cryptography.cpp /Fe:build-tests\cyn-dj-lifecycle.exe /Fo:build-tests\lifecycle-obj\'
    # Assertions must stay active for this test, including release compilation.
    $line=$line.Replace('/D NDEBUG ','')+' /link Shell32.lib Ole32.lib'
    & cmd.exe /d /c $line
    if($LASTEXITCODE -ne 0){throw 'Lifecycle compile failed'}
    Copy-Item .build-tools/yamnet-package/onnxruntime.dll build-tests -Force
    & ./build-tests/cyn-dj-lifecycle.exe "$PWD/.build-tools/dj/lifecycle-storage" "$PWD/.build-tools/dj/ast-audioset-int8.onnx"
    if($LASTEXITCODE -ne 0){throw 'Lifecycle tests failed'}
} finally {Pop-Location}
