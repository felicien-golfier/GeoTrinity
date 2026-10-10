---
name: gem-doc-to-csv
description: Pull the gem values edited in the "GeoTrinity — Gem System" doc's catalog table into Data/gems.csv, keeping per-gem and max-per-class consistent in both, refreshing stale effect text and filling empty fields; refuses to overwrite a CSV changed since the last sync without asking
---

# Gem values: doc → game

The doc is https://claude.ai/artifact/SQYwGjtE7bspcCA5KKV67K (Claude Docs id `cdbe4119-4ccb-4017-a52c-70d469714c62`).
Its "Gem catalog" table is a 5-column view of `Data/gems.csv`: `Tier`, `Gem`, `Effect per gem`, `Max per class`,
`Attribute or system`. The CSV holds more (`Id`, `Operation`, `Color`, `GrantedTag`, `GrantedAbility`): the doc never
shows them, so a write to the CSV keeps every column it does not own. `AI/Python/Data/gem_doc_sync.py` keeps
`Data/gems.sync.json`, the stamp of the last sync (the CSV's hash and the doc table's hash `h`); its `to-csv`,
`doc-table` and `diff` still expect an older 10-column doc layout, so do not run them. Use `status` and `stamp` only.

**The two Core columns belong to the game, never to the doc.** "Attribute or system" in the doc is prose
("Dash ability"), not either of them.
- `GrantedTag` (`Gem.Core.Critical`, `Gem.Core.Leverage`, `Gem.Core.Surplus`): a native tag the code of a tag Core
  checks.
- `GrantedAbility` (`/Game/AbilitySystem/Abilities/Gem/GA_Core_Bond.GA_Core_Bond_C`): the Core's passive ability
  Blueprint, a child of a `UGeoCorePassiveAbility` C++ class that holds the rule. The gem gives it while slotted.
  Only `AI/Python/Ability/gem_core_passives.py` writes this cell, when it creates the Blueprint.
- A Core holds one or the other. A Core with neither has no rule in the game yet.
- The editor refuses the whole CSV when a `GrantedAbility` path does not load as a Core passive class, so never type
  or guess one.
- A Core's tunables (Relay's angle, Bond's 15%, Wake's 8 s) live on its `GA_Core_*` Blueprint, not in the CSV. A
  value changed in a Core's rule text in the doc is a Blueprint change, so ask before touching it.
The opposite direction is the `gem-csv-to-doc` skill.

**Never overwrite a CSV changed since the last sync.** The game's values may have moved: through the editor's
Details panel, a script, git or a hand edit. When the CSV changed since the stamp, or there is no stamp, show the
user every difference and ask with AskUserQuestion. Never pick a winner yourself. Ask too about anything else
unclear.

## The rules this skill keeps

- **Per gem and max per class are one number.** Max per class = per gem × the class's sockets of that tier:
  Chip 30, Cut 15, Prism 9, Core 1 (`GeoGem::GetSockets()`).
  - The user may edit either cell. The most recently edited one wins (the cell with the higher `rev` in the read).
  - Derive the other: a new max sets per gem = max ÷ sockets; a new per gem sets max = per gem × sockets.
  - A pair already within display rounding (per gem shown to 2 decimals × sockets) is consistent: leave it.
  - Equal `rev`, or both cells edited since the last sync and disagreeing: ask.
  - CSV `MagnitudePerGem` is the per-gem fraction (`0.0333333` for 3.33%), 6 significant digits, signed.
- **Descriptions follow the numbers.** The number inside "Effect per gem" ("Reload speed +1%") is rewritten to
  the new per gem value. Other doc prose quoting a changed gem value is listed in step 8 for the `gem-csv-to-doc`
  skill's step 5.
- **Empty fields get filled.** A blank or `—` cell in the doc takes the CSV's value (and the other way round for
  `Effect`), never invented. Leave `Attribute` empty for Cores that have none. `GrantedTag` and `GrantedAbility` are
  never filled from the doc (see above).
- **A Core added in the doc** gets a CSV row with both Core columns empty. Say it has no rule in the game until its
  passive class is written and `gem_core_passives.py` is run with it added to `CORES`.
- **Rows match on `Gem` = CSV `Name`** (Wind-up is `Name` "Wind-up", `Id` "WindUp"), never on position.
- **Core rule text is a rule, not a number.** If the doc's text for a Core differs from the CSV `Effect` (a chance
  against a cooldown, a different value), the code behind it decides: ask which is right, do not edit one side.

## Steps

1. **Load the docs tools.** Load the docs skill (or call the connector's `guide` with `topic.index`), then read the
   doc's outline. The catalog table is the first `<table>` after the "Gem catalog" heading. Read it with
   `{"kind":"view","parentId":"<table id>"}`, and note its id, `h`, and each row's per-gem and max cell `rev`.
2. **Check freshness.** Run `python AI/Python/Data/gem_doc_sync.py status <h>`.
   - Doc table unchanged since the stamp: nothing to pull; say so and stop.
   - CSV changed since the stamp, or no stamp: compare the doc rows with the CSV by hand and show the user every
     difference (CSV value / doc value). Ask which side wins: the doc, the CSV (then run `gem-csv-to-doc` instead),
     or gem by gem. A no-stamp CSV that differs from `HEAD` only by new columns is not a value change.
3. **Reconcile** every row with the rules above. List the table for the user: gem, per gem before → after, max
   before → after, which cell won.
4. **Write the CSV** with a short inline Python that loads it with `csv.DictReader`, sets `MagnitudePerGem` (and
   `Effect`, only for a Core whose rule text was confirmed or an empty field), and writes it back with every column
   and `lineterminator="\n"`. `GrantedTag` and `GrantedAbility` stay as they were. Check `git diff Data/gems.csv`
   touches only the intended cells. No changed `GrantedAbility` cell, and the header still ends in
   `GrantedTag,GrantedAbility`.
5. **The import into the game.**
   - **Editor open:** `UGeoGemCsvSync` imports the file within a second. It checks attributes, operations, colours,
     tags, Core passive classes and the gem stats effect's support. It either saves `DA_GemCatalog` or pops a toast
     naming each bad line and changes nothing. Read `Saved/Logs/GeoTrinity.log` (read only) for the `Gem CSV` line, and report a
     rejection to the user.
   - **No editor open:** the next editor launch imports it, or the user runs
     `AI/Python/Asset/gem_catalog.py` headless.
6. **Write back into the doc** the cells the reconcile derived (the other of per gem / max, and the effect text),
   one `replace` per cell with a `find` target `within` that cell and `"as":"text"`, so comments and revisions stay.
   Never replace the whole table. If the docs write is refused, stop there, say which cells are still stale, and
   leave the stamp unwritten.
7. **Stamp.** Re-read the outline for the table's new `h`, then run
   `gem_doc_sync.py stamp <table id> <h> doc-to-csv`.
8. **Values outside the table.** Loot, Forge, XP, sockets and rule text are not in the CSV.
   - If the doc changed any of them, list each change and ask whether to apply it in code or the asset.
   - Where each lives is listed in step 3 of the `gem-csv-to-doc` skill.
   - Prose elsewhere in the doc that quotes a gem value that changed (Power ceiling, outlier paragraphs, chart
     data) is stale: list it and offer to run `gem-csv-to-doc` step 5.
9. **Report** in a few lines: gems changed (before → after), cells written back, import result, and any Core with
   neither `GrantedTag` nor `GrantedAbility` (no rule in the game yet).
