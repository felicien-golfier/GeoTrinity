# AbilitySystem

All GAS infrastructure: abilities, attributes, execution calculations, effect data, library, tags, custom context.

## Subfolders
| Folder | What's inside |
|---|---|
| `Abilities/` | All gameplay ability classes — see `Abilities/CLAUDE.md` |
| `AttributeSet/` | `GeoAttributeSetBase` (all chars), `CharacterAttributeSet` (players) |
| `Components/` | `UGeoAbilitySystemComponent` — custom ASC |
| `Data/` | `AbilityInfo` catalog, `EffectData` polymorphic effects, `GeoAbilityTargetTypes`, `StatusInfo` |
| `ExecCalc/` | Damage and heal execution calculations |
| `Globals/` | `GeoAbilitySystemGlobals` — allocates custom effect context |
| `Lib/` | `GeoAbilitySystemLibrary` (helpers), `GeoGameplayTags` (native tags) |
| `Types/` | `GeoAscTypes` — `FGeoGameplayEffectContext` |

## Key Rules
- Always apply effects via `UGeoAbilitySystemLibrary::ApplyEffectFromEffectData()` — never apply GEs directly. Two-pass: all `UpdateContextHandle()` first, then all `ApplyEffect()`.
- Never use `SphereOverlapActors`/`OverlapMultiByChannel` to find game agents — use `GeoASLib::GetInteractableActors(...)` (iterates ~20 relevant actors, skips the physics scene).
