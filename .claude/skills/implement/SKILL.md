---
name: implement
description: Start workflow step 4 on one AI/Pending file in a single turn. Loads the Pending file, its docs, the Read-map files in full and the CodingStyle, then implements. Argument = Pending name or part of it; empty = top of the README Order table.
disable-model-invocation: true
---

# Implement a Pending file

Loaded context (Pending file, CLAUDE.md docs, `AI/CodingStyle.md`, every Read-map file in full):

!`python AI/Python/Tools/pending_context.py $ARGUMENTS`

## Rules
- Invoking this skill is the user's go for workflow step 4. Do not re-read what is loaded above, do not ask which Pending file. Read other files only for what the LOAD REPORT lists as not inlined or not found, or when the code does not match the map (then fix the Read map).
- Start with the line `Context loaded by /implement — using docs + Pending file.`, then implement the unticked items in order.
- The script output was an error (no / ambiguous name)? Show the list it printed and ask which one.
- Then follow root `CLAUDE.md` step 4: tick items, `/test` (build + tests), fix drifted docs, delete the Pending file when empty, propose `/test quick` + bots / `auditor`. The `mode` rules apply.
