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
    ├── build.yml           triggers on push to `build`        → builds Client + Dedicated Server
    └── build-listen.yml    triggers on push to `build-listen` → builds Game target only (listen server)
```

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
└── _work\           — default job workspace (not used, we build from H:\Work\Git\GeoTrinity)
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

```
H:\Work\Git\GeoTrinity\   — the runner builds directly from this local clone
```

The workflow does a `git pull` here instead of a full checkout, so builds are incremental and fast.

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

## Workflow Breakdown — `build-listen.yml`

```yaml
on:
  push:
    branches: [build-listen]   # fires on every push to this branch
  workflow_dispatch:            # or manually from GitHub UI → Actions

jobs:
  build:
    runs-on: self-hosted
    timeout-minutes: 240        # UE full rebuilds can take 1-3 hours

    steps:
      - name: Pull latest
        # Pulls the latest commits into the local source folder.
        # GITHUB_TOKEN is injected automatically by GitHub and used for HTTPS auth.
        # safe.directory is handled via C:\Windows\System32\config\systemprofile\.gitconfig
        # (SYSTEM's global gitconfig) so it doesn't need to be passed here.
        run: |
          "C:\Program Files\Git\cmd\git.exe" ^
            -C "H:\Work\Git\GeoTrinity" ^
            -c "http.https://github.com/.extraheader=Authorization: Bearer %GITHUB_TOKEN%" pull
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}

      - name: Build Game (GeoTrinity Win64 Development)
        # Builds the Game target = listen server capable binary.
        # GeoTrinity    → Type = TargetType.Game  (listen server)
        # GeoTrinityServer → Type = TargetType.Server (dedicated only, not built here)
        run: |
          "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" ^
            GeoTrinity Win64 Development ^
            "H:\Work\Git\GeoTrinity\GeoTrinity.uproject" ^
            -WaitMutex -NoHotReloadFromIDE
        env:
          UE_IgnoreOutdatedImportedLibraries: 1
```

---

## Build Targets

| Target file | `Type` | Used for |
|---|---|---|
| `Source/GeoTrinity.Target.cs` | `TargetType.Game` | Client + listen server host |
| `Source/GeoTrinityServer.Target.cs` | `TargetType.Server` | Dedicated server only |
| `Source/GeoTrinityEditor.Target.cs` | `TargetType.Editor` | Editor only |

`build-listen.yml` builds `GeoTrinity` (Game).  
`build.yml` builds both `GeoTrinity` (Game) and `GeoTrinityServer` (Dedicated).

---

## Triggering a Build

### Option A — Push to the trigger branch

```bash
# From H:\Work\Git\GeoTrinity
git checkout build-listen
git merge master              # bring in latest changes
git push origin build-listen
git checkout master           # return to normal work
```

### Option B — GitHub UI

Go to **GitHub → Actions → Build GeoTrinity (Listen Server) → Run workflow**.

---

## Known Quirks

| Issue | Cause | Fix applied |
|---|---|---|
| `git pull` exits 128 instantly | Git 2.36+ rejects repos owned by a different user when run as SYSTEM | `safe.directory = *` written to `C:\Windows\System32\config\systemprofile\.gitconfig` (SYSTEM's global gitconfig) |
| `git pull` auth failure | SYSTEM has no stored HTTPS credentials for GitHub | `GITHUB_TOKEN` passed as HTTP header |
| Windows service wouldn't start | `ImagePath` in registry had 3 corrupt trailing characters | Fixed registry key directly |
| `.service` file missing | File was absent so runner reported `IsServiceConfigured: False` | Created `H:\actions-runner\.service` |
