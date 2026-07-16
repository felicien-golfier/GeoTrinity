# GeoTrinity CI — GitHub Actions & Self-Hosted Runner

## Overview

Two separate systems talk to each other to run builds automatically:

- **GitHub's side** — hosted in the cloud, owns the workflow queue
- **This machine** — runs the actual build work via a self-hosted runner

```
You push to build-listen branch
        ↓
GitHub reads .github/workflows/build-listen.yml
        ↓
GitHub creates a job and puts it in a queue
        ↓
Runner (Windows service on this PC) claims the job
        ↓
Runner executes the steps (git pull → Build.bat)
        ↓
Results reported back → visible in GitHub → Actions tab
```

---

## Files and What They Do

### In the Repo

```
.github/
└── workflows/
    ├── build-auto.yml     triggers on push to `build` → calls build-custom.yml with defaults
    └── build-custom.yml   the real job: checkout → resolve engine → package client (+ server)
```

`build-auto.yml` is a thin wrapper; every real step lives in `build-custom.yml`, which is also runnable
by hand from **Actions → Build GeoTrinity (Custom) → Run workflow** (pick branch, config, whether to
also build the dedicated server, and whether to clean first).

The `.yml` files are the entire job definition. They tell GitHub:
- **When** to run (`on: push: branches`)
- **Where** to run (`runs-on: self-hosted` = this machine)
- **What** to do (each `steps:` entry)

### On This Machine

```
H:\actions-runner\
├── .runner          — binds this runner to felicien-golfier/GeoTrinity
├── .credentials     — OAuth token to authenticate with GitHub
├── .service         — tells the runner it is managed as a Windows service
├── bin\
│   ├── RunnerService.exe    — the Windows service process (auto-starts on boot)
│   └── Runner.Listener.exe  — the runner logic, spawned by the service
└── _work\           — job workspace; actions/checkout puts the repo here ($env:GITHUB_WORKSPACE)
```

`.runner` is the key file that links everything:

```json
{
  "agentName": "BuildGeoTrinity",
  "gitHubUrl": "https://github.com/felicien-golfier/GeoTrinity",
  "poolName": "Default"
}
```

### Source Code

The workflow uses `actions/checkout`, so GitHub decides where the repo lives and tells the job via
`$env:GITHUB_WORKSPACE` (in practice `H:\actions-runner\_work\GeoTrinity\GeoTrinity`).

**No path to the repo or to the engine is written down in this repo, deliberately** — the same workflow
has to run on any machine. The engine comes from `Tools\Resolve-Engine.ps1`, which resolves the
`.uproject`'s `EngineAssociation` through the registry; see `AI/Commands.md`.

Checkout passes `clean: false` so `Binaries\`, `Intermediate\` and `DerivedDataCache\` (all gitignored)
survive between runs and builds stay incremental. The default `clean: true` runs `git clean -ffdx`,
which would wipe them and turn every run into a 1-3 hour full rebuild.

---

## How the Runner Stays Connected

```
RunnerService.exe      (Windows service — auto-starts on boot)
        ↓ spawns
Runner.Listener.exe    (maintains long-poll HTTPS to broker.actions.githubusercontent.com)
        ↓ when a job arrives
Runner.Worker.exe      (spawned per job to execute the steps)
```

The service is named `"actions.runner.felicien-golfier-GeoTrinity.BuildGeoTrinity"` and is set to **Automatic** start.

---

## What Links the Workflow to This Runner

Three things must align:

| In the workflow `.yml`  | Meaning |
|-------------------------|---------|
| `runs-on: self-hosted`  | Use a self-hosted runner, not GitHub's cloud machines |
| Workflow lives in this repo | GitHub only dispatches to runners registered to the same repo |
| `.runner` points to this repo | Runner only listens for jobs from `felicien-golfier/GeoTrinity` |

No other configuration is needed — GitHub matches by repo automatically.

---

## Workflow Breakdown — `build-custom.yml`

The two steps worth understanding; the rest (notifications, optional clean, archive tidy-up) are
straightforward. Read the file itself for the full picture.

```yaml
      - name: Checkout
        uses: actions/checkout@v4
        with:
          ref: ${{ inputs.branch }}
          clean: false          # keep Binaries/Intermediate so builds stay incremental

      - name: Resolve engine
        # Derives the engine from the .uproject's EngineAssociation rather than hardcoding a path,
        # so this workflow is not tied to one machine's layout. Sets UE for the later steps.
        # Override with the repo variable UE_ROOT if detection ever fails.
        run: |
          $ue = & "$env:GITHUB_WORKSPACE\Tools\Resolve-Engine.ps1" -Verbose
          "UE=$ue" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
