[CmdletBinding()]
param(
    [string]$ProjectRoot,
    [string]$ManifestPath,
    [string]$Configuration = 'Release',
    [string]$Platform = 'x64'
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

try {
    $manifest = Get-Content $ManifestPath -Raw | ConvertFrom-Json
    if ($manifest.schemaVersion -ne 1) {
        throw "Unsupported manifest schema version: $($manifest.schemaVersion)"
    }

    foreach ($dependency in $manifest.dependencies) {
        $path = Join-Path $ProjectRoot $dependency.path
        $valid = Test-Path $path
        if ($valid -and $dependency.type -eq 'file' -and -not $dependency.allowEmpty) {
            $valid = (Get-Item $path).Length -gt 0
        }
        if ($valid -and $dependency.type -eq 'directory') {
            $pattern = if ($dependency.pattern) { $dependency.pattern } else { '*' }
            $minimumCount = if ($null -ne $dependency.minimumCount) { [int]$dependency.minimumCount } else { 1 }
            $valid = @(Get-ChildItem $path -Filter $pattern -File -ErrorAction SilentlyContinue).Count -ge $minimumCount
        }
        if (-not $valid) {
            Write-Error "Missing dependency '$($dependency.id)': $path (source: $($dependency.source))" -ErrorAction Continue
            $failed = $true
        }
    }
}
catch {
    Write-Error $_ -ErrorAction Continue
    $failed = $true
}

if ($failed) {
    $global:LASTEXITCODE = 1
    return
}

Write-Host "Offline dependencies are complete for $Configuration|$Platform"
$global:LASTEXITCODE = 0
