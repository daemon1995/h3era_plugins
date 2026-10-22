param([string]$GamePath = 'D:\games\HoMM 3 ERA', [switch]$Build)
$ErrorActionPreference = 'Stop'
$workspace = Split-Path $PSScriptRoot -Parent
$projects = @('API_ErmFunctions', 'ERA_DwellingsExtender', 'RMG_CustomizeObjectProperties', 'ERA_LocaleManager')
if (Get-Process -Name 'h3era','h3era HD' -ErrorAction SilentlyContinue) { throw 'Close the game before installing plugins.' }
if ($Build) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vs = & $vswhere -latest -property installationPath
    $msbuild = Join-Path $vs 'MSBuild\Current\Bin\MSBuild.exe'
    foreach ($name in $projects) {
        $project = Join-Path $workspace $name
        & $msbuild (Join-Path $project "$name.vcxproj") /nologo /m /p:Configuration=Release /p:Platform=Win32 "/p:PluginOutDir=$project\build\\" "/p:IntDir=$project\build\split-obj\\" "/p:ReleaseMapPath=$project\build\$name.map" /verbosity:minimal
        if ($LASTEXITCODE -ne 0) { throw "Build failed: $name" }
    }
}
$plugins = Join-Path $GamePath 'Mods\WoG\EraPlugins'
if (!(Test-Path -LiteralPath $plugins -PathType Container)) { throw "Plugin directory does not exist: $plugins" }
$stamp = [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssZ')
$backupRoot = Join-Path $PSScriptRoot "build\installed-backups\$stamp"
$files = @()
foreach ($name in $projects) {
    foreach ($extension in @('era', 'pdb')) {
        $source = Join-Path $workspace "$name\build\$name.$extension"
        if (!(Test-Path -LiteralPath $source -PathType Leaf)) { throw "Build output missing: $source" }
        $target = Join-Path $plugins "$name.$extension"
        $files += @{ Source = $source; Target = $target; Backup = (Join-Path $backupRoot "$name.$extension"); Existed = (Test-Path -LiteralPath $target) }
    }
}
New-Item -ItemType Directory -Path $backupRoot | Out-Null
foreach ($file in $files) {
    if ($file.Existed) { Copy-Item -LiteralPath $file.Target -Destination $file.Backup }
}
$files | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $backupRoot 'manifest.json') -Encoding UTF8
try {
    foreach ($file in $files) {
        Copy-Item -LiteralPath $file.Source -Destination $file.Target -Force
        if ((Get-FileHash -LiteralPath $file.Source).Hash -ne (Get-FileHash -LiteralPath $file.Target).Hash) {
            throw "Installed file differs: $($file.Target)"
        }
    }
} catch {
    $installationError = $_
    foreach ($file in $files) {
        try {
            if ($file.Existed) { Copy-Item -LiteralPath $file.Backup -Destination $file.Target -Force }
            elseif (Test-Path -LiteralPath $file.Target) { Remove-Item -LiteralPath $file.Target }
        } catch {
            Write-Warning "Cannot restore $($file.Target); previous file is at $($file.Backup)."
        }
    }
    throw $installationError
}
Write-Output 'Installed and hash-verified: API_ErmFunctions, ERA_DwellingsExtender, RMG_CustomizeObjectProperties, ERA_LocaleManager.'
Write-Output "Previous files and manifest: $backupRoot"
