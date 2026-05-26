# Coding Style & Conventions

## General Rules
- Prefer fewer but longer `if` statements — merge conditions with `&&` rather than nesting
- `const` by default; remove only when mutation is needed
- Prefer non-const parameters over creating new variables (`FTransform SpawnTransform` not `const FTransform& SpawnTransform` + local copy)
- Prefer forward declarations in headers over includes (enums: `enum class EMyEnum : uint8;`)
- Remove trivial wrapper functions that just delegate — call directly instead
- **YAGNI — code only what's needed.** No unused parameters, variables, or speculative features
- **Functions do exactly what their name says — nothing more.** A getter returns a value; it never mutates state. A query never triggers side effects. If a function needs to do more, split it.
- **Delete unused code**: remove declaration + implementation when removing a call site. No dead code safety nets.
- **No duplicated code**: extract to base class, component, or free function when logic appears in more than one place
- **Before adding or removing any field/function**: Grep for all read and write sites across the codebase. Never assume a field is unused or needs a new setter based on the header alone — it may already be populated elsewhere (e.g. a base class method).
- **Before adding a function**: check if an existing function already covers the same operation. If two functions differ only by a constant (e.g. a trigger type, a flag), merge them into one with a parameter. Never add a wrapper that just forwards with a hardcoded argument.
- Be consistent: same code style, same naming convention throughout
- `Super` call placement: choose what makes semantic sense (Init = top, Destroy = bottom); when no ordering dependency exists, always top
- No abbreviations in variable names; full class names except `ASC` for AbilitySystemComponent
- **Never use `GetComponentByClass` to get an ASC** — always use `GeoASLib::GetGeoAscFromActor` instead
- No comments that restate what the code says — code should be self-documenting
- Never assume method names or constants (including `FColor::X`) — they change between versions.
**Check Epic source before using any UE named constant or method**: Always read the actual engine header to confirm it exists. If the constant doesn't exist, define it with explicit RGB/RGBA values instead.
**Check Epic source before implementing UE features**: Always read actual engine/plugin headers before implementing StateTree tasks, GAS, AI, etc.
- Plugin source: `Engine\Plugins\Runtime\<PluginName>\Source\<Module>\`

## Class Hierarchy Rules
- **Read the full class hierarchy** before adding any new function or data member
- **Verify access specifiers**: public = callable from outside, private = internal, protected = subclass extension, ask to change them if needed.
- **Reuse existing virtual methods**: check if base class already has a virtual for the same concept before adding new methods
- **Extracting to base class**: copy code exactly — never modify logic during the move. Introduce virtual getters for subclass variation.
- **Never re-implement a template method just to inject logic mid-flow**: prefer making the relevant sub-step virtual so the subclass calls `Super` and patches the result. When the sub-step requires state unavailable to a virtual override, add a named virtual hook at that point in the base instead. Duplicating the full method body to change one line is always wrong.

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
- Runtime misses (no target found, empty list, optional feature) → plain `if`, no assert, add a `UE_LOG()` instead.

## Binding Rules
- Gate condition-based bindings at the binding site, not inside the callback
- Server-only or local-player-only bindings: guard before `AddDynamic`, not inside the handler

---

## GAS Conventions (Rules Violated by AI in the Past)

**Base class**: All ability classes must extend `UGeoGameplayAbility`. Never use `UGameplayAbility` directly.

**Server check**: Always use `UGameplayLibrary::IsServer(GetWorld())`.

**`OnFireTargetDataReceived` is server-only by design**: Never add `IsServer` guards inside it.

**Never override `Fire()` as a no-op**: It sends `FGeoAbilityTargetData` to the server, triggering `OnFireTargetDataReceived`. Overriding with empty body silently breaks the server chain. If subclass only needs server-side logic, override `OnFireTargetDataReceived` only.

**RNG in abilities**: Always seed from `StoredPayload.Seed` — `FRandomStream Stream(StoredPayload.Seed)`. Never call `FMath::Rand*` directly. Both client and server must derive identical values from the same seed.

**New projectiles default to `AGeoPooledProjectile`**: Extend `AGeoPooledProjectile` (not `AGeoProjectile` directly) unless  explicitly stated otherwise.

**VFX — never use multicast RPC or multicast delegate**: Clients have enough local context. Trigger VFX via Gameplay Cues.

---

## Debugging Rules

**NO WORKAROUNDS**: Find the actual root cause. Don't propose workarounds until the real problem is understood.

