# BuildPackage.md — Packaging a distributable build of GeoTrinity

Everything needed to **package** GeoTrinity (cook + stage + pak + archive into a runnable, shippable build). **Read this before any packaging task.**

> This is *only* about packaging the game. For editor/dev builds, MCP, and the dev environment, see [`AI/Commands.md`](AI/Commands.md).

## Big RULE
**NEVER close, kill, or restart the user's Unreal editor — not even to package.** Packaging can run with the editor open.

## Packaging (cook + stage + pak + archive)
**Always archive packaged builds into `C:\GeoTrinity\Build` — never `Packaged` or any other folder.**

**Never hardcode an engine path** — it differs per machine (launcher install on the dev PC, source build on the
CI box). Use the wrapper, which resolves it from the `.uproject`'s `EngineAssociation`:

```bash
Tools\Build_Package.bat
```

Or resolve it yourself (the path has spaces — **keep it quoted**):

```powershell
$ue = & Tools\Resolve-Engine.ps1
& "$ue\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\GeoTrinity\GeoTrinity.uproject" -noP4 -platform=Win64 -clientconfig=Development -cook -build -stage -pak -archive -archivedirectory="C:\GeoTrinity\Build"
```

Client packaging (`Development`, `DebugGame`, `Shipping`) works from a launcher install — it ships the
precompiled `UnrealGame` engine libs those configs link against.

**A dedicated-server package needs a source engine**: launcher installs ship no `UnrealServer` libs and no
launcher option adds them, so `-server` / `-serverconfig` fails there. The dev PC has a launcher install, so
package the server on CI — run the **Build GeoTrinity (Custom)** workflow with `build_server: true`.

- UAT archives to `C:\GeoTrinity\Build\Windows`. `Tools\Build_Package.bat` then renames that folder to `C:\GeoTrinity\Build\GeoTrinity` and zips it to `C:\GeoTrinity\Build\GeoTrinity.zip` alongside it (the zip contains a top-level `GeoTrinity\` folder). The raw RunUAT command does neither — it leaves `Windows\`. After a successful build, rename and reorganise to match the CI convention (see `.github/workflows/build-custom.yml`):
  - Rename `C:\GeoTrinity\Build` → `C:\GeoTrinity\Build\GeoTrinity_{branch}_{config}_{timestamp}` (branch from `git rev-parse --abbrev-ref HEAD`, `/`→`-`; timestamp `yyyyMMdd_HHmmss`; config = `Development` unless specified)
  - Move contents of `Windows\` up into the renamed folder, then remove the empty `Windows\` subfolder.
- Run the listen server: `GeoTrinity.exe` in the renamed folder → MainMenu → Play Local → Host.
- Packaged client logs to `%LOCALAPPDATA%\GeoTrinity\Saved\Logs`.
- This is a long cook (several minutes); run it in the background and report the result.
