param([switch]$SkipNativeHotkeys)
$ErrorActionPreference='Stop'
& (Join-Path $PSScriptRoot 'dependencies.ps1')
$taskRoot=Split-Path $PSScriptRoot -Parent
Push-Location (Join-Path $taskRoot 'app')
try {
    New-Item -ItemType Directory -Path dist -Force | Out-Null
    & g++ tests/assistant-stream-child.cpp -o dist/assistant-stream-child.exe -std=c++17 -O2 -static
    if($LASTEXITCODE -ne 0){throw 'Could not compile structured-stream lifecycle fixture'}
    $taskTests=@('motion-test','spring-profiles-test','visibility-test','desktop-region-test','presentation-test','color-test','material-test','hardware-test','assistant-test','sound-test','shortcuts-test','personal-test','task-engine-test')
    foreach($taskName in $taskTests){
        $taskArgs=@("tests/$taskName.cpp",'-o',"dist/$taskName.exe",'-std=c++17','-O2','-Wall','-Wextra','-Wno-missing-field-initializers','-static','-ld2d1','-ldwrite','-lwindowscodecs','-ladvapi32','-lole32','-loleaut32','-luuid','-lgdi32','-luser32','-lshell32','-lwbemuuid','-lpdh','-lbthprops')
        if($taskName -eq 'assistant-test'){$taskArgs+='-municode'}
        if($taskName -eq 'personal-test' -or $taskName -eq 'task-engine-test'){$taskArgs+=@('deps/sqlite3.o','-lcrypt32','-lbcrypt','-lwinhttp','-lws2_32')}
        & g++ @taskArgs
        if($LASTEXITCODE -ne 0){throw "Could not compile $taskName"}
        if($SkipNativeHotkeys -and $taskName -eq 'shortcuts-test'){& "./dist/$taskName.exe" '--skip-native'}
        else{& "./dist/$taskName.exe"}
        if($LASTEXITCODE -ne 0){throw "$taskName failed"}
    }
    & g++ tests/restart-store-test.cpp deps/sqlite3.o -o dist/restart-store-test.exe -std=c++17 -O2 -Wall -Wextra -municode -static -ladvapi32 -lcrypt32 -lbcrypt
    if($LASTEXITCODE -ne 0){throw 'Could not compile restart-store-test'}
    $taskRestart=Join-Path (Get-Location).Path ('dist/restart-'+[guid]::NewGuid().ToString('N'))
    & ./dist/restart-store-test.exe save $taskRestart
    if($LASTEXITCODE -ne 0){throw 'Could not save restart fixture'}
    & ./dist/restart-store-test.exe load $taskRestart
    if($LASTEXITCODE -ne 0){throw 'Fresh-process persistence verification failed'}
} finally {Pop-Location}
