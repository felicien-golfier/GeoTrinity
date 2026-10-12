# Gem

Per-class gem progression: loot, class XP, Forge, sockets, loadouts. Values live in `Data/gems.csv` (design doc sync: skills `gem-csv-to-doc`, `gem-doc-to-csv`).

| File | Role |
|---|---|
| `GeoGemTypes.h` | `EGeoGemTier` (Chip / Cut / Prism / Core), `FGeoGemInfo`, `FGeoGemDropTable`, `FGeoGemReward`, `FGeoGemLoadout`, `GeoGem::GetSockets()` |
| `GeoGemCatalog.h` | `UGeoGemCatalog` primary data asset (`GameDataSettings` points at it): gems per tier, drop tables per `EGeoBossType`, class XP, Forge rates |
| `GeoGemProfileSave.h` | `UGeoGemProfileSave` SaveGame per local player: owned gems, shards, per-class level / XP / builds |
| `GeoGemSubsystem.h` | `ULocalPlayerSubsystem` owning the profile; `CommitChanges()` saves and sends loadouts to the server |
| `GeoGemComponent.h` | On `AGeoPlayerState`: server half, applies the current class's loadout to the ASC |
| `GeoGemStatsEffect.h` | Native infinite GE carrying all slotted gem stats as SetByCaller modifiers |

## Flow
- Kill → `FGeoGemReward` (class XP for the class played + gems rolled from the drop table, difficulty-scaled) → profile → `CommitChanges()`.
- `CommitChanges()` → disk + `UGeoGemComponent::ServerSetLoadout` → server applies `UGeoGemStatsEffect` spec + Core passive abilities on the ASC (`APlayableCharacter::ResetAttributes`).
- Server trusts the client loadout; owned counts are never sent.
- `FGeoGemLoadout::Level` rides with each sent loadout (filled by `GetLoadout`); the server reads it with `UGeoGemComponent::GetClassLevel` (leaderboard stamps it on each player).

## Invariants
- Lowering a class level empties every locked-again socket of all its builds (`SetClassLevel`); fill (`FillSockets`) replaces slotted gems.
- Gem tier fits only a socket of its own tier; a Core's rule is its `GrantedAbility` (`Abilities/Gem/CLAUDE.md`).
- Socket order = loadout index order = save format: append, never reorder. Three clusters of seven steps over levels 1-20, the Core in the last step.
- `FGeoGemInfo::Id` is the key in saves and loadouts; renaming orphans every owned copy.
- `Operation`: `Add` (attributes starting at 0 or multipliers), `Percent` (flat values), `Flat` (counts).
- Drop roll goes high tier down: Core chance, else Prism, else Cut, else Chip.
- New gem stat = attribute in `UGeoGemAttributeSet` + catalog row.
- Editor sync: `GeoGemCsvSync` (`GeoTrinityEditor/Gem`) keeps catalog gems and the CSV equal, both ways; tests in `Private/Tests/GeoGemTests.cpp`.
