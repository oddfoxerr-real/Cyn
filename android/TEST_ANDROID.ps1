$ErrorActionPreference = 'Stop'
if (-not $env:JAVA_HOME) { $env:JAVA_HOME = 'C:\Users\wiese\Documents\Codex\2026-10-05\design-m\work\android-build\jdk\jdk-17.0.20.1+1' }
$out = Join-Path $PSScriptRoot 'app\build\host-tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$src = Join-Path $PSScriptRoot 'app\src\main\java\io\github\oddfoxerr\cyn'
& "$env:JAVA_HOME\bin\javac.exe" -encoding UTF-8 -d $out "$src\PreferenceModel.java" "$src\AudioFeatures.java" "$src\AndroidRelease.java" "$PSScriptRoot\tests\AudioModelTest.java"
if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed.' }
& "$env:JAVA_HOME\bin\java.exe" -cp $out io.github.oddfoxerr.cyn.AudioModelTest
if ($LASTEXITCODE -ne 0) { throw 'Audio model test failed.' }
