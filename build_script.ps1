$env:TMP = "j:\OptiScaler-master-0.10-2\tmp"
$env:TEMP = "j:\OptiScaler-master-0.10-2\tmp"
Remove-Item -Recurse -Force "j:\OptiScaler-master-0.10-2\OptiScaler\x64" -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force "j:\OptiScaler-master-0.10-2\x64" -ErrorAction SilentlyContinue
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\amd64\MSBuild.exe"
$psi.Arguments = '"j:\OptiScaler-master-0.10-2\OptiScaler\OptiScaler.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:SolutionDir="j:\OptiScaler-master-0.10-2\" /t:Rebuild /m /nologo /v:minimal'
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.WorkingDirectory = "j:\OptiScaler-master-0.10-2"
$p = [System.Diagnostics.Process]::Start($psi)
$stdout = $p.StandardOutput.ReadToEnd()
$stderr = $p.StandardError.ReadToEnd()
$p.WaitForExit()
$stdout | Out-File "j:\OptiScaler-master-0.10-2\build_full_out.txt" -Encoding UTF8
$stderr | Out-File "j:\OptiScaler-master-0.10-2\build_full_err.txt" -Encoding UTF8
Write-Host "ExitCode: $($p.ExitCode)"
