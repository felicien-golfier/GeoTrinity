# CLAUDE.md

## Project
GeoTrinity — multiplayer 2D boss-fight bullet-hell, Unreal Engine 5.8, GAS. Players are shapes (Tank=Square, Heal=Circle, DPS=Triangle). Orthographic camera.

## Workflow (every feature) — MANDATORY FEATURE GATE
Applies to every session, every agent (interactive, daily, nightly, cloud). Default is the gate. Waiver, for that request only: the user directly asks for the code ("just code it", "skip the workflow", "quick fix", "straight") → do it straight away, no discussion, no docs, no Pending file. A goal or idea with no ask to code is not a waiver.

1. **Why** — the user states the goal. No file written.
2. **How** — discuss the approach, or the user dictates it. No file written. At the end, always propose an `auditor` run on the approach (optional, Rules).
3. **Write it down** — the `CLAUDE.md` of every code folder and `AI/Python` folder touched, written as if the feature exists (map, intent, invariants). Then ONE temporary `AI/Pending/<Feature>.md`: its `Goal:` line is the Why, its `Priority:` line (ask the user when unsure) with its row in the README "Order" table, its `Read map` of the files step 4 must read, then everything left to do (template: `AI/Pending/README.md`). Stop; the user reviews the docs.
   Grouping: a new feature is also a chance to regroup. Look for existing code of the same kind and fold it, with the new code, into a shared folder (own `CLAUDE.md`, ≥ 3 files rule below); list the moves in the Pending file.
   Tests: the Pending file lists them per `AI/CodingStyle.md` ("Tests").
   Sort every piece of info with one test: *still true and useful to a future task once this feature ships?* Yes (a lasting design rule, layout or behaviour choice) → the `CLAUDE.md` of the leaf folder that owns it. No (one-off steps, sources to fetch from, assets to import, values to copy) → the Pending file only, gone when it is deleted.
4. **What** — only on the user's go, with the context cleared or not: implement what the docs say (code, scripts, Pending items an editor script can do). Tick the Pending file, delete it when empty, and fix any CLAUDE.md that drifted from what was built.
   Tests: written and run per `AI/CodingStyle.md` ("Tests"). 
   Verifications : `/test` with your own tests and the others that could be impacted (filters). don't launch full tests if possible. be quick. Red → the skill hands it to `test-fixer`.
   End: Propose `/test quick` (plus bot sessions) and the `auditor`. By default : no editor testing (PIE, live editor, MCP in-editor checks) — only when the user asks. OR Big tasks (full feature, tricky refactor, …) must at least pass `/test quick`;

Gate rules:
- No code, script or asset change for a feature until step 3 is on disk (docs + Pending file), unless waived.
- Step 4 from a cleared context: work from the docs and the Pending file; read its `Read map`, and search further only when the code does not match it. Not cleared: first output line is `Context not cleared — using docs + discussion.`, then use both. Either way, a design change goes back into the docs.
- Exempt without waiver: typo / comment-only edits, build or test runs, answering questions, the daily JavaDoc task, reverting.
- Autonomous agents with no user to discuss with: write step 3 only, report it, and stop; implementation waits for the user.

## Bug workflow — test first - MANDATORY BUG GATE
A reported bug is not a feature: no Pending file, no step 1-3. Default, in order:
1. **Reproduce** — write the automated test that fails on the bug. Run it and ENSURE it does fails where the bug is ! (per `AI/CodingStyle.md` "Tests", `AI/Testing.md`);
2. **Find** — only then read the code and locate the cause.
3. **Fix** — Easy fix without search for it (Few lines changes, not complexe code) ? Fix it and laucnh the tests yourself. Otherwise hand it over to the `test-fixer` with rules (Context; Test to fix etc...)
4. A fix that needs a design change (not a local correction) goes back into the docs first, and into the feature workflow if it grows into one : Report to the user first.

Skip step 1 (go straight to find + fix) when: the user says so ("no test", "just fix it", …) for that bug; or the test system cannot be applied for his bug. Say in one line which exception applies. A bug that is hard but not impossible to test is still tested first; when a test truly cannot reproduce it (needs real input, timing only; editor visual), say why and ask before skipping.

