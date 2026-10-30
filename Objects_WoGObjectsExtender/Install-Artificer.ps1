# Kept for existing developer shortcuts. A native object update includes the API host and all object data.
param(
    [string]$GameRoot = 'D:\games\HoMM 3 ERA',
    [string]$Python = 'python',
    [switch]$CheckOnly
)
& (Join-Path $PSScriptRoot 'Install-WoGObjects.ps1') @PSBoundParameters
