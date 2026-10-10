@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
  echo Run from an x86 Visual Studio Developer Command Prompt.
  exit /b 1
)
pushd "%~dp0"
set "battlesave_test_out=..\build\ERA_SystemOptionsExtension\checks"
if not exist "%battlesave_test_out%" mkdir "%battlesave_test_out%"
cl /nologo /EHsc /std:c++17 /MT /O1 /Gy /DNOMINMAX BattleSave.tests.cpp /Fo"%battlesave_test_out%\BattleSave.tests.obj" /Fe"%battlesave_test_out%\BattleSave.tests.exe" /link /OPT:REF /OPT:NOICF
if errorlevel 1 goto failed
"%battlesave_test_out%\BattleSave.tests.exe"
if errorlevel 1 goto failed
popd
exit /b 0
:failed
popd
exit /b 1
