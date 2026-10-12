# CLAUDE.md

## Project
GeoTrinity — multiplayer 2D boss-fight bullet-hell, Unreal Engine 5.7, GAS. Players are shapes (Tank=Square, Heal=Circle, DPS=Triangle). Orthographic camera.

## Workflow (every feature) — MANDATORY GATE
Applies to every session, every agent (interactive, daily, nightly, cloud). Default is the gate. Waiver, for that request only: the user directly asks for the code ("code the UI of this artifact", "just code it", "skip the docs", "quick fix") → do it straight away, no discussion, no docs, no Pending file. A goal or idea with no ask to code is not a waiver.

1. **Why** — the user states the goal. No file written.
2. **How** — discuss the approach, or the user dictates it. No file written.
3. **Write it down** — the `CLAUDE.md` of every code folder and `AI/Python` folder touched, written as if the feature exists (map, intent, invariants). Then ONE temporary `AI/Pending/<Feature>.md`: its `Goal:` line is the Why, then everything left to do (template: `AI/Pending/README.md`). Stop; the user reviews the docs.
   When the behaviour can be automated, the Pending file lists the test files to write (lightest layer, `AI/Testing.md`) and the run command.
   Sort every piece of info with one test: *still true and useful to a future task once this feature ships?* Yes (a lasting design rule, layout or behaviour choice) → the `CLAUDE.md` of the leaf folder that owns it. No (one-off steps, sources to fetch from, assets to import, values to copy) → the Pending file only, gone when it is deleted.
4. **What** — only on the user's go, with the context cleared or not: implement what the docs say (code, scripts, Pending items an editor script can do). Tick the Pending file, delete it when empty, and fix any CLAUDE.md that drifted from what was built.
   Tests: when possible, write test files next to the code in the same step (`AI/Testing.md`), and run them headless once built. A feature is not done without its tests unless they cannot be automated (say why).
   Verification: no editor testing by default (PIE, live editor, MCP in-editor checks) — only when the user asks. Big tasks (full feature, tricky refactor, …) must at least be built, overriding "Build only when the user asks"; then run their tests.

Gate rules:
- No code, script or asset change for a feature until step 3 is on disk (docs + Pending file), unless waived.
- Step 4 from a cleared context: work from the docs and the Pending file. Not cleared: first output line is `Context not cleared — using docs + discussion.`, then use both. Either way, a design change goes back into the docs.
- Exempt without waiver: typo / comment-only edits, build or test runs, answering questions, the daily JavaDoc task, reverting.
- Autonomous agents with no user to discuss with: write step 3 only, commit it, and stop; implementation waits for the user.

## Rules
- Build only when the user asks, except big tasks (full feature, tricky refactor, …) which must at least be built (workflow step 4); then the matching `Tools\*.bat` with `GEO_NO_PAUSE=1`, editor closed. Commands: `AI/Commands.md` (editor/dev), `AI/BuildPackage.md` (package). Small changes: hand over unbuilt code and say so. MCP live compile only for Blueprint-touching work.
- Never test in the editor (PIE, live editor, in-editor MCP checks) unless the user asks; automated tests (headless) are the default check.
- Never take mouse or window focus (clicking in the editor, PIE with real clicks, focus-dependent screen grabs) unless the user asked to test it yourself. Default tests: automated (`GeoGemTests.cpp`), headless, bridge scripts (`AI/Testing.md`).
- Launch / quit / dismiss dialogs of an editor only on request, and only your own (`-RemoteControlAllow`).
- Never write into `Saved/`. Sound work → `AI/Audio/`; reports, probes, exports → `AI/Output/`; throwaway → session scratchpad.
- Before writing any code: read `AI/CodingStyle.md`. Planning or answering: don't.
- Opening a `.cpp`: read it entirely, and the `CLAUDE.md` of its Public folder.
- Navigate by docs: this file → module `Public/CLAUDE.md` → folder `CLAUDE.md`; then Read / Grep those files. Explore agents only for undocumented areas.
- Before MCP / Python editor automation: `AI/MCP/CLAUDE.md`.
- Pitfalls and incident post-mortems: `TroobleshootingLogs.md` (root), not in CLAUDE.md.
- **No memory files, ever.** Never create or update anything under `~/.claude/projects/*/memory/`. A fact worth keeping goes in the file from "Where a fact goes" below, in the same session. A rule the user states ("always…", "never…") is written there, not remembered.
- Camera is orthographic: a position mismatch is never parallax or height; find the gameplay or network cause.

