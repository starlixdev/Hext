param([string]$Executable='Hext-preview.exe')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
$taskVersion=(Get-Content (Join-Path $taskRoot 'VERSION') -Raw).Trim()
$taskStage=Join-Path $taskRoot ("release/package-$taskVersion-"+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $taskStage -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $taskRoot "app/dist/$Executable") -Destination (Join-Path $taskStage 'Hext.exe') -Force
foreach($taskFile in @('README.md','LICENSE','THIRD_PARTY_NOTICES.md')){Copy-Item -LiteralPath (Join-Path $taskRoot $taskFile) -Destination $taskStage -Force}
Copy-Item -LiteralPath (Join-Path $taskRoot 'docs') -Destination $taskStage -Recurse -Force
$taskZip=Join-Path $taskRoot 'release/Hext-Windows-x64.zip'
Compress-Archive -LiteralPath (Get-ChildItem -LiteralPath $taskStage).FullName -DestinationPath $taskZip -Force
((Get-FileHash -LiteralPath $taskZip).Hash.ToLowerInvariant()+'  Hext-Windows-x64.zip') | Set-Content (Join-Path $taskRoot 'release/SHA256SUMS.txt') -Encoding ascii
Get-Item -LiteralPath $taskZip | Select-Object Name,Length
