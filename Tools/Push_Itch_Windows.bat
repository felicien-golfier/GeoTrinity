@echo off
REM Pushes Build\GeoTrinity.zip to the itch.io windows channel if newer than itch's. Usage: [version]
call "%~dp0Push_Itch.bat" windows %1
exit /b %errorlevel%