```

**Gotcha — the runner is SYSTEM, and source builds register in HKCU.** Unreal registers a source build under
`HKCU\SOFTWARE\Epic Games\Unreal Engine\Builds`, i.e. in the hive of whoever was logged in when it was
registered. The service runs as `LocalSystem`, whose `HKCU` is `HKEY_USERS\S-1-5-18` — a *different* hive. So
auto-detection can come up empty here even though the engine is plainly registered for the desktop user, and
it would then fall back to a launcher install (which cannot build the dedicated server) or fail outright.

Run this once on the runner box, from an elevated prompt, in the desktop session where the source engine is
registered. It auto-detects that engine, copies the answer into a machine-wide `UE` variable that SYSTEM
*can* read (the resolver honours it as an override), and restarts the runner service so it picks it up:

```powershell
.\Tools\Setup-Runner.ps1                              # or -EngineRoot "H:\Epic\UE_5.7" to pin one
.\Tools\Setup-Runner.ps1 -WhatIf                      # preview, changes nothing
```

It warns if the engine it finds is a launcher install, since `build_server: true` would then fail. No path
goes into the repo — it stays on the machine it describes.

Alternative, if you would rather configure it from GitHub: set the repo variable `UE_ROOT`
(Settings → Secrets and variables → Actions); the workflow passes it to the resolver as `-Override`.

---

## Build Targets

| Target file | `Type` | Used for |
|---|---|---|
| `Source/GeoTrinity.Target.cs` | `TargetType.Game` | Client + listen server host |
| `Source/GeoTrinityServer.Target.cs` | `TargetType.Server` | Dedicated server only |
| `Source/GeoTrinityEditor.Target.cs` | `TargetType.Editor` | Editor only |

`build-custom.yml` always packages `GeoTrinity` (Game), and adds `GeoTrinityServer` (Dedicated) when the
`build_server` input is true.

**Only this runner can build the dedicated server.** `GeoTrinityServer` links against engine libs that
launcher installs do not ship (they carry `UnrealEditor` and `UnrealGame` only — no `UnrealServer`), so it
needs the source engine that lives on this machine. The main dev PC has a launcher install and cannot do
it; `Tools\Build_Server.bat` there detects that and says so.

---

## Triggering a Build

### Option A — Push to the trigger branch

```bash
git checkout build
git merge master              # bring in latest changes
git push origin build         # fires build-auto.yml -> build-custom.yml
git checkout master           # return to normal work
```

### Option B — GitHub UI

**GitHub → Actions → Build GeoTrinity (Custom) → Run workflow**, then pick branch, config, and whether to
build the dedicated server.

---

## Known Quirks

| Issue | Cause | Fix applied |
|---|---|---|
| Source engine invisible to CI | Unreal registers source builds under `HKCU`; the runner is a service running as SYSTEM, whose `HKCU` is `HKEY_USERS\S-1-5-18` — a different hive from the desktop user's | `Tools\Setup-Runner.ps1` (elevated), or the repo variable `UE_ROOT` |
| `git pull` exits 128 instantly | Git 2.36+ rejects repos owned by a different user when run as SYSTEM | `safe.directory = *` written to `C:\Windows\System32\config\systemprofile\.gitconfig` (SYSTEM's global gitconfig). *Historical — the workflow now uses `actions/checkout`, which handles this itself. The setting is harmless and still in place.* |
| `git pull` auth failure | SYSTEM has no stored HTTPS credentials for GitHub | `GITHUB_TOKEN` passed as HTTP header. *Historical — `actions/checkout` now handles auth.* |
| Windows service wouldn't start | `ImagePath` in registry had 3 corrupt trailing characters | Fixed registry key directly |
| `.service` file missing | File was absent so runner reported `IsServiceConfigured: False` | Created `H:\actions-runner\.service` |
