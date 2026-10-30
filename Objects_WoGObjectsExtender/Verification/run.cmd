@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
  echo Run from an x86 Visual Studio Developer Command Prompt.
  exit /b 1
)
set "wog_object_test_out=%TEMP%\H3EraWoGObjectTests"
if not exist "%wog_object_test_out%" mkdir "%wog_object_test_out%"
copy /y "%~dp0..\WoGObjectHero.h" "%wog_object_test_out%\WoGObjectHero.h" >nul
copy /y "%~dp0..\MarketOfTime.cpp" "%wog_object_test_out%\MarketOfTime.cpp" >nul
copy /y "%~dp0..\MarketOfTime.h" "%wog_object_test_out%\MarketOfTime.h" >nul
copy /y "%~dp0..\MarketOfTimeLogic.h" "%wog_object_test_out%\MarketOfTimeLogic.h" >nul
copy /y "%~dp0..\Artificer.h" "%wog_object_test_out%\Artificer.h" >nul
copy /y "%~dp0..\ArtificerLogic.h" "%wog_object_test_out%\ArtificerLogic.h" >nul
copy /y "%~dp0..\WoGObjectsExtender.cpp" "%wog_object_test_out%\WoGObjectsExtender.cpp" >nul
copy /y "%~dp0..\WoGObjectsExtender.h" "%wog_object_test_out%\WoGObjectsExtender.h" >nul
copy /y "%~dp0NativeStubs.h" "%wog_object_test_out%\framework.h" >nul
copy /y "%~dp0ObjectExtenderStubs.h" "%wog_object_test_out%\ObjectExtenderStubs.h" >nul
for %%f in (WoGObjectPolicy.cpp WoGObjectPolicy.h WoGObjectHandlers.h WoGObjectIds.h WoGObjectIds.cpp) do copy /y "%~dp0..\%%f" "%wog_object_test_out%\%%f" >nul
copy /y "%~dp0WoGTableStubs.h" "%wog_object_test_out%\WoGTableStubs.h" >nul
cl /nologo /std:c++14 /EHsc /MT /W3 /D_CRT_SECURE_NO_WARNINGS /I"%wog_object_test_out%" "%~dp0MarketOfTime.tests.cpp" /Fo"%wog_object_test_out%\\" /Fe"%wog_object_test_out%\MarketOfTimeTests.exe"
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -Command "& (Join-Path $env:wog_object_test_out 'MarketOfTimeTests.exe'); exit $LASTEXITCODE"
exit /b %errorlevel%
