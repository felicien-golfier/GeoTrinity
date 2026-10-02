# Packaging a Distributable Build

Cook + stage + pak + archive into a runnable, shippable build. Read this before any packaging task; for
editor/dev builds and how the engine path is resolved see [`Commands.md`](Commands.md).

**NEVER close, kill or restart the user's Unreal editor — not even to package.** Packaging runs with it open.

**Always archive into `C:\GeoTrinity\Build`** — never `Packaged` or any other folder.

```bash
Tools\Build_Package.bat          # Windows (Win64) — the default
Tools\Build_Package.bat Linux    # Linux client, cross-compiled from Windows — see Linux below
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

## Linux

The Linux client is **cross-compiled and cooked on Windows**. Nothing is built on a Linux machine; a Linux PC
only runs the package.

### One-time setup (Windows build machine)
1. **Linux target files for the engine.** A launcher install ships none by default: no
   `Engine\Intermediate\Build\Linux`, no Linux Steamworks libs. In the Epic Games Launcher, open
   Unreal Engine → Library → 5.7 → ▾ → Options → Target Platforms, tick **Linux** and apply.
2. **Cross-compile toolchain.** Install **`v26_clang-20.1.8-rockylinux8`** (clang 20.1.8) from
   `https://cdn.unrealengine.com/CrossToolchain_Linux/v26_clang-20.1.8-rockylinux8.exe`. The installer sets
   `LINUX_MULTIARCH_ROOT`. Check it in a **new** terminal with `echo %LINUX_MULTIARCH_ROOT%`. Restart the editor
   and Rider so they pick up the variable.
3. In the editor, Platforms → Linux must show no "SDK not installed" warning.

The toolchain version is pinned per engine version. After an engine upgrade, read
`Engine\Config\Linux\Linux_SDK.json` (`MainVersion`) and install that toolchain. The documentation page often lags
behind the engine release.

### Packaging
`Tools\Build_Package.bat Linux` archives to `Build\GeoTrinity_Linux` and zips it to `Build\GeoTrinity_Linux.zip`.
It refuses to start when `LINUX_MULTIARCH_ROOT` is unset. The Windows package (`Build\GeoTrinity`,
`GeoTrinity.zip`) is left untouched, so itch.io pushes are unaffected. By hand, use the same RunUAT command with
`-platform=Linux`. UAT then archives to `Build\Linux`.

### Running it (Linux test machine)
- **OS:** glibc 2.28+ (the toolchain targets Rocky Linux 8). Recommended: Ubuntu 22.04 or 24.04 LTS.
  Check with `ldd --version`.
- **GPU:** the game renders with Vulkan only. Use NVIDIA driver 570+ (`sudo ubuntu-drivers install`) or Mesa RADV
  24.2.8+ on AMD. `vulkaninfo --summary` (package `vulkan-tools`) must list the GPU.
- Unzip, then run `chmod +x GeoTrinity.sh GeoTrinity/Binaries/Linux/GeoTrinity`. A Windows-made zip carries no
  exec bits. Launch with `./GeoTrinity.sh`.
- Logs and saves: `~/.config/Epic/GeoTrinity/Saved` (the installed-build marker applies as on Windows).
- **Steam:** the Steam client must be running and logged in, as on Windows. Otherwise the net driver falls back
  to plain IP.

### Linux-specific project settings
- **`Config/Linux/LinuxInput.ini` is required.** The engine ships no Linux input ini. Without it, the engine
  ensures on a missing `MaxPlatformUserCount` and falls back to `PrimaryUserSharesKeyboardAndFirstGamepad`, which
  breaks single-gamepad couch coop (see `Source/GeoTrinity/Public/GameClasses/CLAUDE.md`). Keep it in sync with `Config/Windows/WindowsInput.ini`.
- **Vulkan SM5 and SM6 are both targeted** (`[/Script/LinuxTargetPlatform.LinuxTargetSettings]`), as Windows
  keeps DX11 SM5 next to DX12 SM6. An SM6-only package cannot start on an older GPU or driver.
  The engine picks the highest targeted level the GPU supports (`LinuxDynamicRHI.cpp`). Force one with `-sm5` or
  `-sm6` on the command line.
- **Keyboard-layout key seeding is Windows-only.** `AGeoPlayerController::SeedKeyBindingsForKeyboardLayout`
  uses Win32 layout APIs. On Linux, an AZERTY player gets the QWERTY default bindings and must rebind by hand.

### Not covered yet
- **Linux dedicated server** — needs a source engine, like the Win64 server. Do it on CI.
- **itch.io Linux channel** — `Push_Itch.bat` only pushes `exyoe/geotrinity:windows`.
- **Mac** — cannot be built from Windows at all. It needs a Mac with the Xcode version that 5.7 requires, plus an
  Apple Developer account to sign and notarize the build.

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
