param([string]$Codex='codex.exe')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$taskProbe=Join-Path $taskRoot ('app/dist/boundary-'+[guid]::NewGuid().ToString('N'))
$taskWork=Join-Path $taskProbe 'work'
New-Item -ItemType Directory -Path $taskWork -Force | Out-Null
$taskSource=Join-Path $PSScriptRoot 'provider-boundary-child.cpp'
$taskChild=Join-Path $taskWork 'probe.exe'
& g++ $taskSource -o $taskChild -O2 -std=c++17 -municode -static
if($LASTEXITCODE -ne 0){throw 'Boundary helper build failed'}
$taskPrivate=Join-Path $taskProbe 'private.txt'
[IO.File]::WriteAllText($taskPrivate,'Only a generated test canary; no user data.')
$taskWrite=Join-Path $taskWork 'forbidden-write.txt'
$taskCommon=@('-c','features.prefer_mxc=true','-c','windows.sandbox="elevated"','-c','default_permissions="hext"','-c','permissions.hext.filesystem={":root"="deny",":minimal"="read",":workspace_roots"="read"}','-c','permissions.hext.network.enabled=false','sandbox','--include-managed-config','--permission-profile','hext','--')
Push-Location $taskWork
try {
    $taskReadOutput=& $Codex @taskCommon $taskChild read $taskPrivate 2>&1
    if($LASTEXITCODE -ne 10 -or ($taskReadOutput -join ' ') -notmatch 'DENIED 5'){throw ('Read boundary not verified: '+($taskReadOutput -join ' '))}
    'PASS: actual sandboxed native process cannot read the generated file outside its workspace.'
    $taskWriteOutput=& $Codex @taskCommon $taskChild write $taskWrite 2>&1
    if($LASTEXITCODE -ne 10 -or ($taskWriteOutput -join ' ') -notmatch 'DENIED 5' -or (Test-Path -LiteralPath $taskWrite)){throw ('Write boundary not verified: '+($taskWriteOutput -join ' '))}
    'PASS: actual sandboxed native process cannot create a file inside its read-only workspace.'
} finally {Pop-Location}
