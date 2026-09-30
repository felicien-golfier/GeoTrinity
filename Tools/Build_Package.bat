@echo off
REM Packages a distributable client (cook + stage + pak + archive into Build\).
REM The engine is resolved per-machine from the .uproject's EngineAssociation (see Resolve-Engine.ps1).
REM Ends on a result line and waits for a key, unless CI or GEO_NO_PAUSE is set.
REM GEO_DRY_RUN=1 prints every step that builds or changes files instead of running it.
setlocal
set "RUN="
if defined GEO_DRY_RUN set "RUN=call :dry_run"
REM Normalize away the trailing "Tools\.." so it does not leak into the archive path and logs.
REM (Do not write a percent-tilde operator in a REM line -- cmd still parses it and errors out.)
for %%i in ("%~dp0..") do set "REPO=%%~fi"

for /f "usebackq delims=" %%i in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Resolve-Engine.ps1"`) do set UE=%%i
if not defined UE (
  echo Could not resolve the Unreal Engine root. Run Tools\Resolve-Engine.ps1 directly to see why.
  set "RC=1"
  goto :finish
)
echo Engine: %UE%

%RUN% call "%UE%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%REPO%\GeoTrinity.uproject" -noP4 -platform=Win64 -clientconfig=Development -cook -build -stage -pak -archive -archivedirectory="%REPO%\Build"
set "RC=%errorlevel%"
if not "%RC%"=="0" goto :finish
set "RC=1"

if not defined GEO_DRY_RUN if not exist "%REPO%\Build\Windows" (
  echo Expected staged build at "%REPO%\Build\Windows" but it does not exist.
  goto :finish
)

REM UAT always archives to a platform-named folder; rename it to the product name.
if exist "%REPO%\Build\GeoTrinity" %RUN% rmdir /s /q "%REPO%\Build\GeoTrinity"
%RUN% move "%REPO%\Build\Windows" "%REPO%\Build\GeoTrinity"
if errorlevel 1 (
  echo Failed to rename "%REPO%\Build\Windows" to "%REPO%\Build\GeoTrinity".
  goto :finish
)

REM This marker makes the game count as installed, so it saves settings, key bindings and saves to
REM LOCALAPPDATA\GeoTrinity\Saved instead of inside the build, which each new build replaces.
%RUN% mkdir "%REPO%\Build\GeoTrinity\Engine\Build" 2>nul
if defined GEO_DRY_RUN (
  call :dry_run write "%REPO%\Build\GeoTrinity\Engine\Build\InstalledProjectBuild.txt"
) else (
  echo GeoTrinity/GeoTrinity.uproject> "%REPO%\Build\GeoTrinity\Engine\Build\InstalledProjectBuild.txt"
)
if errorlevel 1 (
  echo Failed to write "%REPO%\Build\GeoTrinity\Engine\Build\InstalledProjectBuild.txt".
  goto :finish
)
echo Build: %REPO%\Build\GeoTrinity

echo Zipping "%REPO%\Build\GeoTrinity" to "%REPO%\Build\GeoTrinity.zip"
REM tar, not Compress-Archive: Windows PowerShell writes backslash entry names, which butler rejects.
%RUN% tar -a -c -f "%REPO%\Build\GeoTrinity.zip" -C "%REPO%\Build" GeoTrinity
if errorlevel 1 (
  echo Failed to create "%REPO%\Build\GeoTrinity.zip".
  goto :finish
)
echo Zip: %REPO%\Build\GeoTrinity.zip
set "RC=0"

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
