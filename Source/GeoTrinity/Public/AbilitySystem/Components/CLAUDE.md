# AbilitySystem/Components

## `GeoAbilitySystemComponent.h` — custom ASC used by all characters

### Input-driven activation
- `AbilityInputTagPressed(Tag)` — activates matching ability
- `AbilityInputTagHeld(Tag)` — every frame while held; drives hold-to-fire
- `AbilityInputTagReleased(Tag)` — ends hold abilities

### Passive abilities
`OnAvatarSet` was removed from `UGeoGameplayAbility`; `APlayableCharacter::GiveLife()` calls `ReactivatePassiveAbilities()` after every ability grant and every revive — activates granted Passive-tagged abilities not already running (no-op if all already active).

### Fire helpers
- `GetFireSectionIndex(AbilityTag)` — reference to section counter for animation cycling
- `SetLastBasicAbilityTarget`/`GetLastBasicAbilityTarget` — server-only weak pointer, set by `ExecCalc_Damage` when `bIsFromBasicAbility` is flagged; read by `AGeoTurret::FindBestTarget()` to prefer the owner's last target
- Fire origin/yaw logic lives on `UGeoGameplayAbility::GetFireOrigin2D`/`GetFireYaw` — override there, not here

### Pattern management (enemy-only)
- `CreatePatternInstance()`, `FindPatternByClass(Class)`, `StopAllActivePatterns()`
- `PatternStartMulticast(Payload, PatternClass, PatternData)` — clients instantiate the pattern and call `InitPattern`; `PatternData` optional

### Startup
`GiveStartupAbilities()` — grants class + shared abilities from global `UAbilityInfo`, filtered by `EPlayerClass`.

### Delegates (for passive bindings)
`OnHealProvided(float)` (from `ExecCalc_Heal`), `OnDamageDealt(float, Tag)` (from `ExecCalc_Damage`), `OnHealthChanged(float)`, `OnMaxHealthChanged(float)` (from `GeoAttributeSetBase`).

Gate all bindings at the binding site (server/local-player checks before `AddDynamic`, not inside the handler).
