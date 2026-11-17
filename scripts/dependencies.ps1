$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskDeps=Join-Path $taskRoot 'app/deps'
New-Item -ItemType Directory -Path $taskDeps -Force | Out-Null
$taskFiles=@(
    @{Name='json.hpp';URL='https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp';Hash='aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63'},
    @{Name='sqlite.zip';URL='https://www.sqlite.org/2026/sqlite-amalgamation-3540100.zip';Hash='0a9c8e34352dad46006aa82836d5383bff9dc6bc9ce5a60553c05aa30e6ad1f9'}
)
foreach($taskFile in $taskFiles){
    $taskTarget=Join-Path $taskDeps $taskFile.Name
    if(-not (Test-Path -LiteralPath $taskTarget) -or (Get-FileHash -LiteralPath $taskTarget -Algorithm SHA256).Hash -ne $taskFile.Hash){
        Invoke-WebRequest -Uri $taskFile.URL -OutFile $taskTarget
    }
    if((Get-FileHash -LiteralPath $taskTarget -Algorithm SHA256).Hash -ne $taskFile.Hash){throw 'Dependency digest mismatch'}
}
if(-not (Test-Path -LiteralPath (Join-Path $taskDeps 'sqlite-amalgamation-3540100/sqlite3.c'))){
    Expand-Archive -LiteralPath (Join-Path $taskDeps 'sqlite.zip') -DestinationPath $taskDeps -Force
}
$taskSource=Join-Path $taskDeps 'sqlite-amalgamation-3540100/sqlite3.c'
$taskObject=Join-Path $taskDeps 'sqlite3.o'
if(-not (Test-Path -LiteralPath $taskObject) -or (Get-Item -LiteralPath $taskSource).LastWriteTime -gt (Get-Item -LiteralPath $taskObject).LastWriteTime){
    & gcc $taskSource -c -o $taskObject -O2 -DSQLITE_THREADSAFE=1 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DQS=0 -DSQLITE_DEFAULT_MEMSTATUS=0
    if($LASTEXITCODE -ne 0){throw 'Could not compile SQLite'}
}
