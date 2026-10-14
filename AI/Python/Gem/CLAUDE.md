# AI/Python/Gem

Everything that builds or syncs the gem system: catalog, Core passives, the Gems menu pages, the CSV ↔ design-doc sync. Game side: `Source/GeoTrinity/Public/Gem/CLAUDE.md`, `AbilitySystem/Abilities/Gem/CLAUDE.md`. Skills: `gem-csv-to-doc`, `gem-doc-to-csv`.

| Script | Does |
|---|---|
| `gem_catalog.py` | Drop tables + class XP per source into `DA_GemCatalog`; import gems from `Data/gems.csv`; headless |
| `gem_core_passives.py` | Core gems' passives: BP per Core, `AbilityInfo` entry (Shared, not at startup), `GrantedAbility` cell of `Data/gems.csv`, `GE_CD_Core_<Core>` cooldown GE for Wake / Split / Kinship; headless `-run=pythonscript` |
| `gems_page.py` | Gems menu: `WBP_GemLoadout`, `WBP_GemForge`, `WBP_Gems`, tab underline, echo halo defaults; loads the `UI/` toolkit (`character_sheet.py`, `rail_style.py`) by path; run after `character_sheet.py` and `import_icons.py` |
| `gem_doc_sync.py` | `Data/gems.csv` ↔ the Gem System doc's catalog table, diff, and the stamp `Data/gems.sync.json` telling which side changed since last sync; plain Python, no editor |
