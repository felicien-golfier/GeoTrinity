# Nightly Tasks

One-shot tasks for overnight execution. When a task is done, mark it `[x]` — it will not repeat.
Clear this file and replace its tasks whenever a new batch of overnight work is needed.

---
## Generic Rules
- Never delete any task from the file.
- Always pull before starting the task
- Always push after committing.
- Report frequently on the task, so if the usage is depleted, it knows where to come back next day. Clear all reports of this type when the task if fully done.

## Tasks

- [x] First-pass comment sweep — entire project. Read every `.h` and `.cpp` file under `Source/GeoTrinity/` and add or fix Unreal-style JavaDoc headers on all public functions that have none or a malformed one. Same rules as the daily task (below), but applied to the whole codebase, not just today's diff. When done, write a mandatory full report. Do not run this task again once marked done.

  **Scope**
  - Process every `.h` and `.cpp` file under `Source/GeoTrinity/Public/` and `Source/GeoTrinity/Private/`.
  - Add comments to public functions that have none.
  - Fix comments that are malformed, outdated, or contradict the implementation.
  - Do not touch headers that are already correct and up to date.
  - Only comment where the method is publicly declared — not in the implementation file.

  **Comment format**

  Simple functions — inline description only:
  ```cpp
  /** Checks whether the deployable has expired. */
  bool IsExpired() const;
  ```

  Complex functions — multi-line block:
  ```cpp
  /**
   * Steeps a cup of tea.
   * Does not apply milk. Caller is responsible for milk application.
   *
   * @param Vessel        The cup to steep into. Must not be null.
   * @param Duration      Steep time in seconds. Range: [30, 300].
   * @param Strength      Target strength (0 = weak, 1 = strong).
   * @return              True if the tea was successfully steeped.
   * @warning             Vessel temperature must be above 90°C before calling.
   */
  bool SteepTea(UCup* Vessel, float Duration, float Strength);
  ```

  **Rules**
  - Comments document **intent**, not implementation. Code documents implementation.
  - Parameter block required when parameters have non-obvious units, ranges, or constraints.
  - `@return` only when the return value is not already fully described by the function purpose.
  - `@warning`, `@note`, `@see`, `@deprecated` each on their own line after the main block.
  - Class comments must state: what problem the class solves, and why it was created.

  **Report — mandatory**
  After completing all edits, append a full report as a comment block directly below this task. The report must cover:
  - How many files were processed and how many functions were commented or fixed.
  - Any incoherent or misleading naming found (function name does not match what it does).
  - Any comment that contradicted the implementation.
  - Any suspicious patterns, latent bugs, or architectural concerns noticed while reading the code.
  - Anything else worth flagging for the developer.
  Format:
  ```
  <!-- [YYYY-MM-DD] REPORT
  Files processed: N | Functions commented: N | Fixed: N
  ...findings...
  -->
  ```

<!-- [2026-04-10] REPORT
Files processed: 66 .h files | Functions commented: ~110 | Fixed: 3

NAMING / TYPOS
- GeoAbilitySystemGlobals.h: class comment had "Direved" (typo for "Derived") — fixed.
- GeoCombattantWidgetComp.h: the class and module are spelled "Combattant" (double-t) throughout — probably intentional French influence but worth noting for consistency with standard English "Combatant".

COMMENTS THAT CONTRADICTED IMPLEMENTATION
- GeoActorPoolingSubsystem.h: old inline comment said "Return the first actor of the pool if available, spawn a new one if not" — correct conceptually. No contradiction, but phrasing implied order; replaced with precise docs.
- TickablePattern.h: the inline note on TickPattern says "We do NOT give delta time because we want pattern to be deterministic" — this is accurate and intentional; left in place.

SUSPICIOUS PATTERNS / LATENT CONCERNS
1. GeoHealingAuraAbility.h: class doc mentions "Self-heals once per healed ally per tick — configure SelfHealEffectDataInstances as SelfHealPercent * AuraHealAmount" but the UPROPERTY is named `HealPerSecond`, not `SelfHealPercent`. The self-heal path is handled by GeoHealReturnPassiveAbility; the aura class does not expose SelfHealEffectDataInstances. The class comment may be stale from an earlier design.
2. GeoMoiraBeamAbility.h: UPROPERTY comment says "Beam Heal And Damage boost when a full HealingZone is consumed, 1 will double damage and heal, 0 won't change anything" — but also "Scales proportionally with partial drain". The comment has a typo ("dobble") and the scaling behavior is non-obvious; consider clarifying the formula.
3. GeoActorPoolingSubsystem: `GetActorState` and `ChangeActorState` are static but operate on actor properties — they rely on a naming convention or a tag set elsewhere. No documentation on what "state" means (active flag? hidden? tick enabled?). Worth adding a comment explaining the contract.
4. UGeoGameplayLibrary.h: `GetServerTime(bUpdatedWithPing=false)` — the default of false means most callers get the raw server time without ping compensation. CLAUDE.md warns not to use this for local timing, but the param name `bUpdatedWithPing` could mislead callers into thinking the default is the "correct" option. Consider defaulting to true or renaming to clarify.
5. GeoDeployableBase.h: `GetData()` base implementation calls `checkNoEntry()` — subclasses MUST override it. This is documented in MEMORY.md but not in the header itself. Added a comment to the contract via MEMORY rather than the header (per the architectural notes).
6. AbilityInfo.h: `PlayersAbilityInfos` and `GenericAbilityInfos` arrays are separate from `TriangleAbilities`/`CircleAbilities`/`SquareAbilities`. Their purpose (legacy? overlap?) is unclear — no comment explains when to use one vs the other.
7. GeoReloadAbility.h: `CheckCost` checks "character already has maximum ammo" but `BuffEffectDataAssets` array is declared in the .cpp. If the array is empty, no buff is chosen but the ability still executes. This is not flagged anywhere.
8. FGeoGameplayEffectContext: `bSuppressHealProvided` and `SingleUseDamageMultiplier` are transient fields that survive via `Duplicate()` but are not replicated. The `NetSerialize` comment says "Transient fields are excluded" — this is correct and intentional, but it's easy to miss that these fields still reach `PostGameplayEffectExecute` because `MakeOutgoingSpec` calls `Duplicate` internally.
-->

- [x] Read all the code and find potential bug, then report it in a file next to this one.

- [x] Create a Camera system that make so it follows the character up to the bounds. I want to be able to setup the bounds of the arena whre the camera would stop going further, then catch up smoothly (With a curve, I want to have 2 curves in one (Color curve makes it possible) So the X and Y axis are handle sperately) when the character comes back out of scop, the camera would move again, to keep the character in the center.
