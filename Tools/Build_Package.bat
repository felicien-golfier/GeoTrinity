@echo off
REM Packages a distributable client (cook + stage + pak + archive into Build\).
REM The engine is resolved per-machine from the .uproject's EngineAssociation (see Resolve-Engine.ps1).
setlocal
REM Normalize away the trailing "Tools\.." so it does not leak into the archive path and logs.
REM (Do not write a percent-tilde operator in a REM line -- cmd still parses it and errors out.)
for %%i in ("%~dp0..") do set "REPO=%%~fi"

for /f "usebackq delims=" %%i in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Resolve-Engine.ps1"`) do set UE=%%i
if not defined UE (
  echo Could not resolve the Unreal Engine root. Run Tools\Resolve-Engine.ps1 directly to see why.
  exit /b 1
)
echo Engine: %UE%

"%UE%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%REPO%\GeoTrinity.uproject" -noP4 -platform=Win64 -clientconfig=Development -cook -build -stage -pak -archive -archivedirectory="%REPO%\Build"
