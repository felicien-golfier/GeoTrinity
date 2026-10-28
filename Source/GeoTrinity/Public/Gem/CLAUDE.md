# Gem

Per-class gem progression: loot, class XP, Forge, sockets, loadouts. Values live in `Data/gems.csv` (design doc sync: skills `gem-csv-to-doc`, `gem-doc-to-csv`).

| File | Role |
|---|---|
| `GeoGemTypes.h` | `EGeoGemTier` (Chip / Cut / Prism / Core), `FGeoGemInfo`, `FGeoGemDropTable`, `FGeoBossLoot`, `FGeoGemReward`, `FGeoGemLoadout`, `GeoGem::GetSockets()` |
| `GeoGemCatalog.h` | `UGeoGemCatalog` primary data asset (`GameDataSettings` points at it): gems per tier, drop tables per `EGeoBossType`, class XP, Forge rates |
| `GeoGemProfileSave.h` | `UGeoGemProfileSave` SaveGame per local player: owned gems, shards, per-class level / XP / builds |
| `GeoGemSubsystem.h` | `ULocalPlayerSubsystem` owning the profile; `CommitChanges()` saves and sends loadouts to the server |
| `GeoGemComponent.h` | On `AGeoPlayerState`: server half, applies the current class's loadout to the ASC |
| `GeoGemMeshes.h` | `GeoGem::GetTierMesh(Tier)` / `GetShardMesh(Variant)`: soft paths of the drop meshes ("3D gems", "Shards") |
| `GeoGemStatsEffect.h` | Native infinite GE carrying all slotted gem stats as SetByCaller modifiers |

