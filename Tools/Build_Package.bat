@echo off
set REPO=%~dp0..
"C:\UnrealEngine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%REPO%\GeoTrinity.uproject" -noP4 -platform=Win64 -clientconfig=Development -cook -build -stage -pak -archive -archivedirectory="%REPO%\Build"
