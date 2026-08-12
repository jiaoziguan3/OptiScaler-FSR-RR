$ErrorActionPreference = 'Stop'

$toolsRoot = Split-Path -Parent $PSScriptRoot
$projectRoot = Split-Path -Parent $toolsRoot
$failures = [System.Collections.Generic.List[string]]::new()

function Invoke-Test {
    param([string]$Name, [scriptblock]$Body)
    try {
        & $Body
        Write-Host "PASS $Name"
    }
    catch {
        $failures.Add("$Name`: $($_.Exception.Message)")
        Write-Host "FAIL $Name"
    }
}

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) {
        throw $Message
    }
}

function New-TestRoot {
    $path = Join-Path ([System.IO.Path]::GetTempPath()) ("optiscaler-tools-" + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $path | Out-Null
    return $path
}

Invoke-Test 'manifest defines dependencies artifacts and release files' {
    $manifest = Get-Content (Join-Path $toolsRoot 'release-manifest.json') -Raw | ConvertFrom-Json
    Assert-True ($manifest.schemaVersion -eq 1) 'schemaVersion must be 1'
    Assert-True ($manifest.dependencies.Count -gt 0) 'dependencies must not be empty'
    Assert-True ($manifest.externalArtifacts.Count -eq 3) 'three external artifacts are required'
    Assert-True ($manifest.releaseFiles.Count -gt 0) 'releaseFiles must not be empty'
    foreach ($artifact in $manifest.externalArtifacts) {
        Assert-True ([uri]::IsWellFormedUriString($artifact.url, 'Absolute')) "$($artifact.id) has an invalid URL"
        Assert-True (-not [string]::IsNullOrWhiteSpace($artifact.fallbackPath)) "$($artifact.id) has no fallback"
    }
}

Invoke-Test 'manifest publishes settings executable at release root' {
    $manifest = Get-Content (Join-Path $toolsRoot 'release-manifest.json') -Raw | ConvertFrom-Json
    $entry = @($manifest.releaseFiles | Where-Object {
        $_.source -eq 'installer\dist\OptiScalerSettings.exe' -and
        $_.destination -eq 'OptiScalerSettings.exe' -and
        $_.type -eq 'file'
    })
    Assert-True ($entry.Count -eq 1) 'settings executable must have one release-root manifest entry'
    Assert-True (-not $entry[0].allowEmpty) 'settings executable must not allow empty content'
}

Invoke-Test 'settings fallback is validated and copied to release root' {
    $root = New-TestRoot
    try {
        New-Item -ItemType Directory -Path (Join-Path $root 'installer\dist') -Force | Out-Null
        Set-Content -Path (Join-Path $root 'installer\dist\OptiScalerSettings.exe') -Value 'settings'
        & (Join-Path $toolsRoot 'build_settings.ps1') -ProjectRoot $root -Configuration Release -Platform x64 -FallbackOnly
        Assert-True ($LASTEXITCODE -eq 0) 'valid fallback should return zero'
        $published = Join-Path $root 'x64\Release\a\OptiScalerSettings.exe'
        Assert-True (Test-Path $published -PathType Leaf) 'fallback should be copied to release root'
        Assert-True ((Get-Item $published).Length -gt 0) 'published fallback should not be empty'
    }
    finally {
        Remove-Item $root -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Invoke-Test 'dependency check validates files and directories' {
    $root = New-TestRoot
    try {
        New-Item -ItemType Directory -Path (Join-Path $root 'runtime') | Out-Null
        Set-Content -Path (Join-Path $root 'present.bin') -Value 'ok'
        Set-Content -Path (Join-Path $root 'runtime\one.dll') -Value 'ok'
        $manifestPath = Join-Path $root 'manifest.json'
        @{
            schemaVersion = 1
            dependencies = @(
                @{ id = 'present'; path = 'present.bin'; type = 'file'; allowEmpty = $false; source = 'fixture' },
                @{ id = 'runtime'; path = 'runtime'; type = 'directory'; pattern = '*.dll'; minimumCount = 1; source = 'fixture' }
            )
            externalArtifacts = @()
            releaseFiles = @()
        } | ConvertTo-Json -Depth 8 | Set-Content $manifestPath
        & (Join-Path $toolsRoot 'check_offline_dependencies.ps1') -ProjectRoot $root -ManifestPath $manifestPath
        Assert-True ($LASTEXITCODE -eq 0) 'complete dependencies should pass'
        Remove-Item (Join-Path $root 'present.bin')
        & (Join-Path $toolsRoot 'check_offline_dependencies.ps1') -ProjectRoot $root -ManifestPath $manifestPath
        Assert-True ($LASTEXITCODE -ne 0) 'missing dependency should fail'
    }
    finally {
        Remove-Item $root -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Invoke-Test 'external update uses local fallback after network failure' {
    $root = New-TestRoot
    try {
        Set-Content -Path (Join-Path $root 'fallback.dll') -Value 'fallback'
        $manifestPath = Join-Path $root 'manifest.json'
        @{
            schemaVersion = 1
            dependencies = @()
            externalArtifacts = @(@{
                id = 'fixture'; url = 'https://127.0.0.1:1/unavailable'; fallbackPath = 'fallback.dll'; destination = 'out\artifact.dll'; archiveEntry = $null
            })
            releaseFiles = @()
        } | ConvertTo-Json -Depth 8 | Set-Content $manifestPath
        & (Join-Path $toolsRoot 'update_external_artifacts.ps1') -ProjectRoot $root -ManifestPath $manifestPath -TimeoutSec 1
        Assert-True ($LASTEXITCODE -eq 0) 'successful fallback should return zero'
        Assert-True ((Get-Content (Join-Path $root 'out\artifact.dll') -Raw).Trim() -eq 'fallback') 'destination should use fallback content'
    }
    finally {
        Remove-Item $root -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Invoke-Test 'release assembly copies and renames required files' {
    $root = New-TestRoot
    try {
        New-Item -ItemType Directory -Path (Join-Path $root 'x64\Release') -Force | Out-Null
        Set-Content -Path (Join-Path $root 'x64\Release\OptiScaler.dll') -Value 'dll'
        Set-Content -Path (Join-Path $root 'source.bin') -Value 'payload'
        $manifestPath = Join-Path $root 'manifest.json'
        @{
            schemaVersion = 1
            dependencies = @()
            externalArtifacts = @()
            releaseFiles = @(@{ source = 'source.bin'; destination = 'OptiScaler\renamed.bin'; type = 'file'; allowEmpty = $false; sourceDescription = 'fixture' })
        } | ConvertTo-Json -Depth 8 | Set-Content $manifestPath
        & (Join-Path $toolsRoot 'prepare_release.ps1') -ProjectRoot $root -ManifestPath $manifestPath -Configuration Release -Platform x64 -Offline
        Assert-True ($LASTEXITCODE -eq 0) 'release assembly should pass'
        Assert-True (Test-Path (Join-Path $root 'x64\Release\a\OptiScaler.dll')) 'main DLL should be in release root'
        Assert-True (Test-Path (Join-Path $root 'x64\Release\a\OptiScaler\renamed.bin')) 'manifest file should be renamed'
        Assert-True (Test-Path (Join-Path $root 'x64\Release\a\!! EXTRACT ALL FILES TO GAME FOLDER !!')) 'extract marker should exist'
    }
    finally {
        Remove-Item $root -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Invoke-Test 'MSBuild discovery prefers supplied D drive roots' {
    $root = New-TestRoot
    try {
        $msbuild = Join-Path $root 'D\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe'
        $toolset = Join-Path $root 'D\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.40.33807'
        New-Item -ItemType Directory -Path (Split-Path $msbuild -Parent) -Force | Out-Null
        New-Item -ItemType Directory -Path $toolset -Force | Out-Null
        Set-Content -Path $msbuild -Value 'fake'
        $result = & (Join-Path $toolsRoot 'find_msbuild.ps1') -SearchRoots @((Join-Path $root 'D')) -AsObject
        Assert-True ($result.MSBuildPath -eq $msbuild) 'MSBuild should be found under supplied root'
        Assert-True ($result.PlatformToolset -eq 'v143') 'VS 2022 should map to v143'
    }
    finally {
        Remove-Item $root -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Invoke-Test 'release build has one prepare release trigger' {
    $projectPath = Join-Path $projectRoot 'OptiScaler\OptiScaler.vcxproj'
    $project = Get-Content $projectPath -Raw
    Assert-True (([regex]::Matches($project, 'prepare_release\.ps1')).Count -eq 1) 'vcxproj must invoke prepare_release.ps1 exactly once'
    Assert-True ($project -notmatch '(?im)^\s*(copy|move|md|del)\s') 'Release PostBuild must not contain inline release assembly commands'
    $releaseCondition = "Condition=`"'`$(Configuration)|`$(Platform)'=='Release|x64'`""
    Assert-True ($project.Contains($releaseCondition)) 'prepare_release.ps1 must remain scoped to Release|x64'
}

Invoke-Test 'build entry only checks discovers and invokes MSBuild' {
    $buildPath = Join-Path $toolsRoot 'build.ps1'
    Assert-True (Test-Path $buildPath -PathType Leaf) 'build.ps1 must exist'
    $build = Get-Content $buildPath -Raw
    Assert-True ($build -match 'check_offline_dependencies\.ps1') 'build.ps1 must check offline dependencies'
    Assert-True ($build -match 'find_msbuild\.ps1') 'build.ps1 must discover MSBuild'
    Assert-True ($build -notmatch 'prepare_release\.ps1') 'build.ps1 must not invoke prepare_release.ps1'
    Assert-True ($build -notmatch '(?im)\b(Copy-Item|Move-Item|Remove-Item|New-Item)\b') 'build.ps1 must not assemble release files'
}

Invoke-Test 'script defaults do not depend on PSScriptRoot during parameter binding' {
    foreach ($name in @('build.ps1', 'build_settings.ps1', 'check_offline_dependencies.ps1', 'prepare_release.ps1', 'update_external_artifacts.ps1')) {
        $script = Get-Content (Join-Path $toolsRoot $name) -Raw
        $paramBlock = [regex]::Match($script, '(?s)param\((.*?)\)\s*\r?\n\s*\$ErrorActionPreference').Groups[1].Value
        Assert-True ($paramBlock -notmatch '\$PSScriptRoot') "$name uses PSScriptRoot in a parameter default"
    }
}

Invoke-Test 'menu keeps magnifier implementation and image section entry' {
    $menu = Get-Content (Join-Path $projectRoot 'OptiScaler\menu\menu_common.cpp') -Raw
    Assert-True ($menu -match 'void MenuCommon::RenderMagnifierSettings\(') 'Magnifier implementation is missing'
    Assert-True ($menu -match 'case 2:.*RenderMagnifierSettings\(ctx\)') 'Image section does not render Magnifier settings'
}

Invoke-Test 'menu navigation labels are translated at render time' {
    $menu = Get-Content (Join-Path $projectRoot 'OptiScaler\menu\menu_common.cpp') -Raw
    $sections = [regex]::Match($menu, 'static const char\* sections\[\]\s*=\s*\{(?<body>[\s\S]*?)\};').Groups['body'].Value
    Assert-True ($sections -notmatch 'Translation::Get') 'Navigation label is translated during static initialization'
    Assert-True ($menu -match 'Translation::Get\(sections\[i\]\)') 'Navigation labels are not translated during rendering'
}

Invoke-Test 'translation preserves ImGui ID suffixes' {
    $translation = Get-Content (Join-Path $projectRoot 'OptiScaler\translation\Translation.cpp') -Raw
    Assert-True ($translation -match 'key\.find\("##"\)') 'Translation does not inspect ImGui ID suffixes'
    Assert-True ($translation -match 'translated\.find\("##"\)') 'Translation does not preserve existing translated IDs'
    Assert-True ($translation -match 'translated\.append\(key, idPos') 'Translation does not append the source ImGui ID suffix'
}

Invoke-Test 'FSRFG routes Reflex through the non-NVIDIA low latency backend' {
    $reflex = Get-Content (Join-Path $projectRoot 'OptiScaler\hooks\Reflex_Hooks.cpp') -Raw
    Assert-True ($reflex -match 'ShouldUseFakenvapiReflex\s*\(') 'Reflex routing helper is missing'
    Assert-True ($reflex -match 'activeFgOutput\s*==\s*FGOutput::FSRFG') 'FSRFG is not included in Reflex routing'
    Assert-True ($reflex -match 'vendorId\s*!=\s*VendorId::Nvidia') 'Reflex routing is not limited to non-NVIDIA GPUs'
    foreach ($call in @('SetSleepMode', 'Sleep', 'GetLatency', 'SetLatencyMarker', 'SetAsyncFrameMarker')) {
        Assert-True ($reflex -match "ShouldUseFakenvapiReflex\(\)[\s\S]{0,240}nvapi_calls::NvAPI_D3D(?:12)?_$call") "$call does not use the unified Reflex routing decision"
    }
}

Invoke-Test 'FSRFG auto low latency avoids Anti-Lag 2 on non-NVIDIA GPUs' {
    $lowLatency = Get-Content (Join-Path $projectRoot 'OptiScaler\nvapi\fakenvapi\low_latency_d3d.cpp') -Raw
    Assert-True ($lowLatency -match 'fsrfgLatencyFlexFallback\s*=') 'FSRFG LatencyFlex fallback decision is missing'
    Assert-True ($lowLatency -match 'activeFgOutput\s*==\s*FGOutput::FSRFG') 'FSRFG output is not part of the fallback decision'
    Assert-True ($lowLatency -match 'vendorId\s*!=\s*VendorId::Nvidia') 'Fallback is not limited to non-NVIDIA GPUs'
    Assert-True ($lowLatency -match 'force_latencyflex\s*=[\s\S]{0,200}fsrfgLatencyFlexFallback') 'FSRFG fallback does not affect backend selection'
    Assert-True ($lowLatency -match 'last_fsrfg_latencyflex_fallback\s*!=\s*fsrfgLatencyFlexFallback') 'Runtime FSRFG transitions do not reload the backend'
}

Invoke-Test 'Anti-Lag 2 PRESENT_START only marks end of frame while effective FG is enabled' {
    $source = Get-Content (Join-Path $projectRoot 'OptiScaler\low_latency\low_latency_tech\ll_antilag2.cpp') -Raw
    $presentStart = [regex]::Match(
        $source,
        'case\s+MarkerType::PRESENT_START:\s*\{(?<body>[\s\S]*?)\n\s*break;\s*\n\s*\}')
    Assert-True $presentStart.Success 'PRESENT_START branch is missing'
    $body = $presentStart.Groups['body'].Value
    Assert-True (([regex]::Matches($body, 'AMD::AntiLag2DX12::MarkEndOfFrameRendering')).Count -eq 1) 'PRESENT_START must contain exactly one EndFrame call'
    $gate = [regex]::Match($body, 'if\s*\(\s*!\s*effective_fg_state\s*\)\s*\r?\n?\s*return\s*;')
    Assert-True $gate.Success 'PRESENT_START must return early when effective_fg_state is false'
    Assert-True ($gate.Index -lt $body.IndexOf('AMD::AntiLag2DX12::MarkEndOfFrameRendering')) 'the effective_fg_state gate must precede the EndFrame call'
}

Invoke-Test 'Anti-Lag 2 disables pacing fence during FSRFG interpolation' {
    $source = Get-Content (Join-Path $projectRoot 'OptiScaler\low_latency\low_latency_tech\ll_antilag2.cpp') -Raw
    $sleepFn = [regex]::Match($source, '(?ms)inline\s+HRESULT\s+AntiLag2::al2_sleep\(\)\s*\{(?<body>.+?)^\}').Groups['body'].Value
    Assert-True (-not [string]::IsNullOrEmpty($sleepFn)) 'al2_sleep function body not found'
    $dx12Update = [regex]::Match($sleepFn, 'AMD::AntiLag2DX12::Update\s*\(\s*&dx12_ctx\s*,\s*(?<enabled>[^,]+?)\s*,\s*max_fps\s*\)').Groups['enabled'].Value
    Assert-True ($dx12Update -match 'is_enabled\(\)\s*&&\s*!\s*effective_fg_state') 'DX12 Update must pass is_enabled() && !effective_fg_state to disable pacing during FSRFG'
    $dx11Update = [regex]::Match($sleepFn, 'AMD::AntiLag2DX11::Update\s*\(\s*&dx11_ctx\s*,\s*(?<enabled>[^,]+?)\s*,\s*max_fps\s*\)').Groups['enabled'].Value
    Assert-True ($dx11Update -match 'is_enabled\(\)\s*&&\s*!\s*effective_fg_state') 'DX11 Update must pass is_enabled() && !effective_fg_state to disable pacing during FSRFG'
}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Error $_ -ErrorAction Continue }
    exit 1
}

Write-Host "All $($failures.Count + 13) tests passed"
exit 0
