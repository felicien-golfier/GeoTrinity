@echo off
REM Packages the client with Build_Package.bat, then pushes it to itch.io with Push_Itch.bat.
REM Usage: Tools\BuildAndUpload_Itch.bat [version shown to players, e.g. 0.4.0]
REM Ends on a result line and waits for a key, unless CI or GEO_NO_PAUSE is set.
REM GEO_DRY_RUN=1 walks both steps without building, touching Build\ or pushing.
setlocal
REM The two steps skip their own pause so the window waits once, at the very end.
set "PAUSE_AT_END=1"
if defined CI set "PAUSE_AT_END="
if defined GEO_NO_PAUSE set "PAUSE_AT_END="
set "GEO_NO_PAUSE=1"

call "%~dp0Build_Package.bat"
set "RC=%errorlevel%"
if not "%RC%"=="0" goto :finish

call "%~dp0Push_Itch.bat" %1
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
