param(
    [string]$GamePath = 'D:\games\HoMM 3 ERA',
    [string]$ModName = 'WoG',
    [string]$ArtifactsRoot = '',
    [switch]$Build
)
$ErrorActionPreference = 'Stop'
if (Get-Process -Name 'h3era','h3era HD','h3wmaped','h3wmaped_unleashed' -ErrorAction SilentlyContinue) {
    throw 'Close the game and map editor before installing.'
}
$mods = [IO.Path]::GetFullPath((Join-Path $GamePath 'Mods')).TrimEnd('\')
if (!(Test-Path -LiteralPath $mods -PathType Container)) { throw "Missing Mods directory: $mods" }
if ([string]::IsNullOrWhiteSpace($ModName) -or $ModName -in @('.', '..') -or $ModName -match '[<>:"/\\|?*]') {
    throw 'ModName must be a single mod directory name.'
}
$mod = Join-Path $mods $ModName
if (!(Test-Path -LiteralPath $mod -PathType Container)) { throw "Missing target mod: $mod" }
if ((Get-Item -LiteralPath $mod).Attributes -band [IO.FileAttributes]::ReparsePoint) {
    throw 'The target mod must be a real directory, not a junction or symlink.'
}
$name = 'EraEditor_ObjectsExtender'
$binaryDirectory = if ($ArtifactsRoot) { Join-Path $ArtifactsRoot 'bin' } else { Join-Path $PSScriptRoot 'build' }
$mapsDirectory = if ($ArtifactsRoot) { Join-Path $ArtifactsRoot 'DebugMaps' } else { Join-Path $PSScriptRoot 'DebugMaps' }
if ($Build) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vs = & $vswhere -latest -requires Microsoft.Component.MSBuild -property installationPath
    if (!$vs) { throw 'Visual Studio MSBuild was not found.' }
    $output = [IO.Path]::GetFullPath($binaryDirectory).TrimEnd('\') + '\'
    $intermediate = Join-Path $output 'obj\'
    & (Join-Path $vs 'MSBuild\Current\Bin\MSBuild.exe') (Join-Path $PSScriptRoot "$name.vcxproj") `
        /nologo /t:Build /m:1 /p:Configuration=Release /p:Platform=Win32 "/p:PluginOutDir=$output" "/p:IntDir=$intermediate" /verbosity:minimal
    if ($LASTEXITCODE) { throw 'Editor plugin build failed.' }
}
$files = [ordered]@{}
foreach ($extension in @('dll','pdb')) {
    $files[(Join-Path $mod "EraEditor\$name.$extension")] = Join-Path $binaryDirectory "$name.$extension"
}
foreach ($extension in @('map','dbgmap')) {
    $files[(Join-Path $mod "DebugMaps\$name.$extension")] = Join-Path $mapsDirectory "$name.$extension"
}
foreach ($source in $files.Values) {
    if (!(Test-Path -LiteralPath $source -PathType Leaf)) { throw "Missing build output: $source" }
}
# Retire every copy: another enabled mod can otherwise install the same hooks twice.
$retired = @()
foreach ($directory in Get-ChildItem -LiteralPath $mods -Directory) {
    if ($directory.Attributes -band [IO.FileAttributes]::ReparsePoint) { continue }
    foreach ($relative in @('EraEditor\EraEditor_JsonOverrides.dll','EraEditor\EraEditor_JsonOverrides.pdb',
        'DebugMaps\EraEditor_JsonOverrides.map','DebugMaps\EraEditor_JsonOverrides.dbgmap')) {
        $target = Join-Path $directory.FullName $relative
        if (Test-Path -LiteralPath $target -PathType Leaf) { $retired += $target }
    }
}
$backup = Join-Path $mod ('Backup\EraEditor-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
$plan = @()
foreach ($target in (@($files.Keys) + $retired)) {
    $absolute = [IO.Path]::GetFullPath($target)
    if (!$absolute.StartsWith($mods + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "Outside Mods: $absolute" }
    $relative = $absolute.Substring($mods.Length + 1)
    $saved = Join-Path $backup $relative
    $exists = Test-Path -LiteralPath $absolute -PathType Leaf
    if ($exists) {
        New-Item -ItemType Directory -Path (Split-Path $saved -Parent) -Force | Out-Null
        Copy-Item -LiteralPath $absolute -Destination $saved
        if ((Get-FileHash -LiteralPath $saved).Hash -ne (Get-FileHash -LiteralPath $absolute).Hash) {
            throw "Backup verification failed: $absolute"
        }
    }
    $plan += @{ Target = $absolute; Backup = $saved; Existed = $exists }
}
New-Item -ItemType Directory -Path $backup -Force | Out-Null
$plan | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $backup 'manifest.json') -Encoding UTF8
try {
    foreach ($target in $files.Keys) {
        New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null
        Copy-Item -LiteralPath $files[$target] -Destination $target -Force
        if ((Get-FileHash -LiteralPath $files[$target]).Hash -ne (Get-FileHash -LiteralPath $target).Hash) {
            throw "Installed file differs: $target"
        }
    }
    foreach ($target in $retired) { Remove-Item -LiteralPath $target }
} catch {
    $installationError = $_
    foreach ($entry in $plan) {
        try {
            if ($entry.Existed) {
                if (!(Test-Path -LiteralPath $entry.Target -PathType Leaf) -or
                    (Get-FileHash -LiteralPath $entry.Backup).Hash -ne (Get-FileHash -LiteralPath $entry.Target).Hash) {
                    Copy-Item -LiteralPath $entry.Backup -Destination $entry.Target -Force
                }
            }
            elseif (Test-Path -LiteralPath $entry.Target -PathType Leaf) { Remove-Item -LiteralPath $entry.Target }
        } catch { Write-Warning "Restore failed: $($entry.Target). Backup: $($entry.Backup)" }
    }
    throw $installationError
}
Write-Output "Installed and verified $name.dll and debug files. Retired $($retired.Count) legacy files."
Write-Output "Backup and manifest: $backup"
