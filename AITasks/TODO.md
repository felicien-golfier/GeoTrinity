# TODO

Tasks marked `[x]` without a recurrence tag are permanently done.
Tasks tagged `[recur:daily]` are reset to `[ ]` each day by this automated agent.

## Tasks

- [ ] [recur:daily] For every function modified today and every public function that has no comment or a malformed comment: add or fix its Unreal-style JavaDoc header. Rules below.
<!-- [2026-04-27] Processed 5 headers from today's commits (Shield Burst passive + GameplayCue rate-limiting work). Added: IsSuppressGameplayCue/SetSuppressGameplayCue docs in GeoAscTypes.h (new fields from rate-limit fix, zero docs); GetFireOrigin/GetFireYaw docs in GeoAbilitySystemComponent.h (sibling GetFireOrigin2D was already documented, these were not); InitializeMaterialInstances doc in ShieldBurstPassiveComponent.h (non-obvious public method with no hint of what it does); all public accessor/setter docs in GeoPlayerState.h (14 methods with no docs — notably SetDebugCombatStats takes 6 params and now has full @param block, and GetDebugRecv was ambiguous — it's damage received, not heal received); improved GeoProjectileAbility class comment (was a single vague line with no mention of client-prediction or extension pattern). Notable: GeoPlayerState had 14 public methods with zero documentation. SetPlayerClass has a subtle constraint — it does NOT grant abilities — that is easy to miss and now captured. -->


  **Scope**
  - Run `git diff --name-only HEAD` to get today's modified files. Only open and process `.h` and `.cpp` files from that list — do not scan the whole codebase.
  - Within those files: add comments to public functions that have none, fix comments that are malformed or contradict the current implementation, and update any comment whose intent changed today.
  - Do not touch headers that are already correct.
  - Only comment where the method is publicly declared — not in the implementation.

  **Comment format**

  Simple functions — inline description only (no parameter block needed):
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

  **Report**
  After completing the task, optionally append a short timestamped comment (usually 1–5 lines, it can be longer if needed.) directly below the task bullet. Flag anything worth surfacing: incoherent naming, a function whose comment contradicts its implementation, suspicious patterns, or anything that looks like a latent bug. Skip the report if nothing notable was found.
  Format: `<!-- [YYYY-MM-DD] <findings> -->`

