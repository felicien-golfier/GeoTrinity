---
name: audit-fix
description: Audit one task like /audit, then fix every finding that has one clear fix, on its own; findings that need a discussion or a choice are left untouched and asked at the end. Argument = the scope (Pending file, paths, commit), same as /audit.
---

# Audit + fix

The user invoking this skill is the direct ask to code (root `CLAUDE.md` waiver): fix without a Pending file or a stop for review.

## 1. Audit
Read `.claude/skills/audit/SKILL.md` and run it on `$ARGUMENTS`: same scope rules, reading, checks and rules. Keep the findings; no report yet.

## 2. Sort each finding
**Fix** — one obvious right fix, no trade-off:
- a bug with a concrete scenario and a local fix;
- a missed caller / sibling path, a missing cleanup, a missing redirect;
- code style, naming, readability, a missing comment on a non-obvious why;
- dead or over-engineered code inside the scope (unused option, layer, parameter);
- a missing test for the changed behaviour;
- doc and code disagree and one side is plainly the stale one;
- a Pending file item that is vague, misplaced (lasting fact → `CLAUDE.md`) or contradicts the code, when the right wording is clear.

**Ask** — the user decides:
- several valid fixes, or the fix changes gameplay, feel, balance, look, sound or a tuned value;
- a design / architecture change, a new class, module or folder move beyond the finding;
- renaming or removing anything a Blueprint, asset, save or map uses;
- touching code outside the scope, deleting files, editor or asset work;
- doc and code disagree and either could be the intent;
- a plan's open decision, or a Goal the plan misses.

Unsure → Ask.

## 3. Fix
- Before code: `AI/CodingStyle.md` (already read in step 1). Each fix is the smallest change that removes the finding; nothing else.
- Plan audit: fixes go into the Pending file and the docs it lists; no code.
- A fix changes a lasting fact → update the owning `CLAUDE.md` in the same pass (root `CLAUDE.md`, "Where a fact goes").
- A fix that turns out to need a choice → revert it, move the finding to Ask.

## 4. Verify
- Re-run the audit checks on what was changed; new findings go through step 2 once.
- Code changed under `Source/` → build and tests per root `CLAUDE.md` ("Rules": Tutorial gate, big-task build) and `AI/CodingStyle.md` ("Tests"). Not green → say so with the failure; never loosen a test to pass.
- No editor testing unless the user asked.

## Report
Scope and audit used, then:
- **Fixed**: one line each — `file:line` — problem — fix.
- **Needs your call**: one line each — `file:line` or Pending item — problem — options, the recommended one first.
- **Build / tests**: what ran, result; or "not run" and why.

Verdict: ready / decide first / fix first.
