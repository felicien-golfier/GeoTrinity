# CritBase — pending

Goal: crits work without the Critical Core (base +20% damage); the Core only adds +30% crit damage and +10% crit chance.
Docs written: `Source/GeoTrinity/Public/Gem/CLAUDE.md` (Invariants, "Crits belong to everyone").

## Editor
- [ ] `DA_GemCatalog` Critical row — `Data/gems.csv` is changed but the asset is not: open the editor and save the catalog (or run the CSV sync) so it reads `CritDamage`, `Add`, 0.3 (no editor was opened). Then delete this file.

## Status 2026-10-25
`Data/gems.csv` and `Gem/CLAUDE.md` match. Remaining: save `DA_GemCatalog` / CSV sync so the asset reads `CritDamage Add 0.3`, then delete this file.
