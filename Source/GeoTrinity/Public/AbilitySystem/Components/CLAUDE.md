# AbilitySystem/Components

## `GeoAbilitySystemComponent.h` — custom ASC

Extended ASC used by all characters.

### Input-driven activation
- `AbilityInputTagPressed(FGameplayTag)` — activates matching ability
- `AbilityInputTagHeld(FGameplayTag)` — called every frame while held; drives hold-to-fire
- `AbilityInputTagReleased(FGameplayTag)` — ends hold abilities

### Fire helpers (used by ability classes)
- `GetFireSectionIndex(AbilityTag)` — returns reference to section counter for animation cycling
- Fire origin and yaw logic lives on `UGeoGameplayAbility::GetFireOrigin2D` / `GetFireYaw` — override there to customize per-ability

### Pattern management (enemy-only)
- `CreatePatternInstance()` — instantiates a `UPattern` subclass
- `FindPatternByClass(Class)` — returns active pattern or nullptr
- `StopAllActivePatterns()` — ends all running patterns
- `PatternStartMulticast()` — multicast RPC; clients instantiate the pattern here

### Startup
- `GiveStartupAbilities()` — grants class abilities + shared abilities from global `UAbilityInfo` data asset, filtered by `EPlayerClass`

### Delegates (for passive ability bindings)
- `OnHealProvided(float)` — broadcast by `ExecCalc_Heal` on each heal (unless suppressed)
- `OnDamageDealt(float, FGameplayTag)` — broadcast by `ExecCalc_Damage`
- `OnHealthChanged(float)` — broadcast from `GeoAttributeSetBase`
- `OnMaxHealthChanged(float)` — broadcast from `GeoAttributeSetBase`

**Gate all bindings at the binding site**: if a delegate should only fire on server or local player, guard before `AddDynamic`, not inside the handler.
