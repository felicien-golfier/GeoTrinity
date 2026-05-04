# Coding Style & Conventions

## Formatting
- 4-space indentation with tabs, 120 column limit, Allman brace style (`.clang-format` enforced)
- UE macros handled: UPROPERTY, UCLASS, USTRUCT, GENERATED_BODY, UENUM

## General Rules
- Prefer fewer but longer `if` statements — merge conditions with `&&` rather than nesting
- `const` by default; remove only when mutation is needed
- Prefer non-const parameters over creating new variables (`FTransform SpawnTransform` not `const FTransform& SpawnTransform` + local copy)
- Prefer forward declarations in headers over includes (enums: `enum class EMyEnum : uint8;`)
- Remove trivial wrapper functions that just delegate — call directly instead
- No unused parameters, variables, or speculative features (YAGNI)
- **Delete unused code**: remove declaration + implementation when removing a call site. No dead code safety nets.
- **No duplicated code**: extract to base class, component, or free function when logic appears in more than one place
- Be consistent: same code style, same naming convention throughout
- `Super` call placement: choose what makes semantic sense (Init = top, Destroy = bottom); when no ordering dependency exists, always top
- No abbreviations in variable names; full class names except `ASC` for AbilitySystemComponent
- No comments that restate what the code says — code should be self-documenting

## Class Hierarchy Rules
- **Read the full class hierarchy** before adding any new function or data member
- **Verify access specifiers**: public = callable from outside, private = internal, protected = subclass extension
- **Reuse existing virtual methods**: check if base class already has a virtual for the same concept before adding new methods
- **Extracting to base class**: copy code exactly — never modify logic during the move. Introduce virtual getters for subclass variation.

## Error Handling — No Silent Fallbacks
- Never silently skip or substitute when something required is missing. Always surface the error.
- Ask: *can this condition legitimately be false at runtime?* If no → must flag it.
- `ensureMsgf(condition, TEXT("..."))` — flags the problem
  - If execution must continue: `if (!x) { ensureMsgf(x, TEXT("...")); return; }` — ensure *inside* the if body
  - If no return needed: `ensureMsgf(x, TEXT("..."))` alone
- `checkf` — for critical invariants (wrong actor type, missing subsystem, corrupted state) where silent continuation causes hard-to-debug damage
- Never use `condition ? A : B` as a quiet fallback
- Configured assets (ProjectileClass, EffectData, curves, subsystems) → `ensureMsgf` (configuration bug)
- Wrong actor/component types → `ensureMsgf` (design bug)
- Runtime misses (no target found, empty list, optional feature) → plain `if`, no assert

## Binding Rules
- Gate condition-based bindings at the binding site, not inside the callback
- Server-only or local-player-only bindings: guard before `AddDynamic`, not inside the handler

---

## GAS Conventions (Rules Violated by AI in the Past)

**Base class**: All ability classes must extend `UGeoGameplayAbility`. Never use `UGameplayAbility` directly.
- Include: `"AbilitySystem/Abilities/Base/GeoGameplayAbility.h"`

**Server check**: Always use `UGameplayLibrary::IsServer(GetWorld())`.
- Never `HasAuthority()` (true for autonomous proxies on clients)
- Never `IsNetMode(NM_DedicatedServer)` (misses listen server)

**`OnFireTargetDataReceived` is server-only by design**: Never add `IsServer` guards inside it.

**Never override `Fire()` as a no-op**: It sends `FGeoAbilityTargetData` to the server, triggering `OnFireTargetDataReceived`. Overriding with empty body silently breaks the server chain. If subclass only needs server-side logic, override `OnFireTargetDataReceived` only.

**RNG in abilities**: Always seed from `StoredPayload.Seed` — `FRandomStream Stream(StoredPayload.Seed)`. Never call `FMath::Rand*` directly. Both client and server must derive identical values from the same seed.

**`StoredPayload.Owner` naming**: Local variable copies must be named `PayloadOwner`, never `AvatarActor`.

**New projectiles default to `AGeoPooledProjectile`**: Extend `AGeoPooledProjectile` (not `AGeoProjectile` directly) unless explicitly stated otherwise.

**VFX — never use multicast RPC or multicast delegate**: Clients have enough local context. Trigger VFX via Gameplay Cues. If a deployable needs to stay alive visually after logic ends, keep actor alive but logic-dead (disable overlap/tick).

**No movement/physics workarounds**: Never use `SetMovementMode(MOVE_Flying)` or similar as a side-effect fix. Find the actual root cause.

---

## Debugging Rules

**NO WORKAROUNDS**: Find the actual root cause. Don't propose workarounds until the real problem is understood.

**Check Epic source before implementing UE features**: Always read actual engine/plugin headers before implementing StateTree tasks, GAS, AI, etc.
- Key references: `FStateTreeDelayTask`, `FStateTreeMoveToTask`
- Plugin source: `Engine\Plugins\Runtime\<PluginName>\Source\<Module>\`
- Never assume method names — they change between versions.
