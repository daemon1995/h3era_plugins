@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
  echo Run this file from a Visual Studio Developer Command Prompt.
  exit /b 1
)
if not exist "%~dp0build" mkdir "%~dp0build"
cl /nologo /EHsc /W4 /std:c++14 /Fe:"%~dp0build\RuntimeState.tests.exe" /Fo:"%~dp0build\RuntimeState.tests.obj" "%~dp0RuntimeState.tests.cpp"
if errorlevel 1 exit /b 1
"%~dp0build\RuntimeState.tests.exe"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /W4 /std:c++14 /Fe:"%~dp0build\JsonConfig.tests.exe" /Fo:"%~dp0build\JsonConfig.tests.obj" "%~dp0Verification\JsonConfig.tests.cpp"
if errorlevel 1 exit /b 1
"%~dp0build\JsonConfig.tests.exe"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /W4 /O2 /std:c++14 /Fe:"%~dp0build\ObjectGeometry.tests.exe" /Fo:"%~dp0build\ObjectGeometry.tests.obj" "%~dp0Verification\ObjectGeometry.tests.cpp"
if errorlevel 1 exit /b 1
"%~dp0build\ObjectGeometry.tests.exe"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /W4 /O2 /std:c++14 /Fe:"%~dp0build\ObjectReplacementAllocation.tests.exe" /Fo:"%~dp0build\ObjectReplacementAllocation.tests.obj" "%~dp0Verification\ObjectReplacementAllocation.tests.cpp"
if errorlevel 1 exit /b 1
"%~dp0build\ObjectReplacementAllocation.tests.exe"
if errorlevel 1 exit /b 1
if "%~1"=="" exit /b 0
cl /nologo /EHsc /W4 /std:c++14 /Fe:"%~dp0build\HdMinimapLayout.tests.exe" /Fo:"%~dp0build\HdMinimapLayout.tests.obj" "%~dp0Verification\HdMinimapLayout.tests.cpp"
if errorlevel 1 exit /b 1
"%~dp0build\HdMinimapLayout.tests.exe" "%~1"
exit /b %errorlevel%
