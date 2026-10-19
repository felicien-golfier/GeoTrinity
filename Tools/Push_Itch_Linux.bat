@echo off
REM Pushes Build\GeoTrinity_Linux.zip to the itch.io linux channel if newer than itch's. Usage: [version]
call "%~dp0Push_Itch.bat" linux %1
exit /b %errorlevel%
