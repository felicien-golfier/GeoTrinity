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
- Death never removes gems: the stats GE stays on the ASC (`GetSurvivingEffectHandles()` = the stats handle; any future gem-granted effect handle is appended there) and Core passives stay active. Revive and class change re-run `ApplyGems`, which replaces both.
- `FGeoGemLoadout::Level` rides with each sent loadout (filled by `GetLoadout`); the server reads it with `UGeoGemComponent::GetClassLevel` (leaderboard stamps it on each player).

## Invariants
- Lowering a class level empties every locked-again socket of all its builds (`SetClassLevel`); fill (`FillSockets`) replaces slotted gems.
- `UnequipTier(Catalog, PlayerClass, Tier)` empties every socket of that tier in the build PlayerClass plays, and its echo when the echoed gem is of that tier; the copies go back to free. There is no single-socket unequip in the menus (a socket changes by replacement), `Unequip` stays for `SetClassLevel` and the dev commands.
- Gem tier fits only a socket of its own tier; a Core's rule is its `GrantedAbility` (`Abilities/Gem/CLAUDE.md`).
- A Core is in one socket per build: slotting it into another socket moves it (the previous socket empties, `Equip`; the copy moves, no free copy needed). Its own socket is refused. `FillSockets` of a Core slots one socket at most, none when the build already holds it.
- Refusals have a reason: `GetEquipRefusal` / `GetEchoRefusal` return an `EGeoGemRefusal` (`None` = allowed); `CanEquip` / `CanEcho` are `== None`. The menus turn the reason into a message (`HUD/Menu/Gems/CLAUDE.md`, "Messages").
- Socket order = loadout index order = save format: append, never reorder. Three clusters of seven steps over levels 1-20, the Core in the last step.
- `FGeoGemInfo::Id` is the key in saves and loadouts; renaming orphans every owned copy.
- `Operation`: `Add` (attributes starting at 0 or multipliers), `Percent` (flat values), `Flat` (counts).
- Drop roll goes high tier down: Core chance, else Prism, else Cut, else Chip.
- New gem stat = attribute in `UGeoGemAttributeSet` + catalog row.
- Crits belong to everyone, no Core needed: `RollCritMultiplier` rolls `CritChance` (base 0%, raised by Precision) for `CritDamage` (base x1.2, raised by Edge). The Critical Core adds +30% `CritDamage` (its catalog row) and +10% `CritChance` (`UGeoGemStatsEffect`, constant next to the `Gem.Core.Critical` grant); the tag stays for the menus.
- Editor sync: `GeoGemCsvSync` (`GeoTrinityEditor/Gem`) keeps catalog gems and the CSV equal, both ways; tests in `Private/Tests/GeoGemTests.cpp`.

## Echo socket
Design: "GeoTrinity — Gem System" doc, "Echo socket". One extra socket per build, outside `GetSockets()` (that order is the save format): `FGeoGemLoadout::Echo`, no field on `FGeoGemBuild`.
- Socket is open on every class once the shared echo level is 1 or more (the first class at `GeoGem::EchoUnlockLevel` = `MaxClassLevel`, 20, gets it there), so a level 1 class of another type can fill it; locked and grayed out while the level is 0. Holds one Chip / Cut / Prism; never a Core.
- Echo level: ONE per profile, shared by the three classes (`UGeoGemProfileSave::EchoLevel`, `EchoXp`; nothing per class). 0 .. `EchoMaxLevel` (100); 0 = not unlocked, never shown, worth nothing. The first class to reach level 20 takes it 0 → 1 (any class at 20 ⇔ level ≥ 1, held by `SetClassLevel`, `AddClassXp` and `Load`); from then it is shown in every UI whatever class is played, even one under 20, whose socket is open and gets the bonus.
- XP routing is by the class played: under level 20 its XP goes to the class (`FGeoGemClassProgress::Xp`); at level 20 it feeds the shared echo: `AddClassXp`'s level-up loop continues into echo levels, each costing `EchoXpCurve(level reached)` (`Curve_EchoXp` in `/Game/AbilitySystem/Data`, required). Changing to a class under 20 sends XP back to that class. Shards play no part.
- Every echo XP point is stored: at `EchoMaxLevel` the level stops but `EchoXp` keeps growing (never zeroed, never shown). No migration: saves from before the shared level read echo 0, the invariant above lifts it to 1 when a class is at 20.
- XP bar: `GetXpProgress` is the one fill (class XP, then echo XP, no difference) for the character sheet and the HUD XP strip; `UGeoGemSubsystem::OnProfileCommitted` fires after every `CommitChanges` for widgets that follow the profile.
- Endless to the player: `EchoMaxLevel` and the XP kept past it are never shown. No "n / max", no pips, no "MAX" label, no "next level" text that ends; the echo level reads as a bare number that always can grow.
- Share per tier: each echo level adds `EchoPercentPerLevel` (1%) to ONE tier, cycling `EchoTierOrder` (Chip, Cut, Prism). Level 1 = Chip 1%, level 2 = Cut 1%, level 3 = Prism 1%, level 4 = Chip 2%… Level 100 = Chip 34%, Cut 33%, Prism 33%.
- Value: the echo is worth `Catalog.GetEchoCopies(Tier, EchoLevel)` slotted copies = the tier's socket count in `GetSockets()` × its share; times `MagnitudePerGem`, same attribute / operation as a slotted copy. The one formula: the stats GE (`UGeoGemStatsEffect::MakeSpec`) and the menus (`GetMagnitudeText(Copies)`) both read it.
- Echo level rides with the sent loadout like `Level` (`FGeoGemLoadout::EchoLevel`, filled by `GetLoadout`).
- Last placed wins, per build: a type is in the echo socket or in sockets, never both. Echoing a type the build slots empties those sockets (`SetEcho`, no confirmation), so `CanEcho` needs one owned copy, not a free one. Slotting the echoed type clears the echo and fills the socket (`Equip`; the echo's copy moves, no free copy needed). Echoing the gem already there is refused.
- Copies: the echoed gem takes one copy like a socket; `GetFreeCount` and `GetBreakableCount` count it (`CountSlotted`).
- The echo follows class levels, not a past moment: while no class is at 20 the echo level and XP are 0 and every build's echo gem is emptied (`SyncEchoWithClassLevels`, run by `SetClassLevel`, `AddClassXp`, `Load`); any class at 20 lifts it to 1. Lowering one class while another stays at 20 changes nothing.
- Dev commands (non-shipping): `Geo.Gems.Echo <class> <GemId|None>`, `Geo.Gems.AddXp <class> <Xp>`, `Geo.Gems.SetEchoLevel <Level>` (shared, 0 .. max, XP restarts at 0; above 0 with no class at 20 it takes Triangle to 20 first); `Geo.Gems.Dump` lists the shared echo level / XP and each class's echo gem.
