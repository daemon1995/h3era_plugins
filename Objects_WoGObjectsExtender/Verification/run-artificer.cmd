@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 exit /b 1
set "artificer_test_out=%TEMP%\H3EraArtificerTests"
if not exist "%artificer_test_out%" mkdir "%artificer_test_out%"
for %%f in (Artificer.cpp Artificer.h ArtificerDialog.h ArtificerLogic.h WoGObjectResources.h WoGObjectsExtender.cpp WoGObjectsExtender.h MarketOfTime.h MarketOfTimeLogic.h) do copy /y "%~dp0..\%%f" "%artificer_test_out%\%%f" >nul
copy /y "%~dp0ArtificerNativeStubs.h" "%artificer_test_out%\framework.h" >nul
for %%f in (WoGObjectPolicy.cpp WoGObjectPolicy.h WoGObjectHandlers.h WoGObjectIds.h WoGObjectIds.cpp) do copy /y "%~dp0..\%%f" "%artificer_test_out%\%%f" >nul
copy /y "%~dp0WoGTableStubs.h" "%artificer_test_out%\WoGTableStubs.h" >nul
cl /nologo /std:c++14 /EHsc /MT /W3 /D_CRT_SECURE_NO_WARNINGS /I"%artificer_test_out%" "%~dp0Artificer.tests.cpp" /Fo"%artificer_test_out%\\" /Fe"%artificer_test_out%\ArtificerTests.exe"
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -Command "& (Join-Path $env:artificer_test_out 'ArtificerTests.exe'); exit $LASTEXITCODE"
exit /b %errorlevel%
