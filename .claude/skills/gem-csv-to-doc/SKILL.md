---
name: gem-csv-to-doc
description: Push the game's gem values (Data/gems.csv, plus the catalog's loot, Forge, XP and socket values in code) into the "GeoTrinity — Gem System" doc, updating its numbers, charts and rule descriptions; stops and asks when the doc was edited since the last sync
---

# Gem values: game → doc

The doc is https://claude.ai/artifact/SQYwGjtE7bspcCA5KKV67K (Claude Docs id `cdbe4119-4ccb-4017-a52c-70d469714c62`).
Its "Gem catalog" table is a 5-column view of `Data/gems.csv`: `Tier`, `Gem`, `Effect per gem`, `Max per class`,
`Attribute or system`. `AI/Python/Data/gem_doc_sync.py` keeps `Data/gems.sync.json`, the stamp of the last sync: the
CSV's hash and the doc table's hash `h`. Its `doc-table` and `diff` still expect an older 10-column layout, so use
`status` and `stamp` only and compare rows by hand, on `Gem` = CSV `Name`.
The opposite direction is the `gem-doc-to-csv` skill, which also holds the per-gem / max-per-class rules.

**Never overwrite a side edited since the last sync.** Whenever the doc table changed since the stamp, or there is
no stamp, show the user every difference and ask with AskUserQuestion. Never pick a winner yourself. Ask too about
anything else unclear.

## Steps

1. **Load the docs tools.** Load the docs skill (or call the connector's `guide` with `topic.index`), then read the
   doc's outline. The catalog table is the first `<table>` after the "Gem catalog" heading. Read it with
   `{"kind":"view","parentId":"<table id>"}`, and note its id and `h`.
2. **Check freshness.** Run `python AI/Python/Data/gem_doc_sync.py status <h>`.
   - Doc table unchanged since the stamp: go on.
   - Doc table changed, or no stamp:
     - Copy the table's cells exactly, as a pipe table, to `<scratchpad>/doc_table.md`, then run
       `gem_doc_sync.py diff <scratchpad>/doc_table.md`.
     - Ask the user what to do: keep the doc's edits (run `gem-doc-to-csv` first), overwrite them with the CSV, or
       choose gem by gem.
     - Compare the table with the CSV by hand (the script's layout is older than the doc's).
3. **Gather the game's values.**
   - Gems: `Data/gems.csv`.
   - Forge craft and break-down costs, loot and XP difficulty scales, XP per level: `UGeoGemCatalog` defaults in
     `Source/GeoTrinity/Public/Gem/GeoGemCatalog.h`.
   - Drop tables and XP per health bar: `AI/Python/Asset/gem_catalog.py`.
   - Sockets and unlock levels: `GeoGem::GetSockets()` in `Source/GeoTrinity/Private/Gem/GeoGemTypes.cpp`.
     `SOCKETS_PER_CLASS` in the script must match it; fix the script if it does not.
   - Base crit chance: `UGeoGemAttributeSet`'s constructor. Crit damage: `GeoAbilitySystemLibrary.cpp`.
   - A value someone hand-edited in `DA_GemCatalog`, other than a gem, is invisible from code. When the user says the
     asset differs, read it through the editor bridge.
4. **Update the catalog table cell by cell.** Per gem from `MagnitudePerGem`; max per class = per gem × sockets
   (Chip 30, Cut 15, Prism 9, Core 1); the number inside "Effect per gem" rewritten to match; an empty cell filled
   from the CSV. One `replace` per cell, a `find` target `within` it with `"as":"text"`; never replace the table.
5. **Update every value that depends on those numbers**, in place, changing only what differs:
   - The gem count sentence under "Gem catalog" (per-tier counts).
   - The sockets table and the class-level sentences, from `GetSockets()`.
   - Class XP: XP per bar, difficulty scales, XP per level, the XP to reach level 20 = 19 × XP per level.
   - The Loot table:
     - average count = (min + max) / 2 × difficulty scale;
     - Prisms = average × Prism chance;
     - Cuts = (average − Prisms) × Cut chance;
     - Chips = the rest.
   - The Forge table, and the "Shard math" list:
     - shards from a kill = the sum over tiers of (gems × break-down value);
     - full-set craft cost = the sum over tiers of (sockets × craft cost).
     - A Core counts 3 sockets here, one of each different Core.
   - "Power ceiling" and the outlier paragraphs: every number quoted from a gem's max.
   - Charts: each `embed` is a widget whose code holds a `sources` data block.
     - Read the widget node and change only the numbers in its data, never its drawing code.
     - Update the embed's caption when it quotes values.
     - The crit chart's data holds `chips`, `cuts`, `powerPerChip`, `precisionPerChip`, `precisionCap`,
       `edgePerCut`, `baseCritChance`, `baseCritDamage` and `damageCap`, in percent.
     - `precisionCap` and `damageCap` are design limits from the doc, not game values.
6. **Update rule descriptions when the code changed.**
   - Compare the doc's prose with the code behind it:
     - loot roll and XP: `GeoGemCatalog.cpp`;
     - equip, break-down and craft rules: `GeoGemProfileSave.cpp`;
     - stat application: `GeoGemStatsEffect.cpp`;
     - tag Cores: find each `GrantedTag` (`Gem.Core.*`) in the code;
     - passive Cores: the `GrantedAbility` Blueprint's C++ parent in
       `Source/GeoTrinity/*/AbilitySystem/Abilities/Gem/` holds the rule. Its tunables (angle, share, cooldown) are
       the `GA_Core_*` Blueprint's values, the C++ defaults unless someone edited the Blueprint.
   - Rewrite only the sentences the code now contradicts.
   - A Core with neither a checked tag nor a `GrantedAbility` is not built yet. Its doc text is design: leave it.
   - The doc table has no column for `GrantedTag` or `GrantedAbility`. Never add one.
   - A design limit the code does not enforce (such as the Precision cap) is not a contradiction. Mention it to the
     user instead.
7. **Stamp.** Re-read the outline for the table's new `h`, then run
   `gem_doc_sync.py stamp <table id> <h> csv-to-doc`.
8. **Report** in a few lines: values changed, sentences rewritten, design gaps noticed.
