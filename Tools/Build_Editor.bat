@echo off
REM Builds the editor target. The engine is resolved per-machine from the .uproject's
REM EngineAssociation (see Resolve-Engine.ps1) so this works on any PC without editing paths.
setlocal
REM Normalize away the trailing "Tools\.." so it does not leak into logs.
for %%i in ("%~dp0..") do set "REPO=%%~fi"

for /f "usebackq delims=" %%i in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Resolve-Engine.ps1"`) do set UE=%%i
if not defined UE (
  echo Could not resolve the Unreal Engine root. Run Tools\Resolve-Engine.ps1 directly to see why.
  exit /b 1
)
echo Engine: %UE%

"%UE%\Engine\Build\BatchFiles\Build.bat" GeoTrinityEditor Win64 DebugGame -Project="%REPO%\GeoTrinity.uproject" -WaitMutex
