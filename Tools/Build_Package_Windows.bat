@echo off
REM Packages the Windows client with Build_Package.bat Win64. Output: Build\GeoTrinity and Build\GeoTrinity.zip.
REM Ends on a result line and waits for a key, unless CI or GEO_NO_PAUSE is set.
REM GEO_DRY_RUN=1 prints every step that builds or changes files instead of running it.
call "%~dp0Build_Package.bat" Win64
exit /b %errorlevel%
