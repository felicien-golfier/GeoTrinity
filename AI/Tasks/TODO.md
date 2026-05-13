# TODO

Tasks marked `[x]` without a recurrence tag are permanently done.
Tasks tagged `[recur:daily]` are reset to `[ ]` each day by this automated agent.

## Tasks

- [ ] [recur:daily] For every header modified in the last 25h and every public function that has no comment or a malformed comment: add or fix its Unreal-style JavaDoc header. Rules below.

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
  After completing the task, optionally append a short timestamped comment (usually 1–5 lines, it can be longer if needed.) directly below this bullet. Flag anything worth surfacing: incoherent naming, a function whose comment contradicts its implementation, suspicious patterns, or anything that looks like a latent bug. Skip the report if nothing notable was found.
  Format: `<!-- [YYYY-MM-DD] <findings> -->`

- [ ] [recur:daily] Read every `.h` and `.cpp` file changed in the last 25h and update the corresponding `CLAUDE.md` files to stay in sync with the code. End with the CLAUDE.md at the root, ensure Structure is still fine and update what's needed.

  **Scope**
  - Run `git diff --name-only HEAD` to get today's modified files. Only open `.h` and `.cpp` files from that list.
  - For each changed file, identify its subfolder `CLAUDE.md` (the one sitting next to the `.h` files) and the parent-folder `CLAUDE.md` above it — update both if needed.
  - Only update what actually changed. Do not touch sections unaffected by today's diff.

  **Two-tier CLAUDE.md rule — strictly enforce this:**
  - **Subfolder CLAUDE.md** (lives next to the `.h` files) — this is the full reference. Include: class purpose, lifecycle, override points, key fields, networking rules, and any non-obvious constraints. Keep it concise but complete. Remove lines that are no longer accurate.
  - **Parent-folder CLAUDE.md** (one level up, e.g. `Deployable/CLAUDE.md` vs `Mine/CLAUDE.md`) — one short sentence per subfolder/file, nothing more. It is an index, not a reference. Do not duplicate details already in the subfolder CLAUDE.md.

  **Writing rules**
  - Be concise. Every line must earn its place — delete stale or redundant lines before adding new ones.
  - Document *why* something works the way it does, not *what* the code does (the code documents itself).
  - Use bullet points, not prose paragraphs.
  - Never describe the task, the fix, or the PR in the CLAUDE.md — only the steady-state design.

  **Report**
  After completing the task, optionally append a short timestamped comment directly below this bullet listing which CLAUDE.md files were updated and what changed. Skip if nothing needed updating.
  Format: `<!-- [YYYY-MM-DD] <findings> -->`

