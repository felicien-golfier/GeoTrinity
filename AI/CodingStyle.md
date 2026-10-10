# Coding Style & Conventions

## Simplicity is the #1 priority
Solve the exact ask and nothing more. Before writing, find the simplest mechanism already available — a timer,
an existing function, an existing member — and use it.

- **Simple, not merely small.** Plain constructs and obvious control flow. If a shorter version makes the
  reader work out *why* it works, the longer obvious one wins.
- **Smallest correct diff.** Fewest new members, functions, files, branches and lines. Reuse or extend what
  exists rather than adding alongside it.
- **One mechanism, not two.** Pick the single construct that expresses the whole behaviour; never bolt extra
  state or a parallel path alongside it. Don't add state to track what an existing construct already bounds.
- **Write the simple version, then stop.** It is the deliverable — never ship an elaborate one "to be safe".
- **YAGNI.** No unused parameters, variables or speculative features. Delete a declaration *and* its
  implementation when its last call site goes; no dead-code safety nets.

## Writing code
- Prefer fewer, longer `if`s — merge with `&&` rather than nesting.
- One blank line after the closing `}` of an `if`/`else`/loop/`switch` block when another statement follows it
  (never before `else` or a closing `}`). clang-format can't insert it — write it yourself.
- **No `return` that skips the rest of a function.** Once the function has started its work, never bail out
  of it: wrap the remaining work in an `if` (or fold the condition into the loop's), so whoever appends code
  later sees which case it runs in. Fine: an early return — a guard at the top, before any work (the
  `if (!ensureMsgf(...)) { return; }` form included) — and a return that *is* the result, like a search loop
  returning what it found, with the not-found return closing the function.
- `const` by default. Prefer a non-const parameter over a const ref plus a local copy.
- Forward declare in headers rather than including (`enum class EMyEnum : uint8;`).
- No abbreviations in names, except `ASC`. Same style and naming throughout.
- **A name states exactly what the code does** — every variable, flag, function and member. A name that promises
  more or less than the code does is a bug: `bFixedAmount` was wrong when the amount still moved with
  `SingleUseDamageMultiplier`. Rather than listing every member, name the category and define it once in the
  declaration's doc comment: `bSkipStatModifiers`, whose comment says exactly which multipliers count as stat
  modifiers and which do not. When the code changes, fix the name or the category's definition in the same change.
- Every `UPROPERTY`/`UFUNCTION` `Category` starts with `Geo` (`"GeoCamera|Zoom"`), so project params are
  recognizable among engine ones.
- Name unused parameters in the `.h`; comment them out (`/*Name*/`) in the `.cpp`.
- `Super` placement follows semantics (Init at top, Destroy at bottom); with no ordering dependency, top.
- **No unnamed namespaces, ever.** A file-local helper belongs either in a library — `GeoASLib` when it
  touches the ability system, `GeoLib` otherwise — or to the class as a `const` member function.
- Get an ASC with `GeoASLib::GetGeoAscFromActor`, never `GetComponentByClass`.
- Remove trivial wrappers that only delegate; call directly.
- **Never assume an engine name exists** — read the engine header before using any UE constant (including
  `FColor::X`) or method, and before implementing against StateTree, GAS or AI. If a constant does not exist,
  write explicit RGB/RGBA values. Plugin source: `Engine\Plugins\Runtime\<PluginName>\Source\<Module>\`.

## Comments
- **No comments in a `.cpp` when the code speaks for itself** — the default is zero. Doc comments live on the
  declaration in the `.h`; the body carries none. Before writing one, name the thing better instead: a named
  local or a named function beats a sentence.
- Where a line genuinely is not self-explanatory, a few words are enough. If it takes three lines to explain,
  the code is wrong, not under-commented.
- Never restate the code, and never record history ("this is the correct procedure instead of X") — that
  belongs in the commit message.

## Before adding or changing anything
- **Grep every read and write site** before adding or removing a field or function. A field may already be
  populated by a base class; a setter may already exist.
- **Check for a function that already does it.** If two functions differ only by a constant (a trigger type,
  a flag), merge them into one with a parameter. Never add a wrapper forwarding a hardcoded argument.
- **Functions do exactly what their name says.** A getter never mutates; a query never triggers side effects.
  If it needs to do more, split it.
- **No duplicated code** — extract to a base class, a component or a free function. Extract by *concept*, not
  by textual match: if the same operation happens in more than one method, pull it into one named function
  even when the pieces are not identical lines and span different statement kinds.

## Class hierarchy
- Read the full hierarchy before adding a function or member, and read the `.cpp` of every virtual you plan to
  override — the implementation may branch, return early, or have side effects the signature does not show.
- Verify access specifiers, and **prefer relaxing access over adding complexity**: if moving a member to
  `protected` removes a wrapper, a duplicate handle or an indirection, move it.
- Reuse an existing virtual for the same concept before adding a new one.
- When extracting to a base class, copy the code exactly — never change logic during the move; introduce
  virtual getters for subclass variation.
- **Never re-implement a template method to inject logic mid-flow.** Make the sub-step virtual so the subclass
  calls `Super` and patches the result, or add a named virtual hook at that point in the base. Duplicating the
  method body to change one line is always wrong.

## Error handling — no silent fallbacks
Never silently skip or substitute when something required is missing. Ask: *can this condition legitimately be
false at runtime?* If no, it must be flagged. Never use `condition ? A : B` as a quiet fallback.

| Case | Handling |
|---|---|
| Missing configured asset (projectile class, effect data, curve, subsystem) | `ensureMsgf` — configuration bug |
| Wrong actor or component type | `ensureMsgf` — design bug |
| Critical invariant whose silent continuation corrupts state | `checkf` |
| Runtime miss (no target found, empty list, optional feature) | plain `if` plus `UE_LOG` — no assert |

`if (!ensureMsgf(x, TEXT("...")))  { return; }` where execution must stop; `ensureMsgf(x, TEXT("..."))` alone
otherwise.

**An ensure on something the code needs afterwards must return** (or take the guarded branch): if the lines after it
dereference `x` or give a wrong result without it, use the `if (!ensureMsgf(...))` form. A bare `ensureMsgf` is only for
a value whose absence merely degrades the result (an unset icon draws nothing, nothing crashes). If unsure which, ask
what the next line does with `x` when it is null.

## Bindings
Gate condition-based bindings at the binding site, not inside the callback — guard server-only or
local-player-only bindings before `AddDynamic`.

## GAS conventions
- Every ability extends `UGeoGameplayAbility`, never `UGameplayAbility`.
- Server check is always `UGameplayLibrary::IsServer(GetWorld())`.
- `OnFireTargetDataReceived` is server-only by design — never add an `IsServer` guard inside it.
- **Never override `Fire()` as a no-op**: it sends `FGeoAbilityTargetData` to the server, which triggers
  `OnFireTargetDataReceived`. A subclass needing only server logic overrides `OnFireTargetDataReceived`.
- RNG seeds from `StoredPayload.Seed` (`FRandomStream Stream(StoredPayload.Seed)`) — never `FMath::Rand*`, so
  client and server derive identical values.
- **Base logic reads the default attribute, never the live one.** Anything an ability scales from a target's stat (a
  percentage of its max health, a drain rate, a ratio) takes the unpumped value from
  `UGeoAbilitySystemComponent::GetDefaultAttributeValue`, so gems and multipliers (`HealthMultiplier` on deployables)
  stay a separate layer and a pumped target never changes the ability's own rules. Read the live attribute only for
  the target's current state: is it full, how much can it actually lose, what is left to drain.
- **An ability reacts to an event through a delegate, never through a call planted in someone else's code.** The code
  where the event happens broadcasts a delegate (on the ASC, or another hub its listeners can reach) and never names
  the abilities that react; a passive binds in its activation and unbinds when it ends. An ability that needs
  information from another queries it; it never inserts its own logic into the other's code. The one exception is
  abilities built as one mechanic, such as the Martyr's Wrath mark and its beam.
- **A gem's values live on the gem, never in `UGameDataSettings`.** One number is its `MagnitudePerGem` in the catalog
  (`Data/gems.csv`); a Core's other tunables sit on its passive ability (`FGeoGemInfo::GrantedAbility`).
- New projectiles extend `AGeoPooledProjectile`, not `AGeoProjectile`, unless stated otherwise.
- VFX go through Gameplay Cues — never a multicast RPC or multicast delegate. Clients have the local context.

## Debugging
**No workarounds.** Find the root cause before proposing anything.
