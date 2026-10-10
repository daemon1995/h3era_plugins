@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
  echo Run from an x86 Visual Studio Developer Command Prompt.
  exit /b 1
)
pushd "%~dp0"
set "replay_test_out=..\build\Combat_BattleReplay\checks"
if not exist "%replay_test_out%" mkdir "%replay_test_out%"
cl /nologo /W4 /WX /EHa /std:c++14 /MTd /Od /Zi BattleState.tests.cpp /Fo"%replay_test_out%\BattleState.tests.obj" /Fd"%replay_test_out%\BattleState.tests.pdb" /Fe"%replay_test_out%\BattleState.tests.exe"
if errorlevel 1 goto failed
"%replay_test_out%\BattleState.tests.exe"
if errorlevel 1 goto failed
popd
exit /b 0
:failed
popd
exit /b 1
