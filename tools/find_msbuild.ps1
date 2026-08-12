[CmdletBinding()]
param(
    [string[]]$SearchRoots = @('D:\'),
    [switch]$AsObject
)

$ErrorActionPreference = 'Stop'
$candidates = [System.Collections.Generic.List[string]]::new()

foreach ($root in $SearchRoots) {
    if (Test-Path $root) {
        Get-ChildItem $root -Filter MSBuild.exe -File -Recurse -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\MSBuild\\Current\\Bin\\MSBuild\.exe$' } |
            ForEach-Object { $candidates.Add($_.FullName) }
    }
}

if ($candidates.Count -eq 0) {
    $vswhereCandidates = @(
        "$env:ProgramFiles(x86)\Microsoft Visual Studio\Installer\vswhere.exe",
        "$env:ProgramFiles\Microsoft Visual Studio\Installer\vswhere.exe"
    )
    foreach ($vswhere in $vswhereCandidates) {
        if (Test-Path $vswhere) {
            $installationPath = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -property installationPath
            if ($installationPath) {
                $path = Join-Path $installationPath 'MSBuild\Current\Bin\MSBuild.exe'
                if (Test-Path $path) {
                    $candidates.Add($path)
                    break
                }
            }
        }
    }
}

if ($candidates.Count -eq 0) {
    foreach ($root in @("$env:ProgramFiles\Microsoft Visual Studio", "$env:ProgramFiles(x86)\Microsoft Visual Studio")) {
        if (Test-Path $root) {
            Get-ChildItem $root -Filter MSBuild.exe -File -Recurse -ErrorAction SilentlyContinue |
                Where-Object { $_.FullName -match '\\MSBuild\\Current\\Bin\\MSBuild\.exe$' } |
                ForEach-Object { $candidates.Add($_.FullName) }
        }
    }
}

if ($candidates.Count -eq 0) {
    throw 'MSBuild.exe was not found under supplied roots, vswhere, or common Visual Studio paths'
}

$msbuildPath = $candidates | Sort-Object -Descending | Select-Object -First 1
$visualStudioRoot = Split-Path (Split-Path (Split-Path (Split-Path $msbuildPath -Parent) -Parent) -Parent) -Parent
$toolsets = @(Get-ChildItem (Join-Path $visualStudioRoot 'VC\Tools\MSVC') -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending)
$toolset = if ($msbuildPath -match '\\2022\\') { 'v143' } elseif ($msbuildPath -match '\\2019\\') { 'v142' } else { 'v143' }

$result = [pscustomobject]@{
    MSBuildPath = $msbuildPath
    PlatformToolset = $toolset
    MSVCToolsPath = if ($toolsets.Count -gt 0) { $toolsets[0].FullName } else { $null }
}

if ($AsObject) {
    return $result
}

$result | Format-List
