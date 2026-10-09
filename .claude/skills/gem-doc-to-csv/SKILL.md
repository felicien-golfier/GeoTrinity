---
name: gem-doc-to-csv
description: Pull the gem values edited in the "GeoTrinity — Gem System" doc's catalog table into Data/gems.csv, which the open editor imports into DA_GemCatalog; refuses to overwrite a CSV changed since the last sync without asking
---

# Gem values: doc → game

The doc is https://claude.ai/artifact/SQYwGjtE7bspcCA5KKV67K (Claude Docs id `cdbe4119-4ccb-4017-a52c-70d469714c62`).
Its "Gem catalog" table mirrors `Data/gems.csv` column for column. `AI/Python/Data/gem_doc_sync.py` converts between
the two and keeps `Data/gems.sync.json`, the stamp of the last sync: the CSV's hash and the doc table's hash `h`.
The opposite direction is the `gem-csv-to-doc` skill.

**Never overwrite a CSV changed since the last sync.** The game's values may have moved: through the editor's
Details panel, a script, git or a hand edit. When the CSV changed since the stamp, or there is no stamp, show the
user every difference and ask with AskUserQuestion. Never pick a winner yourself. Ask too about anything else
unclear.

## Steps

1. **Load the docs tools.** Load the docs skill (or call the connector's `guide` with `topic.index`), then read the
   doc's outline. The catalog table is the first `<table>` after the "Gem catalog" heading. Read it with
   `{"kind":"view","parentId":"<table id>"}`, and note its id and `h`.
2. **Copy the table.** Write its cells exactly as they read, header row first, as a markdown pipe table to
   `<scratchpad>/doc_table.md`. Write empty cells as `—`, and escape a `|` inside a cell as `\|`.
3. **Check freshness.** Run `python AI/Python/Data/gem_doc_sync.py status <h>`.
   - Doc table unchanged since the stamp: nothing to pull; say so and stop.
   - CSV changed since the stamp, or no stamp: run `gem_doc_sync.py diff <scratchpad>/doc_table.md`. Show the user
     every difference (CSV value / doc value). Ask which side wins: the doc, the CSV (then run `gem-csv-to-doc`
     instead), or gem by gem. For gem by gem, edit `doc_table.md` to the chosen values before going on.
4. **Preview.** Run `diff` again and show the user what the CSV is about to receive. "No difference" means only
   the stamp needs writing.
5. **Write.** Run `gem_doc_sync.py to-csv <scratchpad>/doc_table.md`. The script rejects:
   - a header other than `DOC_COLUMNS`;
   - a wrong cell count;
   - an unknown tier;
   - a "Per gem" cell that is not a percentage.

   "Max per class" is computed, and ignored on the way in.
6. **The import into the game.**
   - **Editor open:** `UGeoGemCsvSync` imports the file within a second. It checks attributes, operations, colours,
     tags and the gem stats effect's support, and either saves `DA_GemCatalog` or pops a toast naming each bad line
     and changes nothing. Read `Saved/Logs/GeoTrinity.log` (read only) for the `Gem CSV` line, and report a
     rejection to the user.
   - **No editor open:** the next editor launch imports it, or the user runs
     `AI/Python/Asset/gem_catalog.py` headless.
7. **Stamp.** Run `gem_doc_sync.py stamp <table id> <h> doc-to-csv`.
8. **Values outside the table.** Loot, Forge, XP, sockets and rule text are not in the CSV.
   - If the doc changed any of them, list each change and ask whether to apply it in code or the asset.
   - Where each lives is listed in step 3 of the `gem-csv-to-doc` skill.
9. **Report** in a few lines: gems changed, import result.