## Rules
- **Test skill.** Every build + test run, by any agent, goes through the `test` skill (Skill tool, `skill: "test"`, `args` = tokens; written `/test` in these docs; `.claude/skills/test/SKILL.md`, script `Tools/Run_Gate.ps1`): process check, build, tests, one `GATE:` line. Green → one line, no agent. Red → the skill starts `test-fixer`. Never start `test-fixer` to run tests, nor `Tools\Build_Editor.bat` + `Tools\Run_Tests.bat` by hand. A Pending item or older doc naming those (or "quick suite via `test-fixer`") means `/test` with the same filters.
- **Test agent.** `test-fixer` (`.claude/agents/test-fixer.md`) runs only on a red gate or an explicit request, one at a time, with no edits under `Source/` meanwhile. Its prompt is the dev brief its description lists (`Results` = the gate output), written from this conversation or, after a cleared context, from the docs and Pending file. Act on its report without re-reading logs; design decisions it returns go to the user; continue the same agent with `SendMessage`.
- **Audit agent.** `auditor` (`.claude/agents/auditor.md`, read-only), prompt = the brief its description lists. Step 2: optional, on the approach. Step 4, optional exept if "on your own" or alone mode, once `/test` is green; It only reports; the main conversation checks each finding is real and worth doing now, implements the logical ones (code changed → `/test` again), and asks about any doubt once done.
- **Build** Build after each consequent code change (Several files, full 10+lines functions etc...). first test is this: `/test` builds before testing. A build without tests: `Tools\Build_Editor.bat` with `GEO_NO_PAUSE=1`. An open editor blocks nothing: `AI/Commands.md`, "Build and test beside an open editor". Commands: `AI/Commands.md` (editor/dev), `AI/BuildPackage.md` (package). Small changes: hand over unbuilt code and say so. MCP live compile only for Blueprint-touching work.
- **Quick Tests** Any change to code under `Source/`, `Content/Maps/Tutorial.umap`, a tutorial script or the tutorial's content passes `/test quick`: every `GeoTrinity.` and `GeoTrinityUI.` test except the bot sessions `GeoTrinity.Tutorial.FullRun`, `.Path.*` and `.Jump.*`. Not green = not done,go back to Bug workflow step 2.
- **Tutorial runs at risk (mandatory).** A change that can alter how the tutorial plays (entering, skipping or jumping to a room, moving between rooms, the flow of a step, anything a room relies on) also runs, without asking, the bot sessions covering that behaviour (`/test quick <their filters>`): the Coverage table in `Source/GeoTrinityEditor/Private/Tests/Tutorial/CLAUDE.md` names them. In doubt, run them. Not green = not done.
- **Tutorial runs (optional, proposed at the end).** Any other change: the slow play-in-editor runs of the real map (`Source/GeoTrinityEditor/Private/Tests/Tutorial/`, `AI/Testing.md` "Tutorial runs"), `GeoTrinity.Tutorial.FullRun` at least. Once the Quick Tests is green, propose them with the audit in one message; they run only on the user's yes (or "on your own" or alone mode), through `/test` (`bots` or their filters).
- Never test in the editor (PIE, live editor, in-editor MCP checks) unless the user asks; automated tests (headless) are the default check.
- Never take mouse or window focus (clicking in the editor, PIE with real clicks, focus-dependent screen grabs) unless the user asked to test it yourself. Default tests: automated (`Tests/Gameplay/`), headless, bridge scripts (`AI/Testing.md`).
- Launch / quit / dismiss dialogs of an editor only on request, and only your own (`-RemoteControlAllow`).
- Full autonomy (Like alone mode): when the user says "on your own", "I'm out", "you're alone", "Finish it up"  (or similar), every optional rule above is lifted for that task: build, test, play in editor, take focus, and even close the user's editor or test in their open editor. Even Workflow stops are removed, you can implement straight after step 3 directly. Don't stop until task is fully done : Planned, implemented, tested.
- **Mode.** One word in `AI/Output/mode.txt` (`saving` | `normal` | `alone`; missing = `normal`), shown by the status line before `five_hour`. Read it at session start; the user switches it by asking, write the file. An explicit user request beats the mode.
  - `normal`: these rules as written.
  - `saving`: least tokens. Dev only: no tests, no audit, no agents, no MCP, no build unless asked. Anything beyond the dev is skipped or left to the user (asset tweaks: user does them). No futile questions.
  - `alone`: every task runs as "Full autonomy" above, by default.
