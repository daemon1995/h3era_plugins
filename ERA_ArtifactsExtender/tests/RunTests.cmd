@echo off
setlocal
if not defined ARTIFACT_VS_PATH call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath > "%TEMP%\artifact-vs-path.txt"
if not defined ARTIFACT_VS_PATH set /p ARTIFACT_VS_PATH=<"%TEMP%\artifact-vs-path.txt"
if not defined ARTIFACT_VS_PATH exit /b 1
call "%ARTIFACT_VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
if errorlevel 1 exit /b 1
if not exist "%~dp0..\build\tests" mkdir "%~dp0..\build\tests"
pushd "%~dp0..\build\tests"
cl /nologo /std:c++17 /EHsc /MT /O1 /Gy /W3 /D_CRT_SECURE_NO_WARNINGS "%~dp0ArtifactTests.cpp" /Fe:artifact_tests.exe /link /OPT:REF /SUBSYSTEM:CONSOLE /BASE:0x10000000 user32.lib
if errorlevel 1 exit /b 1
artifact_tests.exe
set result=%errorlevel%
popd
exit /b %result%
