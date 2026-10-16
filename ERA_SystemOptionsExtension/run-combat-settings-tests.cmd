@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
  echo Run from an x86 Visual Studio Developer Command Prompt.
  exit /b 1
)
pushd "%~dp0"
set "combat_settings_test_out=..\build\ERA_SystemOptionsExtension\checks"
if not exist "%combat_settings_test_out%" mkdir "%combat_settings_test_out%"
cl /nologo /EHsc /std:c++17 /MT /O1 /Gy /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS CombatSettings.tests.cpp /Fo"%combat_settings_test_out%\CombatSettings.tests.obj" /Fe"%combat_settings_test_out%\CombatSettings.tests.exe" /link /OPT:REF /OPT:NOICF
if errorlevel 1 goto failed
"%combat_settings_test_out%\CombatSettings.tests.exe" %*
if errorlevel 1 goto failed
popd
exit /b 0
:failed
popd
exit /b 1
