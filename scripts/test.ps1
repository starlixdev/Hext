param([switch]$SkipNativeHotkeys)
$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
Push-Location (Join-Path $taskRoot 'app')
try {
    New-Item -ItemType Directory -Path dist -Force | Out-Null
    $taskTests=@('motion-test','spring-profiles-test','visibility-test','desktop-region-test','presentation-test','color-test','material-test','hardware-test','assistant-test','sound-test','shortcuts-test','addon-test')
    foreach($taskName in $taskTests){
        $taskArgs=@("tests/$taskName.cpp",'-o',"dist/$taskName.exe",'-std=c++17','-O2','-Wall','-Wextra','-Wno-missing-field-initializers','-static','-ld2d1','-ldwrite','-lwindowscodecs','-ladvapi32','-lole32','-loleaut32','-luuid','-lgdi32','-luser32','-lshell32','-lwbemuuid','-lpdh','-lbthprops','-lwinhttp','-lbcrypt')
        if($taskName -eq 'assistant-test'){$taskArgs+='-municode'}
        & g++ @taskArgs
        if($LASTEXITCODE -ne 0){throw "Could not compile $taskName"}
        if($SkipNativeHotkeys -and $taskName -eq 'shortcuts-test'){& "./dist/$taskName.exe" '--skip-native'}
        else{& "./dist/$taskName.exe"}
        if($LASTEXITCODE -ne 0){throw "$taskName failed"}
    }
} finally {Pop-Location}
