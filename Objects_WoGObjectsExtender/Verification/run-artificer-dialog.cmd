@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 exit /b 1
set "artificer_ui_out=%TEMP%\H3EraArtificerDialogTests"
if not exist "%artificer_ui_out%\Objects" mkdir "%artificer_ui_out%\Objects"
if not exist "%artificer_ui_out%\headers\EraPluginsAPI" mkdir "%artificer_ui_out%\headers\EraPluginsAPI"
for %%f in (ArtificerDialog.cpp ArtificerDialog.h Artificer.h ArtificerLogic.h) do copy /y "%~dp0..\%%f" "%artificer_ui_out%\Objects\%%f" >nul
copy /y "%~dp0ArtificerDialogStubs.h" "%artificer_ui_out%\Objects\framework.h" >nul
copy /y "%~dp0..\..\headers\EraPluginsAPI\ArtifactDescriptionApi.h" "%artificer_ui_out%\headers\EraPluginsAPI\ArtifactDescriptionApi.h" >nul
cl /nologo /std:c++14 /EHsc /MT /W3 /D_CRT_SECURE_NO_WARNINGS /I"%artificer_ui_out%\Objects" "%~dp0ArtificerDialog.tests.cpp" /Fo"%artificer_ui_out%\\" /Fe"%artificer_ui_out%\ArtificerDialogTests.exe"
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -Command "& (Join-Path $env:artificer_ui_out 'ArtificerDialogTests.exe'); exit $LASTEXITCODE"
exit /b %errorlevel%
