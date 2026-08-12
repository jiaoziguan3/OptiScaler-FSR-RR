[CmdletBinding()]
param(
    [string]$ProjectRoot,
    [string]$Configuration = 'Release',
    [string]$Platform = 'x64',
    [switch]$FallbackOnly
)

$ErrorActionPreference = 'Stop'
$toolsRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $ProjectRoot) {
    $ProjectRoot = Split-Path -Parent $toolsRoot
}
$installerRoot = Join-Path $ProjectRoot 'installer'
$specPath = Join-Path $installerRoot 'OptiScalerSettings.spec'
$executablePath = Join-Path $installerRoot 'dist\OptiScalerSettings.exe'
$releaseRoot = Join-Path $ProjectRoot "$Platform\$Configuration\a"
$releasePath = Join-Path $releaseRoot 'OptiScalerSettings.exe'
$failed = $false

try {
    if (-not $FallbackOnly) {
        if (-not (Test-Path $specPath -PathType Leaf)) {
            throw "Settings specification is missing: $specPath"
        }
        $python = Get-Command python.exe -ErrorAction SilentlyContinue
        if (-not $python) {
            throw 'python.exe is unavailable; use -FallbackOnly to publish the existing executable'
        }
        Push-Location $installerRoot
        try {
            & $python.Source -m PyInstaller --noconfirm --clean $specPath
            if ($LASTEXITCODE -ne 0) {
                throw "PyInstaller failed with exit code $LASTEXITCODE"
            }
        }
        finally {
            Pop-Location
        }
    }

    if (-not (Test-Path $executablePath -PathType Leaf)) {
        throw "Settings executable is missing: $executablePath"
    }
    if ((Get-Item $executablePath).Length -eq 0) {
        throw "Settings executable is empty: $executablePath"
    }

    New-Item -ItemType Directory -Path $releaseRoot -Force | Out-Null
    Copy-Item $executablePath $releasePath -Force
    Write-Host "Settings executable published at $releasePath"
}
catch {
    Write-Error $_ -ErrorAction Continue
    $failed = $true
}

$global:LASTEXITCODE = if ($failed) { 1 } else { 0 }
