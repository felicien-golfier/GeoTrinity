# Automated Testing

Research notes (2026-10-06) on writing and running Unreal automation tests for GeoTrinity, so an agent can write
a test, run it, read the failures and fix them in a loop. **Status:** option A (headless runner) is set up:
`Tools\Run_Tests.bat`. Tests so far: `Source/GeoTrinity/Private/Tests/GeoGemTests.cpp`.

## Test layers

Use the lightest layer that can prove the behaviour.

| Layer | Use for | GeoTrinity candidates |
|---|---|---|
| **Low-level**: `IMPLEMENT_SIMPLE_AUTOMATION_TEST`, or **CQTest** (`TEST_CLASS` / `TEST_METHOD` / `BEFORE_EACH` / `ASSERT_THAT`) | Pure logic with no world, or a minimal world from `FActorTestSpawner` | Gem sockets/stats (`GeoGemTests.cpp`), `ExecCalc_Damage` / `ExecCalc_Heal` maths, `{Token}` ability descriptions, pattern geometry (spiral, cone), leaderboard save round-trips |
| **Automation Spec**: `BEGIN_DEFINE_SPEC`, `Describe` / `It` / `LatentIt` | Readable scenarios that run over several frames | Match teardown one tick after the killing blow, cooldown reset paths |
| **World / functional**: CQTest `FMapTestSpawner`, **`PIENetworkComponent`** (one server + N clients in PIE), `AFunctionalTest` actors placed in test maps | Actors, GAS, replication, Blueprints, StateTree | Predicted-GE tag leaks, deployable ASC ownership (cues/tags on the deployer's client), pooling, boss phases |

- **CQTest** is Epic's modern fixture layer over `FAutomationTestBase`. Add `CQTest` to the test module's
  `PrivateDependencyModuleNames`. Latent steps use `TestCommandBuilder.Do(...).Then(...).Until(...)`.
- **`PIENetworkComponent`** is editor-only. It is the most valuable piece for a multiplayer GAS game: several past
  bugs (predicted cooldown tags, deployable cues) are exactly the client/server divergence it can assert on.
- **Functional tests** are `AFunctionalTest` actors in a small dedicated map (`PrepareTest` / `IsReady` /
  `StartTest` / `FinishTest` / `CleanUp`). They are the only layer a Blueprint-heavy scenario can be authored in.
- **Never `Sleep()`** in a test. Wait on an observable condition with a latent command, `Until`, or `LatentIt`.

## Conventions

- Test path: `GeoTrinity.<Area>.<Case>`, e.g. `GeoTrinity.Gems.SocketLayout`. Filters match by prefix, so
  `GeoTrinity.` runs everything and `GeoTrinity.Gems` runs one area.
- Flags: `EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter`.
- Guard every test file with `WITH_DEV_AUTOMATION_TESTS`.
- Recommended home: a dedicated `GeoTrinityTests` module (Type=Editor, depends on `CQTest`), so test code and
  its dependencies never reach a package. `GeoGemTests.cpp` would move there.
- **A new test class appears only after a full editor restart.** Live Coding does not register it.

## Running

```
Tools\Run_Tests.bat [filter]        # default filter GeoTrinity. = every test; GEO_NO_PAUSE=1 for unattended
```

It runs `UnrealEditor-Win64-DebugGame-Cmd.exe` headless, wipes and refills `AI/Output/Tests` (report, HTML viewer,
`run.log`), then `Tools\Read-TestReport.ps1` prints each failure with its errors and decides the result: SUCCEEDED
only when at least one test ran and none failed or stayed unrun. It loads the **last built DLLs**: Live Coding
patches are not in it, so build before testing a change. It can run beside the open editor. The raw command:

```
UnrealEditor-Win64-DebugGame-Cmd.exe "C:\GeoTrinity\GeoTrinity.uproject"
  -Unattended -NoSplash -NoPause -NullRHI -NoSound -stdout -FullStdOutLogOutput
  -ExecCmds="Automation RunTests GeoTrinity.;Quit"
  -ReportExportPath="C:\GeoTrinity\AI\Output\Tests"
  -abslog="C:\GeoTrinity\AI\Output\Tests\run.log"
```

- Resolve the engine with `Tools\Resolve-Engine.ps1` and never hardcode its path (see `Commands.md`).
- `Quit` must sit inside the same `Automation` command, after `;`: the automation controller queues it and
  quits only once the tests are done. A separate console `Quit` would exit before they run.
- `-ExecCmds` alternatives: `Automation RunTests A+B`, or `Automation RunTests Group:<name>`.
- `-ResumeRunTest` (together with `-ReportExportPath`) restarts after a crash from the first test that has not
  run yet, and counts the one in progress as failed.
- **Drop `-NullRHI`** for anything that renders: screenshot comparison, Slate, viewports, materials, and tests
  flagged `NonNullRHI`. Keep rendering tests in a separate run.
- Functional tests need their map path as the argument right after the `.uproject`.

### Reading the result

**Never trust the exit code.** The editor can exit 0 even when tests failed or never ran. Read either:
- `index.json` in the `-ReportExportPath` folder (per-test state, errors and durations; an HTML viewer sits
  beside it), or
- the stdout summary `Tests Complete. Result={Success|Failed} Total: X Passed: Y Failed: Z`, with per-test
  `Result=Passed|Failed` lines and messages like `Expected <X> but got <Y>`.

Also check that the expected tests **actually ran**: an empty filter match "succeeds" with zero tests.

### Where files land

- Reports and the log go to `AI/Output/Tests` (via `-ReportExportPath` and `-abslog`).
- The engine still writes its own files into `Saved/`. That cannot be avoided, and nothing there may be read or
  relied on.

## Agent loop

1. Write or extend the test next to the change (lightest layer).
2. Build the Editor target (`Tools\Build_Editor.bat`).
3. Run the filtered tests headless and parse `index.json`.
4. On failure: read the messages, fix code or test, go back to 2.
5. Report the passed/failed counts and the exact failures. Never claim green without the parsed report.

## Next steps

1. `GeoTrinityTests` module + `GeoGemTests.cpp` moved into it.
2. CI: a test step after the editor build in the GitHub Actions workflow (see `CI-RUNNER.md`).
3. First tests: ExecCalc damage/heal and gems, then a two-client `PIENetworkComponent` test of the
   cooldown-reset path (`ResetCooldowns()`).

## Sources

- Epic, running automation tests: https://dev.epicgames.com/documentation/en-us/unreal-engine/run-automation-tests-in-unreal-engine
- Epic, CQTest: https://dev.epicgames.com/documentation/en-us/unreal-engine/cqtest-test-framework-for-unreal-engine
- Epic, FActorTestSpawner: https://dev.epicgames.com/documentation/unreal-engine/API/Developer/CQTest/FActorTestSpawner
- Epic, Automation Spec: https://dev.epicgames.com/documentation/unreal-engine/automation-spec-in-unreal-engine
- Epic, FunctionalTesting API: https://dev.epicgames.com/documentation/unreal-engine/API/Developer/FunctionalTesting
- CQTest vs Spec vs functional tests, CI gotchas: https://dev.to/gamedevtoollab/testing-in-unreal-engine-5-cqtest-automation-spec-functional-tests-and-ci-3776
- Authoring macros reference: https://tessl.io/registry/testland/unreal-automation-system/files/references/authoring-macros-and-apis.md
- AI-agent runner skill (stdout parsing): https://skillsmp.com/creators/guangminju/unrealskills/ue-test
