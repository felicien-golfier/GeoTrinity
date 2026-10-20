---
name: audit
description: Audit one task. Code → bugs first, then completeness, asset safety, coherence with the project and Unreal, over-engineering, tests, readability, code style. Pending file → the plan itself: correct, coherent, pitfalls ahead, not over-engineered. Argument = the scope (Pending file, paths, commit). Read-only; reports findings.
---

# Audit

**Scope** = `$ARGUMENTS`: a Pending file → **Plan audit**; paths, a commit or a folder → **Code audit**. Empty → code audit of the files changed in this conversation if it was not cleared, else of the uncommitted files (`git status`).

## Code audit
Read first: the `CLAUDE.md` of every touched folder, `AI/CodingStyle.md`, every touched file in full (not only the diff).

Look for, in order:
1. **Bugs**: wrong result, crash, server/client divergence, leak, wrong ordering, unhandled edge case, cost on a per-tick or per-projectile path.
2. **Completeness**: every caller and sibling code path of what changed still works or was updated.
3. **Assets and save data**: a renamed / removed / retyped property, function, enum value or default breaks no Blueprint, asset or save without a redirect.
4. **Project coherence**: does what the `CLAUDE.md` says; reuses what exists; respects module boundaries.
5. **Unreal coherence**: object lifetime, authority and replication, GAS prediction, cleanup on end of play, editor-only code kept out of game builds.
6. **No over-engineering**: the simplest code that does the job; no unused option, layer, abstraction or config.
7. **Tests**: the changed behaviour is tested, and the test would fail without the change.
8. **Readability**: a human gets it in one read: names, function size, a comment where the why is not obvious.
9. **Code style**: `AI/CodingStyle.md`.

## Plan audit
Read first: the Pending file, the docs it lists, `AI/CodingStyle.md`, the existing code it will touch.

Look for, in order:
1. **Goal**: the plan reaches the `Goal:` line; nothing missing.
2. **Correct**: the design works: authority and replication, ordering, edge cases, asset and save impact.
3. **Pitfalls ahead**: `TroobleshootingLogs.md`, engine limits, a step that cannot work as written.
4. **Coherent**: reuses what exists; right module and folder; planned names and code follow `AI/CodingStyle.md` and read clearly.
5. **No over-engineering**: the smallest plan that reaches the Goal; no task, class, option or abstraction the Goal does not need.
6. **Docs**: lasting facts in a `CLAUDE.md`, one-off steps in the Pending file (root `CLAUDE.md`, workflow step 3).
7. **Tests**: listed, and they would prove the Goal.
8. **Actionable**: each item concrete (path, name, value); an open decision is a question for the user, not a guess.

Items already ticked → also code-audit their code.

## Rules
- Read-only: change nothing.
- A bug needs a concrete scenario (state → wrong result). No scenario → drop it.
- Check each finding against callers and context before reporting it.
- Repeating a pitfall from `TroobleshootingLogs.md` is a bug.
- Doc and code disagree → report it, say which side looks wrong.

## Report
State the scope and the audit used. Findings in check order, one line each: `file:line` or Pending item — problem — scenario or fix.
End with a verdict: ready / fix first.
