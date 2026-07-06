@echo off
set REPO=%~dp0..
"C:\UnrealEngine\Engine\Build\BatchFiles\Build.bat" GeoTrinityServer Win64 Development -Project="%REPO%\GeoTrinity.uproject" -WaitMutex
