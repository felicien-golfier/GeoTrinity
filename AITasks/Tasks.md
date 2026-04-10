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

- [ ] First-pass comment sweep — entire project. Read every `.h` and `.cpp` file under `Source/GeoTrinity/` and add or fix Unreal-style JavaDoc headers on all public functions that have none or a malformed one. Same rules as the daily task (below), but applied to the whole codebase, not just today's diff. When done, write a mandatory full report. Do not run this task again once marked done.

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

- [x] Read all the code and find potential bug, then report it in a file next to this one.

- [ ] Create a Camera system that make so it follows the character up to the bounds. I want to be able to setup the bounds of the arena whre the camera would stop going further, then catch up smoothly (With a curve, I want to have 2 curves in one (Color curve makes it possible) So the X and Y axis are handle sperately) when the character comes back out of scop, the camera would move again, to keep the character in the center.