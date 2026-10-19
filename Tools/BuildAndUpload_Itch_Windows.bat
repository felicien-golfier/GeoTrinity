@echo off
REM Builds then pushes (windows) to itch.io. Usage: [version]
call "%~dp0BuildAndUpload_Itch.bat" windows %1
exit /b %errorlevel%
