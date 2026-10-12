param([string]$OutDir = "$PSScriptRoot\build")
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual Studio x86 MASM tools were not found.' }
$vc = Get-ChildItem -LiteralPath "$vs\VC\Tools\MSVC" -Directory | Sort-Object Name -Descending | Select-Object -First 1
$sdk = Get-ChildItem -LiteralPath "${env:ProgramFiles(x86)}\Windows Kits\10\Lib" -Directory | Sort-Object Name -Descending | Where-Object { Test-Path -LiteralPath "$($_.FullName)\um\x86\kernel32.lib" } | Select-Object -First 1
if (!$sdk) { throw 'Windows SDK x86 libraries were not found.' }
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
Push-Location $PSScriptRoot
try {
    & "$($vc.FullName)\bin\Hostx64\x86\ml.exe" /nologo /c /coff "/Fo$OutDir\video.obj" video.asm
    if ($LASTEXITCODE) { throw 'MASM failed.' }
    & "$($vc.FullName)\bin\Hostx64\x86\link.exe" /nologo /DLL /MACHINE:X86 /ENTRY:DllEntry@12 /NODEFAULTLIB /DYNAMICBASE /NXCOMPAT /SAFESEH:NO /OPT:REF /OPT:ICF /SECTION:.text,ERW /BASE:0x10000000 "/OUT:$OutDir\Video playback in BIK format.dll" "/MAP:$OutDir\video.map" "$OutDir\video.obj" "$($sdk.FullName)\um\x86\kernel32.lib" "$($sdk.FullName)\um\x86\user32.lib"
    if ($LASTEXITCODE) { throw 'Linker failed.' }
    Get-Item -LiteralPath "$OutDir\Video playback in BIK format.dll" | Select-Object FullName,Length
    Get-FileHash -Algorithm SHA256 -LiteralPath "$OutDir\Video playback in BIK format.dll"
} finally { Pop-Location }
