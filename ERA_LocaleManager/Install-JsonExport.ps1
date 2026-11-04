param(
    [string]$ModPath = 'D:\games\HoMM 3 ERA\Mods\WoG',
    [string]$BuildPath = (Join-Path $PSScriptRoot 'Verification\build'),
    [string[]]$Plugins = @('ERA_LocaleManager', 'ERA_JsonOverrides', 'RMG_CustomizeObjectProperties', 'Objects_CreatureBanksExtender'),
    [switch]$SkipTranslations
)
$ErrorActionPreference = 'Stop'
$files = [ordered]@{}
foreach ($plugin in $Plugins) {
    foreach ($extension in 'era', 'pdb') {
        $files["EraPlugins\$plugin.$extension"] = Join-Path $BuildPath "plugins\$plugin.$extension"
    }
    foreach ($extension in 'map', 'dbgmap') {
        $files["DebugMaps\$plugin.$extension"] = Join-Path $BuildPath "DebugMaps\$plugin.$extension"
    }
}
if (!$SkipTranslations) {
    foreach ($locale in '', 'ru\', 'uk\', 'pl\') {
        $relative = "lang\${locale}era_language_dlg.json"
        $files[$relative] = Join-Path $PSScriptRoot $relative
    }
}
foreach ($source in $files.Values) {
    if (!(Test-Path -LiteralPath $source -PathType Leaf)) { throw "Missing build file: $source" }
    if ([IO.Path]::GetExtension($source) -eq '.json') { Get-Content -LiteralPath $source -Raw | ConvertFrom-Json | Out-Null }
}
$backupPath = Join-Path $ModPath ('Backup\JsonExport-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
foreach ($relative in $files.Keys) {
    $destination = Join-Path $ModPath $relative
    if (Test-Path -LiteralPath $destination) {
        $backupFile = Join-Path $backupPath $relative
        New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($backupFile)) -Force | Out-Null
        Copy-Item -LiteralPath $destination -Destination $backupFile
    }
}
foreach ($relative in $files.Keys) {
    $destination = Join-Path $ModPath $relative
    New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($destination)) -Force | Out-Null
    Copy-Item -LiteralPath $files[$relative] -Destination $destination -Force
}
Write-Output "Installed JSON export. Backup: $backupPath"
