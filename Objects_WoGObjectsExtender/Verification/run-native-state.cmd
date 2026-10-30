@echo off
setlocal
set "native_state_out=%TEMP%\H3EraNativeStateTests"
if not exist "%native_state_out%" mkdir "%native_state_out%"
for %%f in (WoGObjectSetup.h WoGObjectState.cpp WoGObjectState.h) do copy /y "%~dp0..\%%f" "%native_state_out%\%%f" >nul
copy /y "%~dp0NativeStateStubs.h" "%native_state_out%\framework.h" >nul
cl /nologo /std:c++14 /EHsc /MT /W3 /I"%native_state_out%" "%~dp0NativeState.tests.cpp" /Fo"%native_state_out%\\" /Fe"%native_state_out%\NativeStateTests.exe"
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -Command "& (Join-Path $env:native_state_out 'NativeStateTests.exe'); exit $LASTEXITCODE"
exit /b %errorlevel%
