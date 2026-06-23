# AbilitySystem/Data

Data assets and structs that configure abilities and effects.

---

## `EffectData.h` — polymorphic effect system

`FEffectData` base — two virtual methods:
- `UpdateContextHandle(ContextHandle)` — pre-apply hook; sets values on `FGeoGameplayEffectContext`
- `ApplyEffect(SourceASC, TargetASC, ContextHandle)` — applies a `UGameplayEffect`

**Subtypes:**

| Type | What it does |
|---|---|
| `FDamageEffectData` | Flat damage. `bSuppressGameplayCue` suppresses hit flash unconditionally. `bLimitGameplayCue` rate-limits the cue via the target's `UGeoGameFeelComponent` (use on tick-based effects). `bSuppressCombatStats` skips the DPS meter |
| `FHealEffectData` | Flat heal. `bSuppressHealProvided` suppresses `OnHealProvided` delegate. `bSuppressGameplayCue` suppresses heal visual unconditionally. `bLimitGameplayCue` rate-limits the cue via the target's `UGeoGameFeelComponent`. `bSuppressCombatStats` skips the HPS meter |
| `FShieldEffectData` | Flat shield amount |
| `FGameplayEffectData` | Applies arbitrary `UGameplayEffect` with SetByCaller magnitude/duration |
| `FSingleUseDamageMultiplierEffectData` | Sets `SingleUseDamageMultiplier` on context in `UpdateContextHandle`. Read by `ExecCalc_Damage`. **Do NOT read in `FDamageEffectData::ApplyEffect`** |
| `FStatusEffectData` | Probabilistic status (StatusChance 0..100%). Uses seeded roll from context for determinism |
| `FLethalEffectData` | Instantly kills the target via `GameDataSettings.LethalEffect`. No parameters — BP sets health to 0 |

`UEffectDataAsset` — container for a reusable array of `TInstancedStruct<FEffectData>`. Create one only when effects are shared across multiple abilities.

**Container choice:**
- Inline on ability (`TArray<TInstancedStruct<FEffectData>>`) — for effects specific to one ability
- Asset reference (`TArray<TSoftObjectPtr<UEffectDataAsset>>`) — for shared/reused effects

---

## `AbilityInfo.h` — global ability catalog

`UAbilityInfo` data asset (configured once, referenced globally):
- `TriangleAbilities`, `CircleAbilities`, `SquareAbilities` — class-specific
- `SharedAbilities` — given to all players
- `EnemyAbilityInfos` — non-player abilities (enemies, passives, system); no InputAction, no class filter

`FPlayersGameplayAbilityInfo` per entry: `AbilityClass`, `AbilityTag`, `InputAction`, `InputTag`, `bGiveAtStartup`, `AbilityIcon`.

`populate_ability_tags()` (`BlueprintCallable`, `CallInEditor`) — re-reads all CDO `AssetTags` and fills `AbilityTag` fields on all entries. From Python, set `AbilityTag` directly in `import_text` — see `MCP_NewEnemyAbility.md`.

`GetAbilitiesForClass(EPlayerClass)` — returns class abilities + shared.

Multiple abilities can share the same `AbilityTag` but differ by `EPlayerClass` — use this instead of making separate ability classes per class.

---

## `GeoAbilityTargetTypes.h` — per-shot RPC payload

`FGeoAbilityTargetData` extends `FGameplayAbilityTargetData`:
- Fields: `Origin` (`FVector2D`), `Yaw`, `ServerSpawnTime`, `Seed`
- Custom `NetSerialize` — compact binary serialization for the server RPC

Sent client → server via `ServerSetReplicatedTargetData`.

---

## `StatusInfo.h`
Status effect configuration data (types, durations, visuals). Referenced by `FStatusEffectData`.

---

## `GeoSoundRow.h`
`FGeoSoundRow : FTableRowBase` — `Tag` (`FGameplayTag`) + `Sound` (`USoundBase`). DataTable row type for tag→sound lookup; the tag is an explicit field (picked from the tag tree), **not** the row name.
`UGeoSoundRowLibrary::FindSoundForTag(SoundTable, Tag)` — `BlueprintPure`, returns the `Sound` of the first row whose `Tag` matches exactly (`MatchesTagExact`), else nullptr. Used by the `GenericSound` GameplayCueNotify: `MatchedTagName` → `FindSoundForTag` → `Play Sound`.
