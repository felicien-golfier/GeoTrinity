@echo off
REM Builds the editor target. The engine is resolved per-machine from the .uproject's
REM EngineAssociation (see Resolve-Engine.ps1) so this works on any PC without editing paths.
REM Full compiler output goes to AI\Output\Build\Editor.log; Read-BuildLog.ps1 prints only errors, warnings and the result.
REM Ends on a result line and waits for a key, unless CI or GEO_NO_PAUSE is set.
setlocal
REM Normalize away the trailing "Tools\.." so it does not leak into logs.
for %%i in ("%~dp0..") do set "REPO=%%~fi"

for /f "usebackq delims=" %%i in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Resolve-Engine.ps1"`) do set UE=%%i
if not defined UE (
  echo Could not resolve the Unreal Engine root. Run Tools\Resolve-Engine.ps1 directly to see why.
  set "RC=1"
  goto :finish
)
echo Engine: %UE%

set "LOG_DIR=%REPO%\AI\Output\Build"
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"
echo Building, full output in %LOG_DIR%\Editor.log
call "%UE%\Engine\Build\BatchFiles\Build.bat" GeoTrinityEditor Win64 DebugGame -Project="%REPO%\GeoTrinity.uproject" -WaitMutex > "%LOG_DIR%\Editor.log" 2>&1
set "RC=%errorlevel%"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Read-BuildLog.ps1" -Log "%LOG_DIR%\Editor.log" -BuildExitCode %RC%

:finish
echo.
if "%RC%"=="0" (
  echo ==== %~n0: SUCCEEDED ====
) else (
  echo ==== %~n0: FAILED ^(exit code %RC%^) ====
)
if not defined CI if not defined GEO_NO_PAUSE pause
exit /b %RC%