## Where a fact goes
| Kind of fact | File |
|---|---|
| How a class / folder works, invariant, ordering, replication rule | the leaf folder's `CLAUDE.md` |
| Cross-cutting design (networking, data structures, effects, cue transport) | `AI/Architecture.md` |
| Code style, reuse rules, module / API-macro rules | `AI/CodingStyle.md` |
| Build, compile check, editor config | `AI/Commands.md`, `AI/BuildPackage.md` |
| What broke and why (post-mortem), silent-loss traps | `TroobleshootingLogs.md` |
| Editor / Python / MCP technique or hazard | `AI/MCP/<topic>.md` (`AI/MCP/CLAUDE.md` → "Hazards") |
| Look of shapes, colours, motion, VFX, materials | `AI/ArtDirection.md`, `AI/VFX.md`, `AI/Materials.md` |
| UI rules, hand-edit protection | `AI/UI.md` |
| Sound: spirit per boss, kits, delivery, reference campaign | `AI/SoundCreation.md`, `AI/Audio/CLAUDE.md`, `AI/MCP/Audacity/` |
| Test and verification method | `AI/Testing.md` |
| Anything only this feature's implementation needs (one-off steps, sources, assets to fetch, editor / Blueprint work) | `AI/Pending/<Feature>.md` |
| How the agent must behave | this file ("Rules"), `AI/CLAUDE.md` ("Working rules") |

## Writing CLAUDE.md
- Written in workflow step 3; the daily agent only does JavaDoc comments (`AI/Tasks/`).
- A map, not documentation: purpose line, file → one-line role table, then only what code and comments do not say — intent, why, invariants, ordering, cross-class links.
- Fragments, not sentences. The good way of doing; never "don't do X" history (a real pitfall goes to `TroobleshootingLogs.md`).
- Pin-point down to the leaf folder; leaf = where the code's architecture is explained (the way we go, what we expect).
- ≥ 3 files of one kind → own subfolder with its own `CLAUDE.md`. Keep each doc small enough to load whole.

## Where things are
| Need | File |
|---|---|
| Gameplay module map (`Source/GeoTrinity/Public`) | `Source/GeoTrinity/Public/CLAUDE.md` |
| UI module (`GeoTrinityUI`) | `Source/GeoTrinityUI/Public/CLAUDE.md` |
| Editor module (`GeoTrinityEditor`) | `Source/GeoTrinityEditor/Public/CLAUDE.md` |
| Gameplay↔UI seam | `Source/GeoTrinity/Public/HUD/Interface/CLAUDE.md` |
| Everything under `AI/` (topic docs, MCP, scripts, audio) | `AI/CLAUDE.md` |
| Pending editor work of a feature | `AI/Pending/` |
| Past issues and fixes | `TroobleshootingLogs.md` |

## Modules
```
Source/GeoTrinity/        gameplay: AbilitySystem, Actor, AI, Characters, GameClasses, Gem, Input, Settings, System, Tool, World
Source/GeoTrinityUI/      runtime UI: HUD, Menu, Style. Depends on GeoTrinity, never the reverse; excluded from the server target
Source/GeoTrinityEditor/  editor only: builders, Details customizations, CSV sync, thumbnails
```
Headers in `Public/`, sources mirrored in `Private/`.
