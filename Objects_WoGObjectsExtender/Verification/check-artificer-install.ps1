param([string]$Python = 'python')
$ErrorActionPreference = 'Stop'
& $Python (Join-Path $PSScriptRoot 'check-wog-install.py')
if ($LASTEXITCODE -ne 0) { throw 'WoG object installation checks failed' }
