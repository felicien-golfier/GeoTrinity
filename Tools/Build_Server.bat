@echo off
REM Builds the dedicated server target.
REM
REM This only works on a machine with a SOURCE engine: launcher (installed) builds ship precompiled
REM libs for UnrealEditor and UnrealGame only -- there is no UnrealServer to link against, and no
REM launcher option adds one. Resolve-Engine.ps1 -RequireSourceBuild decides that per-machine, so this
REM script builds on the source-engine PC and explains itself on a launcher-only PC.
REM Ends on a result line and waits for a key, unless CI or GEO_NO_PAUSE is set.
setlocal
REM Normalize away the trailing "Tools\.." so it does not leak into logs.
for %%i in ("%~dp0..") do set "REPO=%%~fi"

REM stdout carries the engine root; the resolver's explanation (if any) goes to stderr and prints itself.
for /f "usebackq delims=" %%i in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Resolve-Engine.ps1" -RequireSourceBuild`) do set UE=%%i
if not defined UE (
  echo.
  REM Parens need ^ escaping inside this if-block, but NOT inside double quotes -- cmd does not treat
  REM a quoted ) as a block terminator, so an escaped one there prints the caret literally.
  echo Cannot build the dedicated server on this machine ^(reason above^).
  echo Build it on CI instead: run the "Build GeoTrinity (Custom)" workflow with build_server = true.
  set "RC=1"
  goto :finish
)
echo Engine: %UE%

call "%UE%\Engine\Build\BatchFiles\Build.bat" GeoTrinityServer Win64 Development -Project="%REPO%\GeoTrinity.uproject" -WaitMutex
set "RC=%errorlevel%"

:finish
echo.
if "%RC%"=="0" (
  echo ==== %~n0: SUCCEEDED ====
) else (
  echo ==== %~n0: FAILED ^(exit code %RC%^) ====
)
if not defined CI if not defined GEO_NO_PAUSE pause
exit /b %RC%
