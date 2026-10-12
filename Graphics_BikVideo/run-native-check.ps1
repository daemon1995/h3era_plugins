$ErrorActionPreference='Stop'
$vs=& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath
$vc=Get-ChildItem "$vs\VC\Tools\MSVC" -Directory | Sort-Object Name -Descending | Select-Object -First 1
$sdk=Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\Lib" -Directory | Sort-Object Name -Descending | Select-Object -First 1
$sdkInclude="${env:ProgramFiles(x86)}\Windows Kits\10\Include\$($sdk.Name)"
Push-Location $PSScriptRoot
try {
    & "$($vc.FullName)\bin\Hostx64\x86\cl.exe" /nologo /EHsc /MT /O1 "/I$($vc.FullName)\include" "/I$sdkInclude\um" "/I$sdkInclude\shared" "/I$sdkInclude\ucrt" /Ibuild native_smoke.cpp /Fobuild\native_smoke.obj /Febuild\native_smoke.exe /link "/LIBPATH:$($vc.FullName)\lib\x86" "/LIBPATH:$($sdk.FullName)\um\x86" "/LIBPATH:$($sdk.FullName)\ucrt\x86"
    if ($LASTEXITCODE) { throw 'Native smoke test build failed' }
    & '.\build\native_smoke.exe' "$PSScriptRoot\build\Video playback in BIK format.dll" 'D:\games\HoMM 3 ERA\era.dll' 'D:\games\HoMM 3 ERA\Mods\H3HDVideo_menu-Winter-Summer-ENG-RU\binkw32a.dll' "$PSScriptRoot\build\test_video.bik"
    if ($LASTEXITCODE) { throw "Native smoke test failed: $LASTEXITCODE" }
} finally { Pop-Location }
