@echo off
REM Pushes the existing Build\GeoTrinity.zip (windows) and Build\GeoTrinity_Linux.zip (linux) to itch.io
REM (https://exyoe.itch.io/geotrinity) with butler, without building. A channel is pushed only when its zip is
REM newer than the channel's last build on itch; a missing zip is skipped.
REM Usage: Tools\Push_Itch.bat [windows|linux|all (default)] [version shown to players, e.g. 0.4.0]
REM Wrappers: Push_Itch_Windows.bat, Push_Itch_Linux.bat.
REM Ends on a result line and waits for a key, unless CI or GEO_NO_PAUSE is set.
REM GEO_DRY_RUN=1 prints the pushes instead of running them. GEO_FORCE_PUSH=1 skips the newer-than-itch check.
setlocal
set "RUN="
if defined GEO_DRY_RUN set "RUN=call :dry_run"
for %%i in ("%~dp0..") do set "REPO=%%~fi"
set "GAME=exyoe/geotrinity"
set "WHICH=%~1"
if not defined WHICH set "WHICH=all"
set "VERSION=%~2"
set "RC=1"
set "PUSHED=0"

where butler >nul 2>nul
if errorlevel 1 (
  echo butler is not on PATH. Install it from https://itch.io/docs/butler/installing.html and run "butler login" once.
  goto :finish
)

if /i not "%WHICH%"=="windows" if /i not "%WHICH%"=="linux" if /i not "%WHICH%"=="all" (
  echo Unknown channel "%WHICH%". Use windows, linux or all.
  goto :finish
)

set "RC=0"
if /i not "%WHICH%"=="linux" (
  call :push windows "%REPO%\Build\GeoTrinity.zip"
  if not "%RC%"=="0" goto :finish
)
if /i not "%WHICH%"=="windows" call :push linux "%REPO%\Build\GeoTrinity_Linux.zip"

:finish
echo.
if "%RC%"=="0" (
  echo ==== %~n0: SUCCEEDED ^(%PUSHED% channel^(s^) pushed^) ====
) else (
  echo ==== %~n0: FAILED ^(exit code %RC%^) ====
)
if not defined CI if not defined GEO_NO_PAUSE pause
exit /b %RC%

REM :push <channel> <zip> - pushes one channel when its zip exists and is newer than itch's build.
:push
if not exist "%~2" (
  echo [%1] no zip at "%~2"; skipping.
  exit /b 0
)
if not defined GEO_FORCE_PUSH (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Itch_IsNewer.ps1" "%~2" %GAME% %1
  if errorlevel 1 exit /b 0
)
if "%VERSION%"=="" (
  %RUN% butler push "%~2" %GAME%:%1
) else (
  %RUN% butler push "%~2" %GAME%:%1 --userversion "%VERSION%"
)
set "RC=%errorlevel%"
if not "%RC%"=="0" exit /b %RC%
set /a PUSHED+=1
if not defined GEO_DRY_RUN butler status %GAME%:%1
exit /b 0

:dry_run
echo [dry run] %*
exit /b 0
