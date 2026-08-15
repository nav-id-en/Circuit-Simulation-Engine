@echo off
setlocal

set "PROJECT_ROOT=%~dp0.."
set "BUILD_DIR=%PROJECT_ROOT%\build-windows"
set "DIST_DIR=%PROJECT_ROOT%\dist\ProteusLabSDL"
set "ZIP_PATH=%PROJECT_ROOT%\dist\ProteusLabSDL_Windows.zip"

call "%~dp0build_windows.bat"
if errorlevel 1 exit /b 1

if exist "%DIST_DIR%" rmdir /S /Q "%DIST_DIR%"
mkdir "%DIST_DIR%"
copy /Y "%BUILD_DIR%\ProteusLabSDL.exe" "%DIST_DIR%\" >nul
for %%D in (SDL2.dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll) do (
    copy /Y "%BUILD_DIR%\%%D" "%DIST_DIR%\" >nul
    if errorlevel 1 exit /b 1
)
xcopy /E /I /Y "%PROJECT_ROOT%\examples" "%DIST_DIR%\examples" >nul
xcopy /E /I /Y "%PROJECT_ROOT%\firmware" "%DIST_DIR%\firmware" >nul
xcopy /E /I /Y "%PROJECT_ROOT%\docs" "%DIST_DIR%\docs" >nul
copy /Y "%PROJECT_ROOT%\README.md" "%DIST_DIR%\" >nul
copy /Y "%PROJECT_ROOT%\README_FA.md" "%DIST_DIR%\" >nul

powershell -NoProfile -Command ^
    "if (Test-Path -LiteralPath '%ZIP_PATH%') { Remove-Item -LiteralPath '%ZIP_PATH%' -Force }; Compress-Archive -LiteralPath '%DIST_DIR%' -DestinationPath '%ZIP_PATH%'"
if errorlevel 1 exit /b 1

echo [OK] Package created:
echo "%ZIP_PATH%"
endlocal