- Empty folder left by a move or delete (code, docs, scripts): delete it in the same change, always; no ask needed. Same for stale content : Remove all assets that are replaced by new code, or useless.
- Never write into `Saved/`. Sound work → `AI/Audio/`; reports, probes, exports → `AI/Output/`; throwaway → session scratchpad.
- Before writing any code: read `AI/CodingStyle.md`. Planning or answering: don't.
- Opening a `.cpp`: read it entirely, and the `CLAUDE.md` of its Public folder.
- Navigate by docs: this file → module `Public/CLAUDE.md` → folder `CLAUDE.md`; then Read / Grep those files. Explore agents only for undocumented areas.
- **Reading for a decision.** Steps 1-3 happen in the main conversation, which reads the docs and every file a decision depends on, in full; no summary stands in for them. A wide question ("who calls this?", "which abilities do X?") goes to the `code-excerpts` agent (`.claude/agents/code-excerpts.md`); a decision resting on an excerpt reads that whole file.
- Before MCP / Python editor automation: `AI/MCP/CLAUDE.md`.
- **Map gate.** Before launching a script that writes a map: `AI/Python/Level/CLAUDE.md`, "Map gate". A map changed by hand blocks the script until its changes are carried in.
- `TroobleshootingLogs.md` (root): only hard-to-find bugs we spent time figuring out, and only when the user says so. Never add an entry on your own; when one seems worth it, ask.
- **No memory files, ever.** Never create or update anything under `~/.claude/projects/*/memory/`. A fact worth keeping goes in the file from "Where a fact goes" below, in the same session. A rule the user states ("always…", "never…") is written there, not remembered.
- **Research page.** Every research / study / concept-exploration artifact (visual studies, design options, room or feature research) goes into the existing "Research" artifact (`https://claude.ai/artifact/RZhzLd3DV8276eMvRo7bbF`) as a new tab (`pages` array + `<main id="page…">`), never a separate artifact. Read the whole saved file before republishing.
- **Exceptions to workflow** When the task is really small, add a uproperty, rename something, add logs, fix a line in code etc... Also sentences like "Fix it straight", "code right away" or such...  Ignore the workflow ! 

## Where a fact goes
| Kind of fact | File |
|---|---|
| How a class / folder works, invariant, ordering, replication rule | the leaf folder's `CLAUDE.md` |
| Cross-cutting design (networking, data structures, effects, cue transport) | `AI/Architecture.md` |
| Code style, reuse rules, module / API-macro rules | `AI/CodingStyle.md` |
| Build, compile check, editor config | `AI/Commands.md`, `AI/BuildPackage.md` |
| Hard-to-find bug and its fix, when the user asks for it (Rules) | `TroobleshootingLogs.md` |
| Editor / Python / MCP technique or hazard | `AI/MCP/<topic>.md` (`AI/MCP/CLAUDE.md` → "Hazards") |
| Look of shapes, colours, motion, VFX, materials | `AI/ArtDirection.md`, `AI/VFX.md`, `AI/Materials.md` |
| UI rules, hand-edit protection | `AI/UI.md` |
| Sound: spirit per boss, kits, delivery, reference campaign | `AI/SoundCreation.md`, `AI/Audio/CLAUDE.md`, `AI/MCP/Audacity/` |
| Test and verification method | `AI/Testing.md` |
| Anything only this feature's implementation needs (one-off steps, sources, assets to fetch, editor / Blueprint work) | `AI/Pending/<Feature>.md` |
| How the agent must behave | this file ("Rules"), `AI/CLAUDE.md` ("Working rules") |

## Writing CLAUDE.md
- Written in workflow step 3; the daily agent only does JavaDoc comments (`AI/Tasks/`) and never rewrites a `CLAUDE.md` to follow the code — only fixes a mistake or fills a missing part.
- A map, not documentation: purpose line, file → one-line role table, then only what code and comments do not say — intent, why, invariants, ordering, cross-class links.
- Succinct, every doc: fewest words that carry the fact. Fragments, not sentences. The good way of doing; never "don't do X" history (a real pitfall: ask the user about `TroobleshootingLogs.md`).
- Pin-point down to the leaf folder; leaf = where the code's architecture is explained (the way we go, what we expect).
- ≥ 3 files of one kind → own subfolder with its own `CLAUDE.md`. Keep each doc small enough to load whole.
- Each fact in one place only; any other section that needs it points to it (as step 3 "Grouping" points to the ≥ 3 files rule above).

## Where things are
| Need | File |
|---|---|
| Gameplay module map (`Source/GeoTrinity/Public`) | `Source/GeoTrinity/Public/CLAUDE.md` |
| UI module (`GeoTrinityUI`) | `Source/GeoTrinityUI/Public/CLAUDE.md` |
| Editor module (`GeoTrinityEditor`) | `Source/GeoTrinityEditor/Public/CLAUDE.md` |
| Gameplay↔UI seam | `Source/GeoTrinity/Public/HUD/Interface/CLAUDE.md` |
| Everything under `AI/` (topic docs, MCP, scripts, audio) | `AI/CLAUDE.md` |
| Out-of-editor data (plain files, not assets): `gems.csv`, `AbilityDescriptions.txt` | `Data/` at project root (read via `FPaths::ProjectDir()`; runtime files staged by `RuntimeDependencies` in `GeoTrinity.Build.cs`) |
| Pending editor work of a feature | `AI/Pending/` |
| Past issues and fixes | `TroobleshootingLogs.md` |
