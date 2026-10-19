@echo off
REM Builds then pushes (linux) to itch.io. Usage: [version]
call "%~dp0BuildAndUpload_Itch.bat" linux %1
exit /b %errorlevel%
