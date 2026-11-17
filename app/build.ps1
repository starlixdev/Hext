param([switch]$Preview)
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Path '.\dist' -Force | Out-Null
    & windres '.\app.rc' -O coff -o '.\dist\app-res.o'
    if ($LASTEXITCODE -ne 0) { throw 'Could not compile application resources.' }
    $taskOutput = if ($Preview) { '.\dist\Hext-preview.exe' } else { '.\dist\Hext.exe' }
    & g++ '.\src\main.cpp' '.\dist\app-res.o' -o $taskOutput -std=c++17 -O2 -s -Wall -Wextra -Wno-missing-field-initializers -municode -mwindows -static -static-libgcc -static-libstdc++ -ld2d1 -ldwrite -ldwmapi -lwindowscodecs -ladvapi32 -lole32 -loleaut32 -loleacc -luuid -lgdi32 -luser32 -lshell32 -lwinmm -lcomdlg32 -lwbemuuid -lpdh -lbthprops -lwinhttp -lbcrypt
    if ($LASTEXITCODE -ne 0) { throw 'The build failed.' }
    Get-Item -LiteralPath $taskOutput | Select-Object FullName,Length
} finally {
    Pop-Location
}



