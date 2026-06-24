# BuildPackage.md — Packaging a distributable build of GeoTrinity

Everything needed to **package** GeoTrinity (cook + stage + pak + archive into a runnable, shippable build). **Read this before any packaging task.**

> This is *only* about packaging the game. For editor/dev builds, MCP, and the dev environment, see [`AI/Commands.md`](AI/Commands.md).

## Big RULE
**NEVER close, kill, or restart the user's Unreal editor — not even to package.** Packaging locks the DLL link and requires the editor closed. If an editor is running, **ask the user to close it and wait**; never do it yourself.

Check for a running editor first:
```powershell
Get-Process UnrealEditor* -ErrorAction SilentlyContinue | Select-Object Id, ProcessName
```

## Packaging (cook + stage + pak + archive)
**Always archive packaged builds into `C:\GeoTrinity\Build` — never `Packaged` or any other folder.**

Always use the **source** engine's BatchFiles (`C:\UnrealEngine\Engine\...`), never the launcher path.

```bash
# Listen-server-capable standalone client (boots MainMenu → Play Local → Host = listen server)
"C:\UnrealEngine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\GeoTrinity\GeoTrinity.uproject" -noP4 -platform=Win64 -clientconfig=Development -cook -build -stage -pak -archive -archivedirectory="C:\GeoTrinity\Build"
```

- Output lands in `C:\GeoTrinity\Build\Windows` (includes `GeoTrinity.exe`).
- Run the listen server: `C:\GeoTrinity\Build\Windows\GeoTrinity.exe` → MainMenu → Play Local → Host.
- Packaged client logs to `%LOCALAPPDATA%\GeoTrinity\Saved\Logs`.
- This is a long cook (several minutes); run it in the background and report the result.
