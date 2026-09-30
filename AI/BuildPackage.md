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
- **Every package gets `Engine\Build\InstalledProjectBuild.txt`** (contents `GeoTrinity/GeoTrinity.uproject`),
  written after archiving by `Build_Package.bat` and CI. Without it, a non-Shipping package writes its `Saved\`
  (GameUserSettings.ini with the volumes, Enhanced Input key bindings, save slots) inside the build folder, so
  each new build loses them. With it, the game counts as installed (`FApp::IsInstalled`), as Shipping already
  does, and saves to `%LOCALAPPDATA%\GeoTrinity\Saved`. The raw RunUAT command does not write it, so add it by
  hand. Launch with `-NotInstalled` to opt out.
- Run the listen server: `GeoTrinity.exe` in that folder → MainMenu → Play Local → Host.
- Packaged client logs to `%LOCALAPPDATA%\GeoTrinity\Saved\Logs`.
- The cook takes several minutes — run it in the background and report the result.

## Publishing to itch.io

The game lives at https://exyoe.itch.io/geotrinity; the Windows build is the butler channel
`exyoe/geotrinity:windows`.

| Script | Does |
|---|---|
| `Tools\BuildAndUpload_Itch.bat [version]` | `Build_Package.bat`, then `Push_Itch.bat` if the package succeeded |
| `Tools\Push_Itch.bat [version]` | Pushes the existing `Build\GeoTrinity.zip`, no build |

- butler unpacks the zip and diffs its contents against the last push, uploading only what changed.
- Build the zip with `tar -a -c -f` (as `Build_Package.bat` does), never Windows PowerShell's `Compress-Archive`
  or `ZipFile`: both write `\` entry names, which butler reads as a file clashing with its folder and rejects
  with "Two entries have the same name".
- butler is installed in `C:\Program Files\Butler` (on the user PATH) and logged in once with `butler login`;
  on CI, set a `BUTLER_API_KEY` secret instead.
- Publishing is outward-facing: only push when the user asks for it.
