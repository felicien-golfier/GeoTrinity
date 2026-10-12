# AbilitySystem

All GAS: abilities, attributes, effect data, exec calcs, library, tags, effect context.

| Folder | Holds |
|---|---|
| `Abilities/` | Every gameplay ability + enemy patterns → `Abilities/CLAUDE.md` |
| `AttributeSet/` | Base / player / gem attribute sets |
| `Components/` | `UGeoAbilitySystemComponent` |
| `Data/` | `AbilityInfo` catalog, `EffectData`, target-data RPC struct; `FX/` = cue/sound/VFX param structs |
| `ExecCalc/` | Damage and heal math |
| `Globals/` | `GeoAbilitySystemGlobals` — allocates `FGeoGameplayEffectContext`, fills cue params |
| `Lib/` | `GeoAbilitySystemLibrary` helpers, `GeoGameplayTags` native tags |
| `Types/` | `FGeoGameplayEffectContext` |

## Rules
- Apply effects only through `UGeoAbilitySystemLibrary::ApplyEffectFromEffectData()`.
- Find agents with `GeoASLib::GetInteractableActors*`; never physics overlaps.
- Tags: `FNativeGameplayTag` constants from `GeoGameplayTags`, never string literals.
- Gems: `Abilities/Gem/`, `AttributeSet/GeoGemAttributeSet`, `Gem/` module folder.
