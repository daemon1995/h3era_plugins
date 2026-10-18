param(
    [string]$GameRoot = 'D:\games\HoMM 3 ERA',
    [switch]$CheckOnly
)
$ErrorActionPreference = 'Stop'
$utf8 = [Text.UTF8Encoding]::new($false)
$gamePath = [IO.Path]::GetFullPath($GameRoot).TrimEnd('\')
$modPath = Join-Path $gamePath 'Mods\WoG'
$scriptPath = Join-Path $gamePath 'Mods\WoG Scripts\Data\s'
$pluginName = 'Objects_WoGObjectsExtender'
$pluginSource = Join-Path $PSScriptRoot "$pluginName.era"
$flatPackage = Test-Path -LiteralPath $pluginSource
if (!$flatPackage) { $pluginSource = Join-Path $PSScriptRoot "build\Release\$pluginName.era" }
if (!(Test-Path -LiteralPath $pluginSource -PathType Leaf)) { throw "Build the Release plugin first: $pluginSource" }
$editsPath = Join-Path $PSScriptRoot 'legacy\artificer-native-handler.edits.json'
$specs = [IO.File]::ReadAllText($editsPath, $utf8) | ConvertFrom-Json
$updates = [Collections.Generic.List[object]]::new()
$updates.Add(@{ Target = Join-Path $modPath "EraPlugins\$pluginName.era"; Source = $pluginSource })

function Set-Property($Object, [string]$Name, $Value) {
    if ($null -eq $Object.PSObject.Properties[$Name]) { $Object | Add-Member -NotePropertyName $Name -NotePropertyValue $Value }
    else { $Object.PSObject.Properties[$Name].Value = $Value }
}
function Ensure-Object($Object, [string]$Name) {
    if ($null -eq $Object.PSObject.Properties[$Name]) { Set-Property $Object $Name ([pscustomobject]@{}) }
    return $Object.PSObject.Properties[$Name].Value
}
foreach ($relative in @('wog_objects.json','ru\wog_objects.json')) {
    $source = [IO.File]::ReadAllText((Join-Path $PSScriptRoot "lang\$relative"), $utf8) | ConvertFrom-Json
    $target = Join-Path $modPath "Lang\$relative"
    $result = $source
    if (Test-Path -LiteralPath $target -PathType Leaf) { $result = [IO.File]::ReadAllText($target, $utf8) | ConvertFrom-Json }
    $wogObjects = Ensure-Object $result 'wog_objects'
    Set-Property $wogObjects 'artificer' $source.wog_objects.artificer
    $rmg = Ensure-Object $result 'RMG'
    $generation = Ensure-Object $rmg 'objectGeneration'
    $eraObjects = Ensure-Object $generation '141'
    Set-Property $eraObjects '52' $source.RMG.objectGeneration.'141'.'52'
    $updates.Add(@{ Target = $target; Text = ($result | ConvertTo-Json -Depth 32) + "`r`n"; Encoding = $utf8 })
}

foreach ($spec in $specs) {
    $target = Join-Path $scriptPath $spec.file
    if (!(Test-Path -LiteralPath $target -PathType Leaf)) { throw "Missing WoG script: $target" }
    $bytes = [IO.File]::ReadAllBytes($target)
    $original = [IO.File]::ReadAllText($target, $utf8)
    $crlf = $original.Contains("`r`n")
    $text = $original.Replace("`r`n", "`n")
    foreach ($edit in $spec.edits) {
        if ($null -ne $edit.append) {
            if (!$text.Contains([string]$edit.append)) { $text += [string]$edit.append }
        } else {
            if ($text.Contains([string]$edit.after)) { continue }
            $count = [regex]::Matches($text, [regex]::Escape([string]$edit.before)).Count
            if ($count -ne 1) { throw "Expected one patch anchor in $target; found $count. No files were installed." }
            $text = $text.Replace([string]$edit.before, [string]$edit.after)
        }
    }
    if ($crlf) { $text = $text.Replace("`n", "`r`n") }
    $hasBom = $bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191
    $updates.Add(@{ Target = $target; Text = $text; Encoding = [Text.UTF8Encoding]::new($hasBom) })
}

foreach ($extension in @('pdb','map','dbgmap')) {
    if ($flatPackage) { $source = Join-Path $PSScriptRoot "$pluginName.$extension" }
    elseif ($extension -eq 'pdb') { $source = Join-Path $PSScriptRoot "build\Release\$pluginName.$extension" }
    else { $source = Join-Path $PSScriptRoot "build\DebugMaps\$pluginName.$extension" }
    if (Test-Path -LiteralPath $source -PathType Leaf) {
        $folder = if ($extension -eq 'pdb') { 'EraPlugins' } else { 'DebugMaps' }
        $updates.Add(@{ Target = Join-Path $modPath "$folder\$pluginName.$extension"; Source = $source })
    }
}
foreach ($update in $updates) {
    $resolvedTarget = [IO.Path]::GetFullPath($update.Target)
    if (!$resolvedTarget.StartsWith($gamePath + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "Target outside game root: $resolvedTarget" }
}
if ($CheckOnly) {
    Write-Output "Validated Artificer DLL, JSON and legacy script edits ($($updates.Count) files)."
    return
}
if (Get-Process | Where-Object { $_.ProcessName -match '^(h3era|heroes3)' }) {
    throw 'Close all Heroes III / ERA processes before installation. No installed files were changed.'
}

$backupRoot = Join-Path $modPath ('Backup\Artificer-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null
foreach ($update in $updates) {
    $update.Existed = Test-Path -LiteralPath $update.Target -PathType Leaf
    if ($update.Existed) {
        $relative = $update.Target.Substring($gamePath.Length + 1)
        $update.Backup = Join-Path $backupRoot $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $update.Backup) -Force | Out-Null
        Copy-Item -LiteralPath $update.Target -Destination $update.Backup
    }
}
$written = [Collections.Generic.List[object]]::new()
try {
    foreach ($update in $updates) {
        New-Item -ItemType Directory -Path (Split-Path -Parent $update.Target) -Force | Out-Null
        $written.Add($update)
        if ($null -ne $update.Source) {
            Copy-Item -LiteralPath $update.Source -Destination $update.Target -Force
            if ((Get-FileHash -LiteralPath $update.Source).Hash -ne (Get-FileHash -LiteralPath $update.Target).Hash) { throw "Copy mismatch: $($update.Target)" }
        } else { [IO.File]::WriteAllText($update.Target, $update.Text, $update.Encoding) }
    }
} catch {
    foreach ($update in $written) {
        if ($update.Existed) { Copy-Item -LiteralPath $update.Backup -Destination $update.Target -Force }
        elseif (Test-Path -LiteralPath $update.Target -PathType Leaf) { Remove-Item -LiteralPath $update.Target }
    }
    throw
}
Write-Output "Installed Artificer. Backup: $backupRoot"
Write-Output 'Start ERA again. Saved games containing old embedded ERM scripts also need their scripts reloaded.'
