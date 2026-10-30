@echo off
setlocal
set "potion_dialog_out=%TEMP%\H3EraPotionDialogTests"
if not exist "%potion_dialog_out%" mkdir "%potion_dialog_out%"
for %%f in (WoGObjectPotionDialog.cpp WoGObjectPotionDialog.h WoGObjectUtils.h) do copy /y "%~dp0..\%%f" "%potion_dialog_out%\%%f" >nul
copy /y "%~dp0ArtificerDialogStubs.h" "%potion_dialog_out%\framework.h" >nul
cl /nologo /std:c++14 /EHsc /MT /W3 /I"%potion_dialog_out%" "%~dp0PotionDialog.tests.cpp" /Fo"%potion_dialog_out%\\" /Fe"%potion_dialog_out%\PotionDialogTests.exe"
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -Command "& (Join-Path $env:potion_dialog_out 'PotionDialogTests.exe'); exit $LASTEXITCODE"
exit /b %errorlevel%
