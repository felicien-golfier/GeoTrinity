# AbilitySystem

All GAS infrastructure: abilities, attributes, execution calculations, effect data, library, tags, and custom context.

## Subfolders
| Folder | What's inside |
|---|---|
| `Abilities/` | All gameplay ability classes — see `Abilities/CLAUDE.md` |
| `AttributeSet/` | Attribute definitions: `GeoAttributeSetBase` (all chars), `CharacterAttributeSet` (players) |
| `Components/` | `UGeoAbilitySystemComponent` — custom ASC |
| `Data/` | `AbilityInfo` catalog, `EffectData` polymorphic effects, `GeoAbilityTargetTypes`, `StatusInfo` |
| `ExecCalc/` | Damage and heal execution calculations |
| `Globals/` | `GeoAbilitySystemGlobals` — allocates custom effect context |
| `Lib/` | `GeoAbilitySystemLibrary` (helpers), `GeoGameplayTags` (native tags) |
| `Types/` | `GeoAscTypes` — `FGeoGameplayEffectContext` |

## Key Rules
**Always use `UGeoAbilitySystemLibrary::ApplyEffectFromEffectData()`** to apply effects — never apply GEs directly.
Two-pass loop: all `UpdateContextHandle()` first, then all `ApplyEffect()`. Order in the array doesn't matter for context setup.

**Never use `SphereOverlapActors` or `OverlapMultiByChannel` to find game agents** — use `GeoASLib::GetInteractableActors(...)` instead. It iterates only the ~20 relevant actors and skips the physics scene entirely.
