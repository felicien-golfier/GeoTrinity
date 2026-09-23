---
name: update-mcp-knowledge
description: Capture new MCP/Python editor-automation techniques discovered this session into the AI/MCP docs and AI/Python scripts
---

# Update MCP Knowledge

Record only what a future session would genuinely miss from this session's MCP / Python editor-automation work: facts go into `AI/MCP/` docs, reusable call sequences into `AI/Python/` scripts. Work autonomously; don't ask the user what to capture.

**Default to adding nothing.** Each candidate must pass this test: *without it, would a future session fail, or lose real time rediscovering it?* If not, drop it. "Nothing new worth recording" is a valid and common outcome.

## What gets dropped
- Anything already said, even in other words, in the docs or scripts.
- Anything one API call, one doc lookup or one glance at the code reveals.
- Anything specific to this session's assets or task.
- Restatements, examples, rationale, or context around a fact.
- A script for a sequence that is short, obvious, or unlikely to be re-run.

## Steps

1. **List candidates.** From this conversation's MCP / Python work (`execute_script`, `blueprint_modify`, C++ shim calls…), note each generic technique or constraint and each non-obvious multi-step script. Apply the test above; most candidates should fall out here.

2. **Check what exists.** Read `AI/MCP/MCP_DocStyle.md`, `AI/MCP/CLAUDE.md` (topic→file index), the target docs, and `AI/Python/CLAUDE.md`. Drop what's covered.

3. **Edit in the smallest form.**
   - Prefer sharpening an existing line over adding one; if a new line makes an old one redundant, replace it. A doc should not grow when it can stay the same length.
   - One sentence per fact, shortest wording that stays unambiguous, per `MCP_DocStyle.md`.
   - New `MCP_<Topic>.md` only when the topic has no home at all; register it in `AI/MCP/CLAUDE.md`.

4. **Scripts, only when earned.** Prefer extending an existing script's functions over a new file. A new script goes in the matching `AI/Python/` subfolder: generic functions taking arguments, a one-line docstring, session values only in one example call at the bottom. Reference it by path from its doc line, and add a one-line row in `AI/Python/CLAUDE.md`.

5. **Re-read every edit** and cut any word, line or file that doesn't change what a future session would do.

## Output
One line per item captured (where it went), or "nothing new" — no list of what was skipped unless asked.
