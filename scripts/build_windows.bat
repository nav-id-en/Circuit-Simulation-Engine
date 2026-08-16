@echo off
setlocal EnableExtensions

set "PROJECT_ROOT=%~dp0.."
set "MSYS_ROOT=C:\msys64"
set "UCRT_BIN=C:\msys64\ucrt64\bin"
set "MSYS_BIN=C:\msys64\usr\bin"
set "CMAKE_EXE=%UCRT_BIN%\cmake.exe"
set "CTEST_EXE=%UCRT_BIN%\ctest.exe"
set "NINJA_EXE=%UCRT_BIN%\ninja.exe"
set "CC_EXE=%UCRT_BIN%\gcc.exe"
set "CXX_EXE=%UCRT_BIN%\g++.exe"
set "BUILD_DIR=%PROJECT_ROOT%\build-windows"

if not exist "%CMAKE_EXE%" (
    echo [ERROR] CMake/MSYS2 is not installed.
    echo Run this first:
    echo powershell -ExecutionPolicy Bypass -File scripts\setup_windows.ps1
    exit /b 1
)

if not exist "%NINJA_EXE%" (
    echo [ERROR] UCRT64 Ninja is missing.
    echo Run scripts\setup_windows.ps1 again.
    exit /b 1
)

if not exist "%CC_EXE%" (
    echo [ERROR] UCRT64 GCC is missing.
    echo Run scripts\setup_windows.ps1 again.
    exit /b 1
)

if not exist "%CXX_EXE%" (
    echo [ERROR] UCRT64 G++ is missing.
    echo Run scripts\setup_windows.ps1 again.
    exit /b 1
)

if not exist "%UCRT_BIN%\SDL2.dll" (
    echo [ERROR] SDL2.dll is missing.
    echo Run scripts\setup_windows.ps1 again.
    exit /b 1
)

rem Keep one coherent toolchain. A different MinGW installation such as
rem C:\mingw64 must never supply the compiler or runtime DLLs for this build.
set "PATH=%UCRT_BIN%;%MSYS_BIN%;%PATH%"
set "CC=%CC_EXE%"
set "CXX=%CXX_EXE%"

echo [INFO] Compiler: %CXX_EXE%
echo [INFO] SDL2 root: %MSYS_ROOT%\ucrt64

rem --fresh discards only CMake's generated cache. This also repairs a build
rem directory that was previously configured with C:\mingw64.
"%CMAKE_EXE%" --fresh -S "%PROJECT_ROOT%" -B "%BUILD_DIR%" -G Ninja ^
    "-DCMAKE_MAKE_PROGRAM:FILEPATH=%NINJA_EXE%" ^
    "-DCMAKE_CXX_COMPILER:FILEPATH=%CXX_EXE%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64 ^
    -DPROTEUS_BUILD_GUI=ON ^
    -DPROTEUS_BUILD_TESTS=ON
if errorlevel 1 exit /b 1

"%CMAKE_EXE%" --build "%BUILD_DIR%" --parallel
if errorlevel 1 exit /b 1

for %%D in (SDL2.dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll) do (
    if not exist "%UCRT_BIN%\%%D" (
        echo [ERROR] Required UCRT64 runtime is missing: %%D
        echo Run scripts\setup_windows.ps1 again.
        exit /b 1
    )
    copy /Y "%UCRT_BIN%\%%D" "%BUILD_DIR%\%%D" >nul
    if errorlevel 1 exit /b 1
)

"%CTEST_EXE%" --test-dir "%BUILD_DIR%" --output-on-failure
if errorlevel 1 exit /b 1

echo.
echo [OK] SDL2 build and all tests completed.
echo Executable: "%BUILD_DIR%\ProteusLabSDL.exe"
endlocal
