@echo off
REM Builds then pushes (all) to itch.io. Usage: [version]
call "%~dp0BuildAndUpload_Itch.bat" all %1
exit /b %errorlevel%
