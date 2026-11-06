---
name: test
description: Build GeoTrinity and run its automated tests in one call (Tools/Run_Gate.ps1); green → one line, no agent; red → hands the result to the test-fixer agent. Arguments = tokens - none or `quick` (quick suite), `bots` (every tutorial bot session), `GeoTrinity.<filter>` (only those tests), `nobuild`, `fix` (start test-fixer even if green). Combine them, e.g. `quick GeoTrinity.Tutorial.Path`.
---

# Build + test gate

1. **Run** from the repo root, PowerShell: `& Tools\Run_Gate.ps1 <tokens>` (the arguments without `fix`).
   - Tokens: none / `quick` = quick suite; `bots` = every bot session; `GeoTrinity.<filter>` = those tests only (add `quick` for both); `nobuild` only when nothing changed since the last build. Which bot sessions a change needs: Coverage table, `Source/GeoTrinityEditor/Private/Tests/Tutorial/CLAUDE.md`.
   - Timeout 600000 ms. With `bots`, any `GeoTrinity.Tutorial.FullRun|Path|Jump` filter, or a build from an old tree: `run_in_background` and wait for the notification.
   - Output = the last line `GATE: ...` and what precedes it; copy in `AI/Output/Gate/last.txt`. Never open the full logs here.
2. **Act on the GATE line:**
   - `GREEN` (and no `fix`) → report one line: tokens + `Tests X Passed Y`. Done. No agent.
   - `RED` → mode `saving`: report the failures to the user, stop. Otherwise start `test-fixer` with the dev brief its description lists; the `Results` field = the gate output verbatim (failures, counts, config), `Tests` field = the same tokens. Opus for replication, GAS prediction or design-heavy failures.
   - `BLOCKED` → report the reason; never kill or close the process it names. Another session's run → retry once it is gone, if the user agrees.
   - `GREEN` with `fix` → start `test-fixer` anyway (same brief, `Results` = the green output), for whatever the user asked beyond the gate.
3. Act on the agent's report per root `CLAUDE.md` ("Test agent").
