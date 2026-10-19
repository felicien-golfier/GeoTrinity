@echo off
REM Packages then pushes to itch.io. Usage: Tools\BuildAndUpload_Itch.bat [windows|linux|all (default)] [version, e.g. 0.4.0]
REM Wrappers: BuildAndUpload_Itch_Windows.bat, _Linux.bat, _All.bat. Push is skipped per channel when itch is up to date.
REM Ends on a result line and waits for a key, unless CI or GEO_NO_PAUSE is set.
REM GEO_DRY_RUN=1 walks both steps without building, touching Build\ or pushing.
setlocal
set "WHICH=%~1"
if not defined WHICH set "WHICH=all"
REM The steps skip their own pause so the window waits once, at the very end.
set "PAUSE_AT_END=1"
if defined CI set "PAUSE_AT_END="
if defined GEO_NO_PAUSE set "PAUSE_AT_END="
set "GEO_NO_PAUSE=1"

if /i "%WHICH%"=="windows" (
  call "%~dp0Build_Package_Windows.bat"
) else if /i "%WHICH%"=="linux" (
  call "%~dp0Build_Package_Linux.bat"
) else if /i "%WHICH%"=="all" (
  call "%~dp0Build_Package_All.bat"
) else (
  echo Unknown channel "%WHICH%". Use windows, linux or all.
  set "RC=1"
  goto :finish
)
set "RC=%errorlevel%"
if not "%RC%"=="0" goto :finish

call "%~dp0Push_Itch.bat" %WHICH% %2
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
