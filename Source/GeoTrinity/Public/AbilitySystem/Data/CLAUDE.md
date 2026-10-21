# AbilitySystem/Data

Data that configures abilities and effects. Presentation structs (cue/sound/VFX) are in `FX/`.

| File | Role |
|---|---|
| `EffectData.h` | `FEffectData` polymorphic effects + `UEffectDataAsset` shared container |
| `AbilityInfo.h` | `UAbilityInfo` global ability catalog, descriptions, stats |
| `GeoAbilityTargetTypes.h` | `FGeoAbilityTargetData`: `Origin`, `Yaw`, `ServerSpawnTime`, `Seed`; client→server via `ServerSetReplicatedTargetData` |

## EffectData
- Base hooks: `UpdateContextHandle(Handle, Level, AbilityTag)` (pre-apply) and `ApplyEffect(SourceASC, TargetASC, Handle)`.
- Types: `FDamageEffectData`, `FHealEffectData`, `FShieldEffectData`, `FGameplayEffectData` (arbitrary GE with SetByCaller, optional status `Icon`), `FSingleUseDamageMultiplierEffectData`, `FStatusEffectData` (seeded roll), `FLethalEffectData`.
- Damage/heal flags: `bSuppressGameplayCue`, `bLimitGameplayCue`, `bSuppressCombatStats`, `bSuppressHealProvided`.
- Shared set = `UEffectDataAsset`; ability-specific = inline `TInstancedStruct<FEffectData>`.

## AbilityInfo
- Lists: `TriangleAbilities` / `CircleAbilities` / `SquareAbilities`, `SharedAbilities`, `EnemyAbilityInfos`.
- Entry `FPlayersGameplayAbilityInfo`: `AbilityClass`, `AbilityTag`, `InputAction`, `InputTag`, `bGiveAtStartup`, `AbilityIcon`.
- `GetAbilitiesForClass(EPlayerClass)` = class + shared; `GetAbilityClassForTag` cached O(1).
- Several abilities may share one `AbilityTag`, differing by `EPlayerClass`.
- `populate_ability_tags()` (editor button) re-reads CDO `AssetTags`.
- Descriptions: `Data/AbilityDescriptions.txt` is the runtime source (`PostLoad` fills `Description`; edits push all back; UTF-8). Token syntax → `AI/AbilityDescriptions.md`.
- `GetResolvedDescription()` / `GetResolvedStats()` resolve `{Token}`s against live CDO values.
