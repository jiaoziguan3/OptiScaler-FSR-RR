[CmdletBinding()]
param(
    [string]$ProjectRoot,
    [string]$ManifestPath,
    [int]$TimeoutSec = 30
)

$ErrorActionPreference = 'Stop'
$toolsRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $ProjectRoot) {
    $ProjectRoot = Split-Path -Parent $toolsRoot
}
if (-not $ManifestPath) {
    $ManifestPath = Join-Path $toolsRoot 'release-manifest.json'
}
$failed = $false
$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('optiscaler-download-' + [guid]::NewGuid().ToString('N'))

function Copy-ArtifactFallback {
    param($Entry, [string]$Id)
    $source = Join-Path $ProjectRoot $Entry.fallbackPath
    $destination = Join-Path $ProjectRoot $Entry.destination
    if (-not (Test-Path $source -PathType Leaf)) {
        throw "Network update failed and fallback is missing for '$Id': $source"
    }
    New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
    Copy-Item $source $destination -Force
}

function Copy-ExtractedEntry {
    param([string]$ExtractedRoot, $Entry, [string]$Id)
    $name = [System.IO.Path]::GetFileName([string]$Entry.archiveEntry)
    $source = Get-ChildItem $ExtractedRoot -Recurse -File | Where-Object { $_.Name -ieq $name } | Select-Object -First 1
    if (-not $source) {
        throw "Archive entry '$($Entry.archiveEntry)' was not found for '$Id'"
    }
    $destination = Join-Path $ProjectRoot $Entry.destination
    New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
    Copy-Item $source.FullName $destination -Force
}

try {
    New-Item -ItemType Directory -Path $temporaryRoot -Force | Out-Null
    $manifest = Get-Content $ManifestPath -Raw | ConvertFrom-Json
    foreach ($artifact in $manifest.externalArtifacts) {
        $entries = if ($artifact.entries) { @($artifact.entries) } else { @($artifact) }
        try {
            $downloadPath = Join-Path $temporaryRoot ([System.IO.Path]::GetFileName(([uri]$artifact.url).AbsolutePath))
            Invoke-WebRequest -Uri $artifact.url -OutFile $downloadPath -TimeoutSec $TimeoutSec -UseBasicParsing
            if ($artifact.archiveEntry -or $artifact.entries) {
                $extractRoot = Join-Path $temporaryRoot $artifact.id
                New-Item -ItemType Directory -Path $extractRoot -Force | Out-Null
                if ([System.IO.Path]::GetExtension($downloadPath) -ieq '.zip') {
                    Expand-Archive -Path $downloadPath -DestinationPath $extractRoot -Force
                }
                else {
                    $sevenZip = Get-Command 7z.exe -ErrorAction SilentlyContinue
                    if (-not $sevenZip) {
                        throw '7z.exe is required to extract this download'
                    }
                    & $sevenZip.Source x $downloadPath "-o$extractRoot" -y | Out-Null
                    if ($LASTEXITCODE -ne 0) {
                        throw "7-Zip extraction failed with exit code $LASTEXITCODE"
                    }
                }
                foreach ($entry in $entries) {
                    Copy-ExtractedEntry $extractRoot $entry $artifact.id
                }
            }
            else {
                $destination = Join-Path $ProjectRoot $artifact.destination
                New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
                Copy-Item $downloadPath $destination -Force
            }
            Write-Host "Updated external artifact '$($artifact.id)' from network"
        }
        catch {
            Write-Warning "Network update failed for '$($artifact.id)': $($_.Exception.Message). Using local fallback."
            try {
                foreach ($entry in $entries) {
                    Copy-ArtifactFallback $entry $artifact.id
                }
                Write-Host "Used local fallback for '$($artifact.id)'"
            }
            catch {
                Write-Error $_ -ErrorAction Continue
                $failed = $true
            }
        }
    }
}
catch {
    Write-Error $_ -ErrorAction Continue
    $failed = $true
}
finally {
    Remove-Item $temporaryRoot -Recurse -Force -ErrorAction SilentlyContinue
}

$global:LASTEXITCODE = if ($failed) { 1 } else { 0 }
