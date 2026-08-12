[CmdletBinding()]
param(
    [string]$ProjectRoot,
    [string]$Configuration = 'Release',
    [string]$Platform = 'x64',
    [string[]]$SearchRoots = @('D:\'),
    [switch]$Offline
)

$ErrorActionPreference = 'Stop'
$toolsRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $ProjectRoot) {
    $ProjectRoot = Split-Path -Parent $toolsRoot
}
$solutionPath = Join-Path $ProjectRoot 'OptiScaler.sln'
$manifestPath = Join-Path $toolsRoot 'release-manifest.json'

try {
    if (-not (Test-Path $solutionPath -PathType Leaf)) {
        throw "Solution is missing: $solutionPath"
    }
    if (-not (Get-Command powershell.exe -ErrorAction SilentlyContinue)) {
        throw 'powershell.exe is unavailable'
    }
    if (-not (Get-Command git.exe -ErrorAction SilentlyContinue)) {
        Write-Warning 'git.exe is unavailable; build commit metadata will use unknown'
    }

    & (Join-Path $toolsRoot 'check_offline_dependencies.ps1') -ProjectRoot $ProjectRoot -ManifestPath $manifestPath -Configuration $Configuration -Platform $Platform
    if ($LASTEXITCODE -ne 0) {
        throw 'Project dependency check failed'
    }

    $toolchain = & (Join-Path $toolsRoot 'find_msbuild.ps1') -SearchRoots $SearchRoots -AsObject
    if (-not $toolchain.MSVCToolsPath) {
        throw "MSVC x64 tools were not found beside MSBuild: $($toolchain.MSBuildPath)"
    }

    $windowsSdkRoot = @(
        ${env:ProgramFiles(x86)},
        $env:ProgramFiles
    ) | Where-Object { $_ } | ForEach-Object { Join-Path $_ 'Windows Kits\10\Include' } | Where-Object { Test-Path $_ -PathType Container } | Select-Object -First 1
    if (-not $windowsSdkRoot) {
        throw 'Windows 10/11 SDK include directory was not found'
    }

    Write-Host "MSBuild: $($toolchain.MSBuildPath)"
    Write-Host "Platform toolset: $($toolchain.PlatformToolset)"
    Write-Host "MSVC tools: $($toolchain.MSVCToolsPath)"
    Write-Host "Windows SDK: $windowsSdkRoot"

    $offlineProperty = if ($Offline) { 'true' } else { 'false' }
    $arguments = @(
        $solutionPath,
        '/m',
        '/t:Build',
        "/p:Configuration=$Configuration",
        "/p:Platform=$Platform",
        "/p:PlatformToolset=$($toolchain.PlatformToolset)",
        "/p:OptiScalerOffline=$offlineProperty",
        '/nologo'
    )
    & $toolchain.MSBuildPath @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "MSBuild failed with exit code $LASTEXITCODE"
    }
}
catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}

exit 0
