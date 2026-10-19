@echo off
REM Packages the Windows client, then the Linux client (stops if Windows fails).
REM Ends on a result line and waits for a key, unless CI or GEO_NO_PAUSE is set.
REM GEO_DRY_RUN=1 walks both builds without building.
setlocal
set "PAUSE_AT_END=1"
if defined CI set "PAUSE_AT_END="
if defined GEO_NO_PAUSE set "PAUSE_AT_END="
set "GEO_NO_PAUSE=1"

call "%~dp0Build_Package.bat" Win64
set "RC=%errorlevel%"
if not "%RC%"=="0" goto :finish
call "%~dp0Build_Package.bat" Linux
set "RC=%errorlevel%"

:finish
echo.
if "%RC%"=="0" (
  echo ==== %~n0: SUCCEEDED ====
) else (
  echo ==== %~n0: FAILED ^(exit code %RC%^) ====
)
if defined PAUSE_AT_END pause
exit /b %RC%
