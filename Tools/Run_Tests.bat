@echo off
REM Runs the automation tests headless in a separate DebugGame editor process (NullRHI, no window) and reads the
REM exported report. Safe beside an open editor: it loads the last built DLLs, never live-coded patches.
REM Usage: Run_Tests.bat [filter]   e.g. Run_Tests.bat GeoTrinity.Gems   (default: GeoTrinity. = every test)
REM Report, HTML viewer and log go to AI\Output\Tests. See AI\Testing.md.
REM Ends on a result line and waits for a key, unless CI or GEO_NO_PAUSE is set.
setlocal
REM Normalize away the trailing "Tools\.." so it does not leak into logs.
for %%i in ("%~dp0..") do set "REPO=%%~fi"
set "FILTER=%~1"
if not defined FILTER set "FILTER=GeoTrinity."
set "OUT=%REPO%\AI\Output\Tests"

for /f "usebackq delims=" %%i in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Resolve-Engine.ps1"`) do set UE=%%i
if not defined UE (
  echo Could not resolve the Unreal Engine root. Run Tools\Resolve-Engine.ps1 directly to see why.
  set "RC=1"
  goto :finish
)
echo Engine: %UE%
echo Filter: %FILTER%

if exist "%OUT%" rmdir /s /q "%OUT%"
mkdir "%OUT%"

"%UE%\Engine\Binaries\Win64\UnrealEditor-Win64-DebugGame-Cmd.exe" "%REPO%\GeoTrinity.uproject" -Unattended -NoSplash -NoPause -NullRHI -NoSound -stdout -FullStdOutLogOutput -ExecCmds="Automation RunTests %FILTER%;Quit" -ReportExportPath="%OUT%" -abslog="%OUT%\run.log"
echo Editor exit code: %errorlevel% (not trusted, the report decides)

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Read-TestReport.ps1" -ReportDir "%OUT%"
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
