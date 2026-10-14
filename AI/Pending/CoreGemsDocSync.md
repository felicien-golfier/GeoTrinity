# CoreGemsDocSync — pending

Goal: bring the Core gems in line with the "GeoTrinity — Gem System" design doc (artifact `SQYwGjtE7bspcCA5KKV67K`, rev 1213): its Core cooldowns and any Core it still wants. Echo socket excluded for now.
Docs written: none needed yet — Core cooldown seconds are tuning (script + GE assets), not a rule; `Abilities/Gem/CLAUDE.md` already says "seconds live on the GE". If Orbit / Transfer are developed, add their rows to that file.

## Audit result (doc vs code)
- 9 doc Cores = 9 CSV Cores = 9 implemented (Surplus, Overclock, Critical, Leverage stat-only; Relay, Split, Bond, Wake, Kinship passives). Numbers match: Overclock -15%, Critical 10% / 150%, Bond 15%, Relay 30 deg, Split half health, Leverage x0.7..x1.3.
- Mismatch: cooldowns. Doc Split 20 s, Wake 20 s, Kinship 12 s; code Split 10 s, Wake 8 s, Kinship 5 s (`COOLDOWNS` in `gem_core_passives.py`, CSV effect text "max 1 per N s").
- Not in catalog (doc "Candidate gems"): Orbit (deployable follows you slowly), Transfer (max health -30%, deployables +30% health; a second row says drain -30% instead). Waiting for a decision before any work.
- Doc inconsistencies to fix in the doc, not code: overview says "6 Chips, 8 Cuts" (table: 7 and 7); prose still calls Edge a Cut and uses 1% Precision / 3.33% Edge (table and CSV: Chip, 0.83% / 1.33%); Echo table repeats the old Edge numbers.

## Scripts
- [x] (edited) `AI/Python/Ability/gem_core_passives.py` — `COOLDOWNS = {"Wake": 20.0, "Split": 20.0, "Kinship": 12.0}`; STILL TO DO: rerun in the editor to rewrite `GE_CD_Core_Wake / Split / Kinship` durations (`AI/MCP/CLAUDE.md` first; editor only on the user's request).

## Data
- [x] `Data/gems.csv` — Effect text of Split, Wake, Kinship: "cooldown 20 s", "cooldown 20 s", "cooldown 12 s". Edited by hand; catalog asset follows via `GeoGemCsvSync` (effect text only).

## Code
- [ ] None for the three cooldowns (GE assets carry the seconds).

## Parked by the user (play the current system first)
- [ ] Orbit / Transfer candidates not developed. Question was: If yes: which Transfer variant (drain vs health), then a new Pending section per Core (`GeoXxxPassiveAbility` or stat attribute, tag, CSV row, icon `Core_<Gem>.svg`, test).

## Verify
- [ ] `Tools\Run_Tests.bat GeoTrinity.Gems` after the CSV sync (CSV <-> catalog equality test). No new test: cooldown seconds are GE asset data.
