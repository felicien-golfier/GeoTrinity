# Packaging a Distributable Build

Cook + stage + pak + archive into a runnable, shippable build. Read this before any packaging task; for
editor/dev builds and how the engine path is resolved see [`Commands.md`](Commands.md).

**NEVER close, kill or restart the user's Unreal editor — not even to package.** Packaging runs with it open.

**Always archive into `C:\GeoTrinity\Build`** — never `Packaged` or any other folder.

```bash
Tools\Build_Package.bat
```

Or by hand, resolving the engine as `Commands.md` describes (keep the quoted path):

```powershell
$ue = & Tools\Resolve-Engine.ps1
& "$ue\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\GeoTrinity\GeoTrinity.uproject" -noP4 -platform=Win64 -clientconfig=Development -cook -build -stage -pak -archive -archivedirectory="C:\GeoTrinity\Build"
```

Client packaging (`Development`, `DebugGame`, `Shipping`) works from a launcher install. A **dedicated-server
package needs a source engine**, so package it on CI — run **Build GeoTrinity (Custom)** with
`build_server: true`.

- UAT archives to `C:\GeoTrinity\Build\Windows`. `Tools\Build_Package.bat` renames that to
  `C:\GeoTrinity\Build\GeoTrinity` and zips it alongside as `GeoTrinity.zip` (the zip holds a top-level
  `GeoTrinity\` folder). The raw RunUAT command does neither and leaves `Windows\`, so after it, reorganise to
  the CI convention (see `.github/workflows/build-custom.yml`): rename the build folder to
  `GeoTrinity_{branch}_{config}_{timestamp}` (branch from `git rev-parse --abbrev-ref HEAD` with `/`→`-`,
  timestamp `yyyyMMdd_HHmmss`, config `Development` unless specified), move `Windows\`'s contents up into it
  and remove the empty folder.
- Run the listen server: `GeoTrinity.exe` in that folder → MainMenu → Play Local → Host.
- Packaged client logs to `%LOCALAPPDATA%\GeoTrinity\Saved\Logs`.
- The cook takes several minutes — run it in the background and report the result.
