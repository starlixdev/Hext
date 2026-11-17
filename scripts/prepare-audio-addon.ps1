param([string]$OutputDirectory)
$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
if(-not $OutputDirectory){$OutputDirectory=Join-Path $taskRoot 'release/audio'}
$taskOutput=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $taskOutput -Force | Out-Null
$taskManifest=Get-Content (Join-Path $taskRoot 'addons/audio/upstream.json') -Raw | ConvertFrom-Json
foreach($taskAsset in @($taskManifest.installer,$taskManifest.source)){
    $taskURL=[Uri]$taskAsset.url
    if($taskURL.Scheme -ne 'https' -or $taskURL.Host -ne 'github.com' -or -not $taskURL.AbsolutePath.StartsWith('/115dkk/EqualizerAPO-XT/releases/download/')){throw 'Unapproved upstream asset URL'}
    $taskFile=Join-Path $taskOutput $taskAsset.name
    if(-not(Test-Path -LiteralPath $taskFile)){
        Invoke-WebRequest -Uri $taskAsset.url -OutFile $taskFile -Headers @{'User-Agent'='Hext-release'}
    }
    $taskDigest=(Get-FileHash -LiteralPath $taskFile -Algorithm SHA256).Hash.ToLowerInvariant()
    if($taskDigest -ne $taskAsset.sha256){throw "Checksum mismatch: $($taskAsset.name)"}
    if($taskAsset.size -and (Get-Item -LiteralPath $taskFile).Length -ne $taskAsset.size){throw 'Installer size mismatch'}
}
Copy-Item -LiteralPath (Join-Path $taskRoot 'addons/audio/LICENSE-GPL-2.0.txt') -Destination $taskOutput -Force
Copy-Item -LiteralPath (Join-Path $taskRoot 'addons/audio/LICENSE-GPL-3.0.txt') -Destination $taskOutput -Force
Copy-Item -LiteralPath (Join-Path $taskRoot 'addons/audio/upstream.json') -Destination $taskOutput -Force
$taskNotes=@"
Hext Audio reuses the unmodified EqualizerAPO-XT $($taskManifest.version) x64 SSE2 installer.
Upstream: https://github.com/$($taskManifest.upstream)
Source commit: $($taskManifest.commit)
The installer is copied byte-for-byte; its internal branding and copyright are unchanged.
The matching source ZIP and original licenses accompany this installer.
Download and installation are separate actions in Hext. Device setup may require Windows permission and an audio restart.
"@
Set-Content -LiteralPath (Join-Path $taskOutput 'NOTICE.txt') -Value $taskNotes -Encoding utf8
Get-ChildItem -LiteralPath $taskOutput -File | Where-Object {$_.Name -ne 'SHA256SUMS.txt'} | ForEach-Object {
    ((Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()+'  '+$_.Name)
} | Set-Content -LiteralPath (Join-Path $taskOutput 'SHA256SUMS.txt') -Encoding ascii
Get-ChildItem -LiteralPath $taskOutput | Select-Object Name,Length
