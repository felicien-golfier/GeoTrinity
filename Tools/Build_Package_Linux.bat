@echo off
REM Packages the Linux client (cross-compiled on Windows) with Build_Package.bat Linux.
REM Output: Build\GeoTrinity_Linux and Build\GeoTrinity_Linux.zip. Needs the toolchain from AI\BuildPackage.md.
REM Ends on a result line and waits for a key, unless CI or GEO_NO_PAUSE is set.
REM GEO_DRY_RUN=1 prints every step that builds or changes files instead of running it.
call "%~dp0Build_Package.bat" Linux
exit /b %errorlevel%
