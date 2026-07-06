@echo off
set REPO=%~dp0..
"C:\UnrealEngine\Engine\Build\BatchFiles\Build.bat" GeoTrinity Win64 Development -Project="%REPO%\GeoTrinity.uproject" -WaitMutex
