# BuildPackage.md — Packaging a distributable build of GeoTrinity

Everything needed to **package** GeoTrinity (cook + stage + pak + archive into a runnable, shippable build). **Read this before any packaging task.**

> This is *only* about packaging the game. For editor/dev builds, MCP, and the dev environment, see [`AI/Commands.md`](AI/Commands.md).

## Big RULE
**NEVER close, kill, or restart the user's Unreal editor — not even to package.** Packaging can run with the editor open.

## Packaging (cook + stage + pak + archive)
**Always archive packaged builds into `C:\GeoTrinity\Build` — never `Packaged` or any other folder.**

Always use the **source** engine's BatchFiles (`C:\UnrealEngine\Engine\...`), never the launcher path.

```bash
# Listen-server-capable standalone client (boots MainMenu → Play Local → Host = listen server)
"C:\UnrealEngine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\GeoTrinity\GeoTrinity.uproject" -noP4 -platform=Win64 -clientconfig=Development -cook -build -stage -pak -archive -archivedirectory="C:\GeoTrinity\Build"
```

- Output initially lands in `C:\GeoTrinity\Build\Windows`. After a successful build, rename and reorganise to match the CI convention (see `.github/workflows/build-custom.yml`):
  - Rename `C:\GeoTrinity\Build` → `C:\GeoTrinity\Build\GeoTrinity_{branch}_{config}_{timestamp}` (branch from `git rev-parse --abbrev-ref HEAD`, `/`→`-`; timestamp `yyyyMMdd_HHmmss`; config = `Development` unless specified)
  - Move contents of `Windows\` up into the renamed folder, then remove the empty `Windows\` subfolder.
- Run the listen server: `GeoTrinity.exe` in the renamed folder → MainMenu → Play Local → Host.
- Packaged client logs to `%LOCALAPPDATA%\GeoTrinity\Saved\Logs`.
- This is a long cook (several minutes); run it in the background and report the result.
