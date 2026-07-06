@echo off
set REPO=%~dp0..
"C:\UnrealEngine\Engine\Build\BatchFiles\Build.bat" GeoTrinityEditor Win64 DebugGame -Project="%REPO%\GeoTrinity.uproject" -WaitMutex
