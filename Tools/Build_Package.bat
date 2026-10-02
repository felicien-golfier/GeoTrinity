@echo off
REM Packages a distributable client (cook + stage + pak + archive into Build\).
REM Usage: Tools\Build_Package.bat [Win64 (default) or Linux]
REM Linux is cross-compiled and needs the toolchain from AI\BuildPackage.md (LINUX_MULTIARCH_ROOT).
REM The engine is resolved per-machine from the .uproject's EngineAssociation (see Resolve-Engine.ps1).
REM Ends on a result line and waits for a key, unless CI or GEO_NO_PAUSE is set.
REM GEO_DRY_RUN=1 prints every step that builds or changes files instead of running it.
setlocal
set "RUN="
if defined GEO_DRY_RUN set "RUN=call :dry_run"
REM Normalize away the trailing "Tools\.." so it does not leak into the archive path and logs.
REM (Do not write a percent-tilde operator in a REM line -- cmd still parses it and errors out.)
for %%i in ("%~dp0..") do set "REPO=%%~fi"

REM STAGED is the folder UAT archives into; PRODUCT is what it gets renamed and zipped to.
set "PLATFORM=%~1"
if not defined PLATFORM set "PLATFORM=Win64"
if /i "%PLATFORM%"=="Win64" (
  set "STAGED=Windows"
  set "PRODUCT=GeoTrinity"
) else if /i "%PLATFORM%"=="Linux" (
  set "STAGED=Linux"
  set "PRODUCT=GeoTrinity_Linux"
) else (
  echo Unknown platform "%PLATFORM%". Use Win64 or Linux.
  set "RC=1"
  goto :finish
)
if /i "%PLATFORM%"=="Linux" if not defined LINUX_MULTIARCH_ROOT (
  echo LINUX_MULTIARCH_ROOT is not set. Install the Linux cross-compile toolchain listed in AI\BuildPackage.md, then open a new terminal.
  set "RC=1"
  goto :finish
)

for /f "usebackq delims=" %%i in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Resolve-Engine.ps1"`) do set UE=%%i
if not defined UE (
  echo Could not resolve the Unreal Engine root. Run Tools\Resolve-Engine.ps1 directly to see why.
  set "RC=1"
  goto :finish
)
echo Engine: %UE%

%RUN% call "%UE%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%REPO%\GeoTrinity.uproject" -noP4 -platform=%PLATFORM% -clientconfig=Development -cook -build -stage -pak -archive -archivedirectory="%REPO%\Build"
set "RC=%errorlevel%"
if not "%RC%"=="0" goto :finish
set "RC=1"

if not defined GEO_DRY_RUN if not exist "%REPO%\Build\%STAGED%" (
  echo Expected staged build at "%REPO%\Build\%STAGED%" but it does not exist.
  goto :finish
)

REM UAT always archives to a platform-named folder; rename it to the product name.
if exist "%REPO%\Build\%PRODUCT%" %RUN% rmdir /s /q "%REPO%\Build\%PRODUCT%"
%RUN% move "%REPO%\Build\%STAGED%" "%REPO%\Build\%PRODUCT%"
if errorlevel 1 (
  echo Failed to rename "%REPO%\Build\%STAGED%" to "%REPO%\Build\%PRODUCT%".
  goto :finish
)

REM This marker makes the game count as installed, so it saves settings, key bindings and saves to
REM LOCALAPPDATA\GeoTrinity\Saved (Linux: ~/.config/Epic/GeoTrinity/Saved) instead of inside the build, which each new build replaces.
%RUN% mkdir "%REPO%\Build\%PRODUCT%\Engine\Build" 2>nul
if defined GEO_DRY_RUN (
  call :dry_run write "%REPO%\Build\%PRODUCT%\Engine\Build\InstalledProjectBuild.txt"
) else (
  echo GeoTrinity/GeoTrinity.uproject> "%REPO%\Build\%PRODUCT%\Engine\Build\InstalledProjectBuild.txt"
)
if errorlevel 1 (
  echo Failed to write "%REPO%\Build\%PRODUCT%\Engine\Build\InstalledProjectBuild.txt".
  goto :finish
)
echo Build: %REPO%\Build\%PRODUCT%

echo Zipping "%REPO%\Build\%PRODUCT%" to "%REPO%\Build\%PRODUCT%.zip"
REM tar, not Compress-Archive: Windows PowerShell writes backslash entry names, which butler rejects.
%RUN% tar -a -c -f "%REPO%\Build\%PRODUCT%.zip" -C "%REPO%\Build" %PRODUCT%
if errorlevel 1 (
  echo Failed to create "%REPO%\Build\%PRODUCT%.zip".
  goto :finish
)
echo Zip: %REPO%\Build\%PRODUCT%.zip
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
