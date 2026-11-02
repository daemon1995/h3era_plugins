@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
  echo Run from an x86 Visual Studio Developer Command Prompt.
  exit /b 1
)
set "dwelling_test_out=%TEMP%\H3EraDwellingTests"
if not exist "%dwelling_test_out%" mkdir "%dwelling_test_out%"
pushd "%~dp0"
cl /nologo /std:c++14 /EHsc /MT /W3 /D_CRT_SECURE_NO_WARNINGS DwellingSetupTests.cpp /Fo"%dwelling_test_out%\\" /Fe"%dwelling_test_out%\DwellingValidation.exe"
if errorlevel 1 goto failed
"%dwelling_test_out%\DwellingValidation.exe"
if errorlevel 1 goto failed
cl /nologo /std:c++14 /EHsc /MT /W3 DwellingInfoTests.cpp /Fo"%dwelling_test_out%\\" /Fe"%dwelling_test_out%\DwellingInfo.exe"
if errorlevel 1 goto failed
"%dwelling_test_out%\DwellingInfo.exe"
if errorlevel 1 goto failed
popd
exit /b 0
:failed
popd
exit /b 1
