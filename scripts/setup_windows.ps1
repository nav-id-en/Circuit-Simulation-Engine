$ErrorActionPreference = "Stop"

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$msysRoot = "C:\msys64"
$bash = Join-Path $msysRoot "usr\bin\bash.exe"

Write-Host "ProteusLab SDL Windows setup" -ForegroundColor Cyan
Write-Host "Project: $projectRoot"

if (-not (Test-Path $bash)) {
    if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
        throw "MSYS2 is not installed and winget is unavailable. Install MSYS2 from https://www.msys2.org and run this script again."
    }
    Write-Host "Installing MSYS2..." -ForegroundColor Yellow
    winget install --id MSYS2.MSYS2 -e --source winget --accept-package-agreements --accept-source-agreements
    if ($LASTEXITCODE -ne 0) {
        throw "winget could not install MSYS2 (exit code $LASTEXITCODE)."
    }
}

if (-not (Test-Path $bash)) {
    throw "MSYS2 was installed but $bash was not found. Restart Windows and run the script again."
}

$packages = @(
    "mingw-w64-ucrt-x86_64-gcc",
    "mingw-w64-ucrt-x86_64-cmake",
    "mingw-w64-ucrt-x86_64-ninja",
    "mingw-w64-ucrt-x86_64-SDL2",
    "mingw-w64-ucrt-x86_64-pkgconf"
) -join " "

Write-Host "Installing C++ compiler, CMake, Ninja and SDL2..." -ForegroundColor Yellow
& $bash -lc "pacman -Sy --needed --noconfirm $packages"
if ($LASTEXITCODE -ne 0) {
    throw "MSYS2 package installation failed (exit code $LASTEXITCODE)."
}

$cmake = Join-Path $msysRoot "ucrt64\bin\cmake.exe"
$compiler = Join-Path $msysRoot "ucrt64\bin\g++.exe"
$sdl = Join-Path $msysRoot "ucrt64\bin\SDL2.dll"

if (-not (Test-Path $cmake)) { throw "CMake was not installed correctly." }
if (-not (Test-Path $compiler)) { throw "G++ was not installed correctly." }
if (-not (Test-Path $sdl)) { throw "SDL2 was not installed correctly." }

Write-Host ""
Write-Host "Setup completed successfully." -ForegroundColor Green
Write-Host "Run the project with:"
Write-Host "  scripts\run_windows.bat" -ForegroundColor Cyan
