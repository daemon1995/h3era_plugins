@echo off
setlocal
set "cave_test_out=%TEMP%\H3EraCaveTests"
if not exist "%cave_test_out%" mkdir "%cave_test_out%"
cl /nologo /std:c++14 /EHsc /MT /W3 "%~dp0CaveState.tests.cpp" /Fo"%cave_test_out%\\" /Fe"%cave_test_out%\CaveStateTests.exe"
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -Command "& (Join-Path $env:cave_test_out 'CaveStateTests.exe'); exit $LASTEXITCODE"
exit /b %errorlevel%
