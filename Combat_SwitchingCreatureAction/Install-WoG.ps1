param(
    [string]$GamePath = 'D:/games/HoMM 3 ERA',
    [string]$BuildPath = ''
)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
if (!$BuildPath) { $BuildPath = Join-Path $repoRoot '_build/combat-xp-release' }
if (Get-Process | Where-Object { $_.ProcessName -match '^h3era|^Heroes3' }) {
    throw 'Close the game before installing the plugin.'
}
$modRoot = Join-Path $GamePath 'Mods/WoG'
if (!(Test-Path -LiteralPath $modRoot -PathType Container)) { throw "WoG mod not found: $modRoot" }
$files = @(
    @{ Source = (Join-Path $BuildPath 'Combat_SwitchingCreatureAction.era'); Relative = 'EraPlugins/Combat_SwitchingCreatureAction.era' },
    @{ Source = (Join-Path $repoRoot '_build/DebugMaps/Combat_SwitchingCreatureAction.map'); Relative = 'DebugMaps/Combat_SwitchingCreatureAction.map' },
    @{ Source = (Join-Path $repoRoot '_build/DebugMaps/Combat_SwitchingCreatureAction.dbgmap'); Relative = 'DebugMaps/Combat_SwitchingCreatureAction.dbgmap' }
)
foreach ($resourceRoot in @('Lang', 'Data')) {
    $root = Join-Path $PSScriptRoot $resourceRoot
    foreach ($file in Get-ChildItem -LiteralPath $root -Recurse -File) {
        $relative = $file.FullName.Substring($PSScriptRoot.Length + 1)
        $files += @{ Source = $file.FullName; Relative = $relative }
    }
}
foreach ($file in $files) {
    if (!(Test-Path -LiteralPath $file.Source -PathType Leaf)) { throw "Missing artifact: $($file.Source)" }
    if ([IO.Path]::GetExtension($file.Source) -eq '.json') {
        $null = Get-Content -LiteralPath $file.Source -Raw -Encoding UTF8 | ConvertFrom-Json
    }
}
$backupRoot = Join-Path $repoRoot ('_build/before-area-shot-types-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
foreach ($file in $files) {
    $destination = Join-Path $modRoot $file.Relative
    if (Test-Path -LiteralPath $destination -PathType Leaf) {
        $backup = Join-Path $backupRoot $file.Relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $backup) -Force | Out-Null
        Copy-Item -LiteralPath $destination -Destination $backup
    }
}
foreach ($file in $files) {
    $destination = Join-Path $modRoot $file.Relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $file.Source -Destination $destination -Force
    if ((Get-FileHash -LiteralPath $file.Source -Algorithm SHA256).Hash -ne
        (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash) {
        throw "Copy mismatch: $destination"
    }
    Write-Output "Installed: $($file.Relative)"
}
Write-Output "Backup: $backupRoot"
