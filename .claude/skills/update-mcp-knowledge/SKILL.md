---
name: update-mcp-knowledge
description: Capture new MCP/Python editor-automation techniques discovered this session into the AI/MCP docs and AI/Python scripts
---

# Update MCP Knowledge

Record everything genuinely new from the current session's MCP / Python editor-automation work, fully autonomously: facts go into `AI/MCP/` docs, reusable call sequences become generic scripts in `AI/Python/`. Skip anything already documented or specific to this one task. Do not ask the user what to capture — derive it from the conversation.

## Steps

1. **Find what was new this session.** Review the MCP / Python editor-automation work done in the current conversation (e.g. `execute_script`, `blueprint_modify`, `level_ops`, C++ shim calls). List two kinds of output:
   - *Facts*: each generic technique or constraint a future session would need (not the specific assets touched).
   - *Scripts*: each `execute_script` sequence that solved a multi-step or non-obvious problem and would be re-run with different assets.

2. **Read the doc-style rules.** Read `AI/MCP/MCP_DocStyle.md` before writing. Every edit must obey it: document how it works (not what failed), one sentence per constraint, generic concept names in prose (no project class/function names — those go in code), no inline C++ or Python.

3. **Check what already exists.** Read `AI/MCP/CLAUDE.md` (the topic→file index), the candidate target docs, and the existing `AI/Python/*.py` scripts. Drop anything already covered — only genuinely-missing facts and scripts get written; extend an existing script's helpers rather than duplicating them.

4. **Update or create the relevant docs.**
   - A new fact about an existing topic → add a concise line/section to that topic's `.md`.
   - A whole new topic → create a new `AI/MCP/MCP_<Topic>.md` (mirror the structure of an existing one) and register it in the `AI/MCP/CLAUDE.md` index.

5. **Write down the session's Python scripts.** Each retained sequence becomes a `.py` in `AI/Python/`:
   - Structure as **generic functions taking arguments** (asset paths, class names, property names, values) — no hardcoded assets inside function bodies.
   - Session-specific values appear only in one small example call at the bottom, which a future caller adjusts.
   - A docstring at the top states what the script does and how to run it (MCP `execute_script`, target world if relevant).
   - Reference each script by path from the doc section describing its technique — never inline scripts in `.md` files.

6. **Verify compliance.** Re-read each edit against `MCP_DocStyle.md`: generic, concise, no failure history, no inline code, references for scripts. Confirm new scripts are argument-driven with the example call at the bottom.

## Output

Briefly report each fact and script captured and where (file added/updated), plus anything deliberately skipped as already-covered.
