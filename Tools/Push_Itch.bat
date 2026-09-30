@echo off
REM Pushes the existing Build\GeoTrinity.zip to itch.io (https://exyoe.itch.io/geotrinity) with butler, without building.
REM Usage: Tools\Push_Itch.bat [version shown to players, e.g. 0.4.0]
REM Ends on a result line and waits for a key, unless CI or GEO_NO_PAUSE is set.
REM GEO_DRY_RUN=1 prints the push instead of running it.
setlocal
set "RUN="
if defined GEO_DRY_RUN set "RUN=call :dry_run"
for %%i in ("%~dp0..") do set "REPO=%%~fi"
set "TARGET=exyoe/geotrinity:windows"
set "RC=1"

where butler >nul 2>nul
if errorlevel 1 (
  echo butler is not on PATH. Install it from https://itch.io/docs/butler/installing.html and run "butler login" once.
  goto :finish
)

if not exist "%REPO%\Build\GeoTrinity.zip" (
  echo No package at "%REPO%\Build\GeoTrinity.zip". Run Tools\Build_Package.bat first.
  goto :finish
)

if "%~1"=="" (
  %RUN% butler push "%REPO%\Build\GeoTrinity.zip" %TARGET%
) else (
  %RUN% butler push "%REPO%\Build\GeoTrinity.zip" %TARGET% --userversion "%~1"
)
set "RC=%errorlevel%"
if not "%RC%"=="0" goto :finish

butler status %TARGET%

:finish
echo.
if "%RC%"=="0" (
  echo ==== %~n0: SUCCEEDED ====
) else (
  echo ==== %~n0: FAILED ^(exit code %RC%^) ====
)
if not defined CI if not defined GEO_NO_PAUSE pause
exit /b %RC%

:dry_run
echo [dry run] %*
exit /b 0
