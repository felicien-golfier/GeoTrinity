@echo off
set REPO=%~dp0..
echo Cleaning %REPO%...

rd /s /q "%REPO%\Binaries"     2>nul
rd /s /q "%REPO%\Intermediate" 2>nul
rd /s /q "%REPO%\Saved\Cooked" 2>nul

for /d /r "%REPO%\Plugins" %%d in (Binaries Intermediate) do (
    if exist "%%d" rd /s /q "%%d"
)

echo Clean done.
