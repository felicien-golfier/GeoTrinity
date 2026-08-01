# TODO

Tasks marked `[x]` without a recurrence tag are permanently done.
Tasks tagged `[recur:daily]` are reset to `[ ]` each day by this automated agent.

## Tasks

- [x] [recur:daily] For every header modified in the last 25h and every public function that has no comment or a malformed comment: add or fix its Unreal-style JavaDoc header. Rules below.

  **Scope**
  - Run `git diff --name-only @{25.hours.ago} HEAD` to get the files changed by commits in the last day. Only open and process `.h` and `.cpp` files from that list — do not scan the whole codebase. (Do NOT use `git diff --name-only HEAD` — that only reports uncommitted working-tree changes, which are empty in the cloud checkout, so the day's committed work would be missed.)
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


- [x]  UGeoCombatStatsSubsystem needs to add a new field that calculate DPS and HOT during the entire fight (since MatchState Start until it changes). Free all FCombatEventRecord in the Tarray DamageDealt anf HealDealt in FActorCombatStats at the end of the match state and stop ComputePlayerStats when not InProgress. If any actor does damages or heal it should ComputePlayerStats. Don't forget to reset the values at start and end in progress match state. Let's say that you have a "Incombat" mode where you compute the stats, and when not in combat, just display the last values without changing them.


- [x] the void AGeoGameState::Loot() function needs to launch buff pickups all arround where the boss died in loop. I should reuse the info from the CDO Reload and just launch a random buff every .2 sec until gamestate change state.
