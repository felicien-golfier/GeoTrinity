# HUD/Combatant

| File | Role |
|---|---|
| `GenericCombattantWidget.h` | Health / shield bar for any combatant (enemy, boss, deployable); not the player overlay |
| `GeoBossHealthBarWidget.h` | Boss bar + fight timer (arena's elapsed time on the local clock); exists exactly while a boss fight does |
| `GeoDamageNumberWidget.h` | Pooled floating number: jitter, upward drift, re-projected each tick, fade; return to pool by world timer |
| `GeoCombattantWidgetComp.h` | WidgetComponent implementing `IGeoCombattantWidgetHost`; binds the owner's ASC |

- `GenericCombattantWidget`: `ShieldBar` overlays `HealthBar`; `InitializeWithAbilitySystemComponent` binds attributes and registers for damage numbers.
- Widget component is created in C++ on `AGeoCharacter` / `AGeoDeployableBase` from the `GameDataSettings` soft class; `BindToOwnerASC()` idempotent, call again once the ASC exists; per-BP tuning lives on the owner.
- Damage numbers: `AGeoHUD::DamageNumberPool`; an actor opts out through `IGeoDamageNumberHost`.
