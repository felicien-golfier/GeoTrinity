# AbilitySystem/Data

Data assets and structs that configure abilities and effects.

---

## `EffectData.h` — polymorphic effect system

`FEffectData` base — two virtual methods:
- `UpdateContextHandle(ContextHandle, AbilityLevel, AbilityTag)` — pre-apply hook; sets values on `FGeoGameplayEffectContext`. `AbilityTag` identifies the originating ability so subclasses (e.g. `FDamageEffectData`) can flag context fields like `bIsFromBasicAbility` based on the ability's owned tags.
- `ApplyEffect(SourceASC, TargetASC, ContextHandle)` — applies a `UGameplayEffect`

**Subtypes:**

| Type | What it does |
|---|---|
| `FDamageEffectData` | Flat damage. `bSuppressGameplayCue` suppresses hit flash unconditionally. `bLimitGameplayCue` rate-limits the cue via the target's `UGeoGameFeelComponent` (use on tick-based effects). `bSuppressCombatStats` skips the DPS meter |
| `FHealEffectData` | Flat heal. `bSuppressHealProvided` suppresses `OnHealProvided` delegate. `bSuppressGameplayCue` suppresses heal visual unconditionally. `bLimitGameplayCue` rate-limits the cue via the target's `UGeoGameFeelComponent`. `bSuppressCombatStats` skips the HPS meter |
| `FShieldEffectData` | Flat shield amount |
| `FGameplayEffectData` | Applies arbitrary `UGameplayEffect` with SetByCaller magnitude/duration. Optional `Icon` — shown in the HUD status bar while active; baked into a duplicated context on this spec only, so siblings in the same apply call are unaffected |
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

**Description ↔ file sync (editor-only).** The plain-text `Content/Data/AbilityDescriptions.txt` stays the source of truth at runtime and on load: `PostLoad` pulls each entry's `[AbilityTag]` section into its `Description` field so the asset shows the file text. Editing any `Description` in the asset pushes **all** entries' descriptions back to the file (`PostEditChangeProperty` → `WriteAllDescriptionsToFile`; `WriteDescriptionToFile` rewrites one section at a time, preserving every other section and the header comments). The edit path never reads from disk, so a just-typed value is never clobbered — that was the previous bug. To pull external `.txt` edits back into the asset, use the **Reload Descriptions From Disc** button (`ReloadDescriptionsFromDisc`, `CallInEditor`, next to `Populate Ability Tags`), which overwrites in-asset descriptions with the file's. Writes use `SaveStringToFile(..., EEncodingOptions::ForceUTF8)` so the file keeps a stable UTF-8 encoding instead of `AutoDetect` flipping it to ANSI. All of this is `#if WITH_EDITOR`; runtime display is unchanged and still reads the file live.

`FGameplayAbilityInfo::GetResolvedDescription()` — returns the description text with `{Token}`s replaced by live values from the ability CDO (used by `UGeoAbilityDescriptionsWidget`). The text comes from the `[AbilityTag]` section of the plain-text file `Content/Data/AbilityDescriptions.txt` when present (re-read every call — live-editable; staged into packages via `DirectoriesToAlwaysStageAsUFS`), falling back to the `Description` field otherwise. Tokens: `{Cooldown}`, `{FireDelay}`, `{Damage}`/`{Heal}`/`{Shield}` (summed from effect data), `{Effects}` (one line per effect entry: damage/heal/shield, buffs with magnitude + duration named by their `DataTag` leaf, statuses with chance, lethal), or any numeric / `FScalableFloat` property name on the ability class. Scalable values render at the current ability level by default; a `{Token:range}` suffix instead renders them as a `min-max` range over curve levels 1–10 (for values driven by another system than ability level, e.g. the reload's remaining-ammo scale — `{Effects:range}`), collapsing to a single number when flat. A scalar token may also take a `:%` (value×100) or `:+%` ((value−1)×100) suffix to render as a percentage / bonus percentage, so a `1.5` multiplier shows as `150%` or `50%` instead of a bare number (`{EnemyBounceMultiplier:+%}`, `{MinDamageMultiplier:%}`); suffixes combine in any order. A single `TInstancedStruct<FEffectData>` property holding an `FGameplayEffectData` resolves to its `Magnitude` (`{SpeedBuffEffect:%}`), and `{A*B}` renders the product of two numeric/scalable properties for a derived cap (`{DamageAndHealBoostPerAbsorbedZone*MaximumZoneAbsorbed:%}`). Unresolved tokens are kept as-is and logged.

`GetAbilityClassForTag(FGameplayTag)` — O(1) after the first call (lazily builds a cached tag→class map). Returns nullptr on miss with a warning log; safe to call on every effect application including for non-ability sources that pass an invalid tag.

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

## `GeoSoundRow.h` — the one sound system

`FGeoSoundEntry` — **the single sound-definition struct for the whole project** (projectile SoundMap, DataTable rows, one-off entries). Fields: `Sound`, `Volume`, `StartTime`, `Audience` (bitmask of `EGeoSoundAudienceBitflag`: `InstigatorMachine` / `OtherMachines`; default both), `OtherMachinesVolumeMultiplier` (default 0.5 — other players' sounds are half volume on your machine), attribute-driven pitch (`PitchAttribute` + `PitchCurve`), `RandomPitchMultiplierRange`. A `CoreRedirects` struct redirect maps the old `FProjectileSoundEntry` name.

`UGeoSoundRowLibrary` — the single playback path; never call `PlaySoundAtLocation`/`PlaySound2D` directly for gameplay sounds:
- `ShouldPlay(WorldContext, Entry, SoundInstigator)` — false on dedicated server, no Sound asset, or when the Audience mask excludes this machine (instigator == local player's avatar → `InstigatorMachine` bit, else `OtherMachines`; null instigator always plays)
- `GetVolume(Entry, SoundInstigator)` — applies `OtherMachinesVolumeMultiplier` when the instigator is not the local player's avatar
- `GetPitch(Entry, SoundInstigator)` — instigator-ASC attribute sampled against `PitchCurve`, × random range
- `PlaySoundEntry2D(WorldContext, Entry, SoundInstigator)` — `BlueprintCallable`; combines the three above
- `FindSoundForTag(SoundTable, Tag, bFound)` — `BlueprintPure`; first row whose `Tag` matches exactly (`MatchesTagExact`)

`FGeoSoundRow : FTableRowBase` — `Tag` (`FGameplayTag`, explicit field, **not** the row name) + `Entry` (`FGeoSoundEntry`). Row type of `DT_GenericSound`.

## `GeoGenericSoundCueNotify.h`
`UGeoGenericSoundCueNotify : UGameplayCueNotify_Burst` — C++ handler for the generic-sound GameplayCue (`GenericGameplayCueSoundTag` in `UGameDataSettings`). `OnExecute`: for each tag in the cue's `AggregatedSourceTags`, `FindSoundForTag(SoundTable, tag)` → `PlaySoundEntry2D` with `Parameters.Instigator`. The `GenericSound` BP in `/Game/AbilitySystem/GameplayCues/` is reparented to it with no graph logic; to add a new one-off sound, add a row to `DT_GenericSound` and fire the cue with the sound tag in `AggregatedSourceTags` (set `CueParameters.Instigator`).
