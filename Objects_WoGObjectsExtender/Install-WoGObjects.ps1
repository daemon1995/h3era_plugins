param(
    [string]$GameRoot = 'D:\games\HoMM 3 ERA',
    [string]$Python = 'python',
    [switch]$CheckOnly
)
$ErrorActionPreference = 'Stop'
$gamePath = [IO.Path]::GetFullPath($GameRoot).TrimEnd('\')
$stage = Join-Path $PSScriptRoot 'build\install'
& $Python (Join-Path $PSScriptRoot 'Verification\prepare-wog-install.py') --game $gamePath --out $stage
if ($LASTEXITCODE -ne 0) { throw 'Preparation failed. Game files were not modified.' }
$updates = Get-Content -LiteralPath (Join-Path $stage 'manifest.json') -Raw -Encoding UTF8 | ConvertFrom-Json
foreach ($update in $updates) {
    if (![IO.Path]::GetFullPath($update.Target).StartsWith($gamePath + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid installation target' }
    if ((Get-FileHash -LiteralPath $update.Source).Hash -ne $update.Sha256) { throw "Staged content mismatch: $($update.Source)" }
}
if ($CheckOnly) { Write-Output "Validated complete WoG object installation: $($updates.Count) files."; return }
if (Get-Process | Where-Object { $_.ProcessName -match '^(h3era|heroes3)' }) { throw 'Close all Heroes III / ERA instances before installation. Game files were not modified.' }
$backup = Join-Path $gamePath ('Mods\WoG\Backup\WoGObjects-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
$written = [Collections.Generic.List[object]]::new()
foreach ($update in $updates) {
    $update | Add-Member -NotePropertyName Existed -NotePropertyValue (Test-Path -LiteralPath $update.Target -PathType Leaf)
    $saved = Join-Path $backup $update.Target.Substring($gamePath.Length + 1)
    $update | Add-Member -NotePropertyName Backup -NotePropertyValue $saved
    if ($update.Existed) {
        New-Item -ItemType Directory -Path (Split-Path -Parent $saved) -Force | Out-Null
        Copy-Item -LiteralPath $update.Target -Destination $saved
    }
}
try {
    foreach ($update in $updates) {
        New-Item -ItemType Directory -Path (Split-Path -Parent $update.Target) -Force | Out-Null
        $written.Add($update)
        Copy-Item -LiteralPath $update.Source -Destination $update.Target -Force
        if ((Get-FileHash -LiteralPath $update.Target).Hash -ne $update.Sha256) { throw "Installed content mismatch: $($update.Target)" }
    }
} catch {
    foreach ($update in $written) {
        if ($update.Existed) { Copy-Item -LiteralPath $update.Backup -Destination $update.Target -Force }
        elseif (Test-Path -LiteralPath $update.Target) { Remove-Item -LiteralPath $update.Target }
    }
    throw
}
Write-Output "Installed all WoG objects. Backup: $backup"
Write-Output 'Restart ERA. Reload embedded ERM scripts when using an older save.'
