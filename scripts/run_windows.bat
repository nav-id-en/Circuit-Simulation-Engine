@echo off
setlocal

set "PROJECT_ROOT=%~dp0.."
call "%~dp0build_windows.bat"
if errorlevel 1 exit /b 1

cd /d "%PROJECT_ROOT%"
start "" "%PROJECT_ROOT%\build-windows\ProteusLabSDL.exe"
endlocal
