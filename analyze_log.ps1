$logPath = 'F:\gamebox\downs\v2.31a.Steam\v2.31a.Steam\bin\x64\OptiScaler.log'
$matches = Select-String -Path $logPath -Pattern 'FSRDFeatureDx12::Evaluate', 'skipping upscaler dispatch', 'FSR Ray Regeneration Initialized', 'InitFSR3 FSR Ray Regeneration', 'ChangeFeature', 'TryCreateOptiFeature', 'Error', 'skipping dispatch', 'NVSDK_NGX_D3D12_ReleaseFeature', 'Frametime: 19', 'Frametime: 0\.00', 'nvngx_dlssg', 'Upscaler failed', 'dispatch result=[^0]'
Write-Host "Total matches: $($matches.Count)"
$matches | Select-Object -Last 30 | ForEach-Object { '{0}: {1}' -f $_.LineNumber, $_.Line }
