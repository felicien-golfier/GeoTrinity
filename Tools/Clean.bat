@echo off
REM Deletes build outputs (Binaries, Intermediate, Saved\Cooked, plugin Binaries/Intermediate).
REM Ends on a result line and waits for a key, unless CI or GEO_NO_PAUSE is set.
setlocal
for %%i in ("%~dp0..") do set "REPO=%%~fi"
echo Cleaning %REPO%...
set "RC=0"

REM A folder that survives rd is locked -- usually by an open editor or a running build.
for %%d in ("%REPO%\Binaries" "%REPO%\Intermediate" "%REPO%\Saved\Cooked") do (
    rd /s /q "%%~d" 2>nul
    if exist "%%~d" (
        echo Could not fully remove "%%~d" -- something still holds files in it.
        set "RC=1"
    )
)

for /d /r "%REPO%\Plugins" %%d in (Binaries Intermediate) do (
    if exist "%%d" (
        rd /s /q "%%d" 2>nul
        if exist "%%d" (
            echo Could not fully remove "%%d" -- something still holds files in it.
            set "RC=1"
        )
    )
)

:finish
echo.
if "%RC%"=="0" (
  echo ==== %~n0: SUCCEEDED ====
) else (
  echo ==== %~n0: FAILED ^(exit code %RC%^) ====
)
if not defined CI if not defined GEO_NO_PAUSE pause
exit /b %RC%
