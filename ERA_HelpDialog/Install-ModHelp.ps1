[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Mandatory = $true)]
    [string]$ModsRoot
)

$ErrorActionPreference = 'Stop'
$modsPath = (Resolve-Path -LiteralPath $ModsRoot).Path
$sources = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'content/mod-help-sources.json') -Raw | ConvertFrom-Json
$english = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'content/mod-help.en.json') -Raw | ConvertFrom-Json
$russian = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'content/mod-help.ru.json') -Raw | ConvertFrom-Json
$backupRoot = Join-Path $PSScriptRoot ('build/content-audit/backups/' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
$utf8 = New-Object System.Text.UTF8Encoding($false)
$written = 0

function Write-HelpFile([string]$Folder, [string]$Relative, [string]$Json) {
    $modPath = Join-Path $modsPath $Folder
    if (-not (Test-Path -LiteralPath $modPath -PathType Container)) {
        throw "Mod folder not found: $modPath"
    }
    $destination = Join-Path $modPath $Relative
    if ($PSCmdlet.ShouldProcess($destination, 'Install help JSON')) {
        if (Test-Path -LiteralPath $destination) {
            $backup = Join-Path (Join-Path $backupRoot $Folder) $Relative
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $backup) | Out-Null
            Copy-Item -LiteralPath $destination -Destination $backup
        }
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
        [IO.File]::WriteAllText($destination, $Json, $utf8)
        $script:written++
    }
}

foreach ($entry in $sources.mods.PSObject.Properties) {
    $key = $entry.Name
    $folder = $entry.Value.folder
    $filename = $entry.Value.filename
    if (-not $filename -or $filename -notmatch '^era_help_[a-z0-9_]+\.json$') { throw "Invalid help filename for: $key" }
    foreach ($language in @('', 'ru/')) {
        $content = if ($language) { $russian } else { $english }
        $value = $content.help.PSObject.Properties[$key].Value
        if (-not $value) { throw "Missing help profile: $key ($language)" }
        $record = [ordered]@{ help = [ordered]@{ $key = $value } }
        Write-HelpFile $folder ('Lang/' + $language + $filename) (($record | ConvertTo-Json -Depth 60) + "`n")
    }
}

foreach ($language in @('', 'ru/')) {
    $source = Join-Path $PSScriptRoot ('lang/' + $language + 'era_help.json')
    Write-HelpFile 'WoG' ('Lang/' + $language + 'era_help.json') ([IO.File]::ReadAllText($source))
}
Write-Output "Installed $written JSON files. Previous versions, when present: $backupRoot"
