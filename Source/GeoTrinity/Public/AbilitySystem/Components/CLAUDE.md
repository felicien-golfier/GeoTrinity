# AbilitySystem/Components

`GeoAbilitySystemComponent.h` — the one ASC for every character.

- **Input**: `AbilityInputTagPressed` / `Held` (every frame) / `Released` activate and end abilities by `InputTag`.
- **Passives**: `ReactivatePassiveAbilities()` called by `APlayableCharacter::GiveLife()` after every grant and revive.
- **Startup**: `GiveStartupAbilities()` from `UAbilityInfo`, filtered by `EPlayerClass`.
- **Fire helpers**: `GetFireSectionIndex(AbilityTag)` montage counter; `Set/GetLastBasicAbilityTarget` (server weak ptr, read by `AGeoTurret::FindBestTarget`).
- **Patterns** (enemy): `CreatePatternInstance`, `FindPatternByClass`, `StopAllActivePatterns`, `PatternStartMulticast(Payload, Class, Data)`.
- **Delegates for passives**: `OnHealProvided`, `OnDamageDealt(float, Tag)`, `OnHealthChanged`, `OnMaxHealthChanged`.
- `EndActiveAbilitiesLocally()` tears down this machine's instances (death); drives off instance `IsActive()`.
- Bind delegates after gating on server/local player at the binding site.
- Cooldown reset only through `ResetCooldowns()`: the server removes the `Ability.Cooldown`-granting effects, every machine clears its leftover cooldown tag counts. Call it from a path driven by replicated state (`APlayableCharacter::ResetAbilitiesAndEffects()`, `AGeoPlayerState::OnRep_PlayerClass()`). Never an unfiltered `RemoveActiveEffects(FGameplayEffectQuery())` on a player ASC (`TroobleshootingLogs.md`, "Predicted GE removal").
