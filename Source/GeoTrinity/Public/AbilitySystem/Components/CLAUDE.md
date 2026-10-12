# AbilitySystem/Components

`GeoAbilitySystemComponent.h` — the one ASC for every character.

- **Input**: `AbilityInputTagPressed` / `Held` (every frame) / `Released` activate and end abilities by `InputTag`.
- **Passives**: `ReactivatePassiveAbilities()` called by `APlayableCharacter::GiveLife()` after every grant and revive.
- **Startup**: `GiveStartupAbilities()` from `UAbilityInfo`, filtered by `EPlayerClass`.
- **Fire helpers**: `GetFireSectionIndex(AbilityTag)` montage counter; `Set/GetLastBasicAbilityTarget` (server weak ptr, read by `AGeoTurret::FindBestTarget`).
- **Patterns** (enemy): `CreatePatternInstance`, `FindPatternByClass`, `StopAllActivePatterns`, `PatternStartMulticast(Payload, Class, Data)`.
- **Delegates for passives**: `OnHealProvided`, `OnDamageDealt(float, Tag)`, `OnHealthChanged`, `OnMaxHealthChanged`.
- **Delegates raised by shared abilities** (their code never names the listener): `OnDashAiming` (Start, bHasDirection, Direction&, Distance&; server + predicting client), `OnDashStarted` (server), `OnDeployableLanded` (a throw landed, server), `OnDeployableDestroyedByDamage` (hit-killed deployable ends its blink, server).
- `HandleOwnerDeath(KeptEffects)`: the death teardown. Every active instance on this machine gets `OnOwnerDeath()` (each ability decides: default ends cancelled, Core passives stay); `ResetCooldowns()`; server only: every active GE removed except the `KeptEffects` handles (`UGeoGemComponent::GetSurvivingEffectHandles()`). Replaces `CancelAllAbilities` + unfiltered purge on the death path; revive and class change keep the full reset (`APlayableCharacter::ResetAbilitiesAndEffects`).
- Bind delegates after gating on server/local player at the binding site.
- Cooldown reset only through `ResetCooldowns()`: the server removes the `Ability.Cooldown`-granting effects, every machine clears its leftover cooldown tag counts. Call it from a path driven by replicated state (`APlayableCharacter::ResetAbilitiesAndEffects()`, `AGeoPlayerState::OnRep_PlayerClass()`). Never an unfiltered `RemoveActiveEffects(FGameplayEffectQuery())` on a player ASC (`TroobleshootingLogs.md`, "Predicted GE removal").