## Flow
- Kill → `FGeoGemReward` (class XP for the class played + the boss's kill loot: gems and shards, "Boss loot") → profile → `CommitChanges()`.
- `CommitChanges()` → disk + `UGeoGemComponent::ServerSetLoadout` → server applies `UGeoGemStatsEffect` spec + Core passive abilities on the ASC (`APlayableCharacter::ResetAttributes`).
- Server trusts the client loadout; owned counts are never sent.
- Death never removes gems: the stats GE stays on the ASC (`GetSurvivingEffectHandles()` = the stats handle; any future gem-granted effect handle is appended there) and Core passives stay active. Revive and class change re-run `ApplyGems`, which replaces both.
- `FGeoGemLoadout::Level` rides with each sent loadout (filled by `GetLoadout`); the server reads it with `UGeoGemComponent::GetClassLevel` (leaderboard stamps it on each player).

## Invariants
- Lowering a class level empties every locked-again socket of all its builds (`SetClassLevel`); fill (`FillSockets`) replaces slotted gems.
- `UnequipTier(Catalog, PlayerClass, Tier)` empties every socket of that tier in the build PlayerClass plays, and its echo when the echoed gem is of that tier; the copies go back to free. There is no single-socket unequip in the menus (a socket changes by replacement), `Unequip` stays for `SetClassLevel` and the dev commands.
- Gem tier fits only a socket of its own tier; a Core's rule is its `GrantedAbility` (`Abilities/Gem/CLAUDE.md`).
- A Core is in one socket per build: slotting it into another socket moves it (the previous socket empties, `Equip`; the copy moves, no free copy needed). Its own socket is refused. `FillSockets` of a Core slots one socket at most, none when the build already holds it.
- Events: `OnGemEquipped(PlayerClass, SocketIndex, GemId)` after every successful `Equip` (`FillSockets`: once per socket filled), `OnGemCrafted(GemId, Quantity)` after a successful `Craft`; refusals broadcast nothing. Native, local to the machine owning the profile (tutorial triggers, `Encounter/Trigger/CLAUDE.md`).
- Refusals have a reason: `GetEquipRefusal` / `GetEchoRefusal` return an `EGeoGemRefusal` (`None` = allowed); `CanEquip` / `CanEcho` are `== None`. The menus turn the reason into a message (`HUD/Menu/Gems/CLAUDE.md`, "Messages").
- Socket order = loadout index order = save format: append, never reorder. Three clusters of seven steps over levels 1-20, the Core in the last step.
- `FGeoGemInfo::Id` is the key in saves and loadouts; renaming orphans every owned copy.
- `Operation`: `Add` (attributes starting at 0 or multipliers), `Percent` (flat values), `Flat` (counts).
- Drop roll goes high tier down: Core chance, else Prism, else Cut, else Chip.
- New gem stat = attribute in `UGeoGemAttributeSet` + catalog row.
- Crits belong to everyone, no Core needed: `RollCritMultiplier` rolls `CritChance` (base 0%, raised by Precision) for `CritDamage` (base x1.2, raised by Edge). The Critical Core adds +30% `CritDamage` (its catalog row) and +10% `CritChance` (`UGeoGemStatsEffect`, constant next to the `Gem.Core.Critical` grant); the tag stays for the menus.
- Editor sync: `GeoGemCsvSync` (`GeoTrinityEditor/Gem`) keeps catalog gems and the CSV equal, both ways; tests in `GeoTrinityEditor/Private/Tests/Gameplay/GeoGemTests.cpp`.

## 3D gems
One cut-stone solid per tier, richer and bigger as the tier rises; at rest, seen from above (orthographic camera), it shows its menu glyph's number of corners in the menus (the drop in the world stops in whatever pose its tumble reaches).
- One static mesh per tier, `/Game/Art/Meshes/Gem/SM_Gem_<Tier>`, built by `AI/Python/Gem/generate_gem_meshes.py`: Chip octahedron (a corner up: a square from above), Cut dodecahedron (a pentagon face up), Prism icosahedron (a face up: a hexagon outline); the rarer the tier, the more complex the solid, Core diamond (low-poly brilliant: octagon girdle at the glyph's 22.5° turn, table on top, pavilion to a point).
- Size = tier: circumradius grows Chip < Cut < Prism < Core (`RADIUS` in the script, ~×1.25 a step). Mesh centred on its bounding box. Flat faces, no bevel: the solid and its size tell the tier.
- One material slot `Gem`, each tier wears its own instance `MI_Gem_<Tier>` of the master `M_Gem` (`/Game/Art/Mat/`): glowing faceted stone, emissive above 1 so bloom lights it, rim brighter at grazing angles, flat faces catch the light as facets (self-lit, "Drop feel"), `Flash` scalar (set per frame by the drop) boosting the glow. The stat colour is the `Color` parameter, set on every slot's dynamic instance per drop, same stat colours as `UGeoGemGlyph`. A Core glows hardest (its glyph's bright centre); a Chip, the smallest, glows above Cut to stay visible.
- Halo: second slot `Halo`, a smooth sphere around the stone (1.7× its radius) wearing `M_GemHalo`: unlit additive, the stat `Color` fading from the centre to nothing at the outline. A sphere, so the tumble never shows on it. Gems only; shards have none.
- `AGeoGemDrop` (`Actor/`) is one gem lying in the world: mesh by tier, colour by stat, bob once landed. Visual only: the gems are already in the profile when the boss dies (`GrantGemRewards`, `Boss loot`), the drop never grants anything.
- One drop per gem of the kill loot (a count of 3 = 3 drops), the same gem ids the players were granted. Shards drop too ("Shards" below), in the same fountain.

### Drop feel
A drop is a solid in 3D, never a sprite turned to the camera; every motion is a pure function of the replicated launch data and `FGeoGemDropSpec::Seed`, so every machine plays the same thing.
- Lit by the material itself (`M_Gem`): a fixed key light direction against the pixel normal, so every face shades by its turn whatever the level's lights are, plus a specular glint. A turning gem flashes facet by facet.
- Flight: full 3D tumble around a random axis, starting fast (`TumbleSpeed` range, axis and speed from the seed). Spawn pops the mesh in from zero with an overshoot (`PopTime`).
- Rotation never jumps: one axis, one angular speed that `TumbleFriction` decays exponentially from the throw, so the gem lands turning and winds down to a stop in the pose it reached (angle = speed / friction × (1 − e^(−friction × t))). No resting pose it blends into.
- Walls: the horizontal path is simulated once at `BeginPlay` (`BuildPath`, fixed `PathStep`, deterministic on every machine): a sphere sweep (`CollisionRadius`, below `RestHeight` so the floor is never hit) at floor height against world-static geometry, reflecting the velocity off the wall (`WallRestitution`). Floor height at any altitude: a wall holds a gem in, however high it flies. Height stays the analytic arc.
- Ground friction: from the first touchdown the horizontal speed decays at `GroundFriction`, so the drop slides and stops (`RestSpeed`). The root moves until then.
- Landing: the mesh bounces `BounceCount` times from the first touchdown, each at `BounceRestitution` of the last impact speed, vertical only (even while it slides). Each touchdown sets the material scalar `Flash` to a spike (smaller each bounce) that decays at `FlashDecay`: the gem lights up on every hit. Bob fades in once the bounces are over.
- Pickup: collision turns on `PickupDelay` (0.5 s) after the throw, flying or sliding: a player standing where it comes down takes it without moving. The root stays still once the slide ends (`TravelTime`).

### Fountain feel
- Front-loaded: the first drop leaves at once, then the gaps grow (drop `k` of `N` at `FountainDuration * (k / (N - 1)) ^ FountainFrontLoad`, exponent above 1 = burst first, trickle after). A kill reads as an eruption, not a drip.
- Each throw's upward speed varies by `FountainUpSpread` (fraction), so the gems peak at different heights.

### Drop sounds
Two sounds per drop, `FGeoSoundEntry` on `AGeoGemDrop` (`SpawnSound`, `PickupSound`, set on `BP_GemDrop`; gems and shards share them). Kit `Gem` in `/Game/Art/SFX/Gem/` (`AI/SoundCreation.md`, "Reusable sounds"): crystal, glassy, chime-bright; no metal.
- Spawn: on every machine the moment the drop begins play at its throw location (the fountain throws them one by one over `FountainDuration`, so one sound each, a pile never repeats: one asset, pitch rolled in `DropPitchRange` by both sounds, set in the constructor).
- Pickup: the server destroys the drop, each machine plays the sound where the drop was when its actor is destroyed (`EndPlay` reason `Destroyed`, past `PickupDelay`: a level change, PIE end, streaming-out or a destroy before it could be picked is silent). The sound lives in its own audio component, fire-and-forget at the world position, never attached to the drop: the actor is gone at once, the sound rings out its full length (`UGeoSoundRowLibrary::PlaySoundEntryAtLocation`, `AbilitySystem/Data/FX/CLAUDE.md`). A tail may be as long as the asset wants; never cut on destroy.
- Positional, not 2D: a drop picked up across the arena is quieter. Audience stays `All`; the drop itself is the sound instigator and no player's, so `ShouldPlay` treats it as `InstigatorMachine`: full volume on every machine; no per-picker sound, the pickup is not replicated.
- Dedicated server plays nothing (`ShouldPlay`).

## Shards
The Forge currency, drawn as a raw crystal fragment: irregular, angular, faceted, like a broken piece of a gem (never a regular solid, that is a gem's look).
- 3D: `/Game/Art/Meshes/Gem/SM_Shard_<A|B|C>`, three hand-listed low-poly convex shapes (a tall spike, a tilted block, a squat chunk), smaller than a Chip (`SHARD_SCALE` in the script: one shard is one Forge unit, a pile is many), flat faces, one slot `Gem`, built by `generate_gem_meshes.py` with the gems. Material `MI_Shard` (instance of `M_Gem`), one fixed pink colour set on the instance by `make_gem_materials.py`, no stat (the drop leaves the material alone). Every shard drop picks a variant at random and a random turn, so a pile never repeats.
- One drop per shard: the fountain throws exactly the kill's shard count, so the pile on the floor is what the profile got; same fountain, landing and pickup rules as a gem drop (`FGeoGemDropSpec::Kind` = Gem / Shard).
- 3D only: shards have no 2D look; the menus keep writing the shard count as text.
- Fountain: on the kill the server spawns the drops one by one over `FountainDuration` (about 5 s) at the boss's death position; each is thrown up with a random sideways spread, falls under gravity and lands on the arena floor around the boss. Replicated actor (gem id, launch velocity, start time): every client replays the same arc, no movement replication.
- Stays forever once landed, until a player character touches it: the server destroys it on overlap of any player character (the same drop for everyone, the first to touch it removes it). No effect on the character. Pickup collision is server-only, on from `PickupDelay` (`EnablePickup`, a timer), and overlaps `GeoCharacter` only; until the slide ends the server moves the root along the throw (a dedicated server ticks a drop only until `Land`), so the sphere is where the gem is. Once its slide ends the actor stays still and only its mesh bobs: a lying drop costs no overlap update.

## Boss loot
`FGeoBossLoot`, one per boss on its `AGeoArena` (`Actor/Arena/CLAUDE.md`). Kill loot = random part + fixed part, added together; built by `UGeoGemCatalog::MakeKillLoot`.
- Random part: `bOverrideDropTable` + `DropTable` (count range, tier chances) for this boss; off = the catalog's `DropTables[BossType]`. Scaled by difficulty (`DifficultyLootScale`).
- Fixed part: `FixedGems` (gem + count, every kill) and `FixedShards`. Never scaled. A tutorial kill = count 0 + its fixed list; a themed boss = its roll + guaranteed gems.
- A failed attempt gets neither part (XP only).
- Fixed gems are picked from a dropdown of the catalog's ids (`GetOptions` = `UGeoGemCatalog::GetGemIdOptions`); the stored key is still `Id`, so a renamed gem leaves the entry invalid, flagged by the arena's `IsDataValid`.
- Shards ride in `FGeoGemReward::Shards` → `UGeoGemProfileSave::AddShards`.
- The design doc sync (`gem-csv-to-doc`) covers the catalog defaults only; per-boss loot lives in the levels.

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
