---
name: update-mcp-knowledge
description: Capture new MCP/Python editor-automation techniques discovered this session into the AI/MCP docs
---

# Update MCP Knowledge

Record any genuinely-new MCP / Python editor-automation technique used in the current session into `AI/MCP/`, following the project's doc-style rules. Skip anything already documented or specific to this one task.

## Steps

1. **Find what was new this session.** Review the MCP / Python editor-automation work done in the current conversation (e.g. `execute_script`, `blueprint_modify`, `level_ops`, C++ shim calls). List each generic *technique* or *constraint* — the kind of reusable fact a future session would need, not the specific assets touched.

2. **Read the doc-style rules.** Read `AI/MCP/MCP_DocStyle.md` before writing. Every edit must obey it: document how it works (not what failed), one sentence per constraint, generic concept names in prose (no project class/function names — those go in code), no inline C++ or Python.

3. **Check what already exists.** Read `AI/MCP/CLAUDE.md` (the topic→file index) and the candidate target docs. Drop anything already covered — only the genuinely-missing facts get written.

4. **Update or create the relevant doc.**
   - A new fact about an existing topic → add a concise line/section to that topic's `.md`.
   - A whole new topic → create a new `AI/MCP/MCP_<Topic>.md` (mirror the structure of an existing one) and register it in the `AI/MCP/CLAUDE.md` index.
   - Reusable multi-step scripts go in `AI/Python/` as a `.py` file, referenced by path — never inlined.

5. **Verify compliance.** Re-read each edit against `MCP_DocStyle.md`: generic, concise, no failure history, no inline code, references for scripts.

## Output

Briefly report each technique found and where it was documented (file added/updated). Note anything deliberately skipped as already-covered.
