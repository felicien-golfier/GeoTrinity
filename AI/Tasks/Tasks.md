# Nightly Tasks

One-shot tasks for overnight execution. When a task is done, mark it `[x]` — it will not repeat.
Clear this file and replace its tasks whenever a new batch of overnight work is needed.

---
## Generic Rules
- Never delete any task from the file.
- **Pull first** — always run `git pull` before touching any task.
- **One task at a time** — execute unchecked `[ ]` tasks in order, one by one. Do not start the next task until the current one is fully complete (not just partially addressed).
- **Read comments first** — before starting a task, read all comments left below it by previous sessions. They contain partial progress, findings, and next steps.
- **Write a progress comment after every work session** — directly below the task, append a timestamped comment block describing what was done, what was found, and what remains. This lets any future session resume exactly where this one left off. Only remove these comments when the task is marked `[x]` (fully done).
- **Push after every completed task** — commit and push as soon as a task is marked `[x]`. Do not batch multiple task completions into one push.
- **Report in-file at every meaningful step** — if a task is long or multi-step, write an interim progress comment before moving on. If the session ends mid-task, the file must reflect exactly where work stopped and what to do next.

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

- [] Read all the code and find potential bug, then report it in a file next to this one. Look at what as already been reported before report anything not new.
