# AbilitySystem/Data

Data assets and structs that configure abilities and effects.

## `EffectData.h` — polymorphic effect system
`FEffectData` base — `UpdateContextHandle(ContextHandle, Level, AbilityTag)` (pre-apply hook; `AbilityTag` lets subclasses flag context fields, e.g. `bIsFromBasicAbility`) and `ApplyEffect(SourceASC, TargetASC, ContextHandle)`.

| Type | What it does |
|---|---|
| `FDamageEffectData` | Flat damage. `bSuppressGameplayCue`/`bLimitGameplayCue` (rate-limit via `UGeoGameFeelComponent`)/`bSuppressCombatStats` |
| `FHealEffectData` | Flat heal. `bSuppressHealProvided`, plus same cue-suppress/limit/stats flags |
| `FShieldEffectData` | Flat shield amount |
| `FGameplayEffectData` | Arbitrary `UGameplayEffect` with SetByCaller magnitude/duration. Optional `Icon` shown in HUD status bar, baked into a duplicated context per-spec |
| `FSingleUseDamageMultiplierEffectData` | Sets `SingleUseDamageMultiplier` on context; read only by `ExecCalc_Damage`, never by `FDamageEffectData::ApplyEffect` |
| `FStatusEffectData` | Probabilistic status (0-100%), seeded roll from context for determinism |
| `FLethalEffectData` | Instant kill via `GameDataSettings.LethalEffect`, no params |

`UEffectDataAsset` — container for shared/reused `TInstancedStruct<FEffectData>` arrays. Inline on ability for ability-specific effects; asset reference for shared ones.

## `AbilityInfo.h` — global ability catalog
`UAbilityInfo`: `TriangleAbilities`/`CircleAbilities`/`SquareAbilities` (class-specific), `SharedAbilities` (all players), `EnemyAbilityInfos` (non-player, no InputAction/class filter). Entry: `FPlayersGameplayAbilityInfo` (`AbilityClass`, `AbilityTag`, `InputAction`, `InputTag`, `bGiveAtStartup`, `AbilityIcon`).

`populate_ability_tags()` (editor `CallInEditor`) re-reads CDO `AssetTags` into `AbilityTag` fields. `GetAbilitiesForClass(EPlayerClass)` returns class + shared abilities.

**Description ↔ file sync (editor-only).** `Content/Data/AbilityDescriptions.txt` is the runtime source of truth: `PostLoad` pulls each `[AbilityTag]` section into `Description`. Editing any `Description` in-editor pushes **all** entries back to the file (never reads from disk on edit, so a just-typed value can't be clobbered). **Reload Descriptions From Disc** button pulls external `.txt` edits back into the asset. Writes force UTF-8 to avoid `AutoDetect` flipping to ANSI.

`FGameplayAbilityInfo::GetResolvedDescription()` — replaces `{Token}`s with live CDO values (re-reads the `.txt` live). Tokens: `{Cooldown}`, `{FireDelay}`, `{Damage}`/`{Heal}`/`{Shield}`, `{Effects}`, or any numeric/`FScalableFloat` property name. `{Token:range}` renders `min-max` over levels 1-10 instead of current-level value. `:%`/`:+%` suffixes render as percentage/bonus-percentage (combinable). `{A*B}` renders the product of two properties. Unresolved tokens are kept as-is and logged.

`GetAbilityClassForTag(Tag)` — O(1) after first call (cached tag→class map); nullptr + warning on miss.

Multiple abilities can share one `AbilityTag` differing only by `EPlayerClass` — use this instead of separate classes.

## `GeoAbilityTargetTypes.h` — per-shot RPC payload
`FGeoAbilityTargetData` extends `FGameplayAbilityTargetData`: `Origin`, `Yaw`, `ServerSpawnTime`, `Seed`, custom `NetSerialize`. Sent client→server via `ServerSetReplicatedTargetData`.

## `StatusInfo.h`
Status effect configuration data (types, durations, visuals); referenced by `FStatusEffectData`.

## `GeoSoundRow.h` — the one sound system
`FGeoSoundEntry` — single sound-definition struct project-wide. Fields: `Sound`, `Volume`, `StartTime`, `Audience` (bitmask `InstigatorMachine`/`OtherMachines`, default both), `OtherMachinesVolumeMultiplier` (default 0.5), attribute-driven pitch (`PitchAttribute`+`PitchCurve`), `RandomPitchMultiplierRange`.

`UGeoSoundRowLibrary` — the single playback path; never call `PlaySoundAtLocation`/`PlaySound2D` directly for gameplay sounds. `ShouldPlay`/`GetVolume`/`GetPitch`/`PlaySoundEntry2D` (combines all three)/`FindSoundForTag`.

`FGeoSoundRow : FTableRowBase` — `Tag` (explicit field, not row name) + `Entry`. Row type of `DT_GenericSound`.

## `GeoGenericSoundCueNotify.h`
`UGeoGenericSoundCueNotify : UGameplayCueNotify_Burst` — handles the generic-sound GameplayCue (`GenericGameplayCueSoundTag`). For each tag in `AggregatedSourceTags`, looks up `DT_GenericSound` and plays it. To add a one-off sound: add a `DT_GenericSound` row and fire the cue with the sound tag in `AggregatedSourceTags`.
