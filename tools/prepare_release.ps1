[CmdletBinding()]
param(
    [string]$ProjectRoot,
    [string]$ManifestPath,
    [string]$Configuration = 'Release',
    [string]$Platform = 'x64',
    [switch]$Offline
)

$ErrorActionPreference = 'Stop'
$toolsRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $ProjectRoot) {
    $ProjectRoot = Split-Path -Parent $toolsRoot
}
if (-not $ManifestPath) {
    $ManifestPath = Join-Path $toolsRoot 'release-manifest.json'
}
$outputRoot = Join-Path $ProjectRoot "$Platform\$Configuration"
$releaseRoot = Join-Path $outputRoot 'a'
$failed = $false

try {
    if (-not $Offline) {
        & (Join-Path $toolsRoot 'update_external_artifacts.ps1') -ProjectRoot $ProjectRoot -ManifestPath $ManifestPath
        if ($LASTEXITCODE -ne 0) {
            throw 'External artifact update and local fallback both failed'
        }
    }

    $mainDll = Join-Path $outputRoot 'OptiScaler.dll'
    if (-not (Test-Path $mainDll -PathType Leaf)) {
        throw "Main build output is missing: $mainDll"
    }

    if (Test-Path $releaseRoot) {
        Remove-Item $releaseRoot -Recurse -Force
    }
    New-Item -ItemType Directory -Path $releaseRoot -Force | Out-Null
    Copy-Item $mainDll (Join-Path $releaseRoot 'OptiScaler.dll') -Force

    $manifest = Get-Content $ManifestPath -Raw | ConvertFrom-Json
    foreach ($file in $manifest.releaseFiles) {
        $source = Join-Path $ProjectRoot $file.source
        $destination = Join-Path $releaseRoot $file.destination
        if ($file.type -eq 'directory') {
            if (-not (Test-Path $source -PathType Container)) {
                throw "Release source is missing: $source ($($file.sourceDescription))"
            }
            New-Item -ItemType Directory -Path $destination -Force | Out-Null
            $pattern = if ($file.pattern) { $file.pattern } else { '*' }
            Get-ChildItem $source -Filter $pattern -File | Copy-Item -Destination $destination -Force
        }
        else {
            if (-not (Test-Path $source -PathType Leaf)) {
                throw "Release source is missing: $source ($($file.sourceDescription))"
            }
            if (-not $file.allowEmpty -and (Get-Item $source).Length -eq 0) {
                throw "Release source is empty: $source ($($file.sourceDescription))"
            }
            New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
            Copy-Item $source $destination -Force
        }
    }

    if (-not $Offline) {
        $manifest.externalArtifacts | ForEach-Object {
            $entries = if ($_.entries) { @($_.entries) } else { @($_) }
            foreach ($entry in $entries) {
                $source = Join-Path $ProjectRoot $entry.destination
                $relativeDestination = switch -Wildcard ($entry.destination) {
                    '*OptiPatcher.asi' { 'OptiScaler\plugins\OptiPatcher.asi'; break }
                    default { Join-Path 'OptiScaler' ([System.IO.Path]::GetFileName($entry.destination)) }
                }
                $destination = Join-Path $releaseRoot $relativeDestination
                New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
                Copy-Item $source $destination -Force
            }
        }
    }
    else {
        $fallbacks = @(
            @{ Source = 'fakenvapi.dll'; Destination = 'OptiScaler\fakenvapi.dll' },
            @{ Source = 'fakenvapi.ini'; Destination = 'OptiScaler\fakenvapi.ini' },
            @{ Source = 'dlssg_to_fsr3_amd_is_better.dll'; Destination = 'OptiScaler\dlssg_to_fsr3_amd_is_better.dll' },
            @{ Source = 'plugins\OptiPatcher.asi'; Destination = 'OptiScaler\plugins\OptiPatcher.asi' }
        )
        foreach ($fallback in $fallbacks) {
            $source = Join-Path $ProjectRoot $fallback.Source
            if (Test-Path $source -PathType Leaf) {
                $destination = Join-Path $releaseRoot $fallback.Destination
                New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
                Copy-Item $source $destination -Force
            }
        }
    }

    New-Item -ItemType File -Path (Join-Path $releaseRoot '!! EXTRACT ALL FILES TO GAME FOLDER !!') -Force | Out-Null
    Write-Host "Release assembled at $releaseRoot"
}
catch {
    Write-Error $_ -ErrorAction Continue
    $failed = $true
}

$global:LASTEXITCODE = if ($failed) { 1 } else { 0 }
