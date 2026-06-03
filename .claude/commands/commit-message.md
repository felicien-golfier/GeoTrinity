---
description: Print a ready-to-paste git commit message based on the current changes
---

Look at the current changes:

- Staged and unstaged: !`git status --porcelain`
- Diff: !`git diff HEAD`

Read the actual diff (not just the filenames) and describe WHAT changed, not which files were touched.

Output rules — the message must be copy-pasteable directly into `git commit -m` from git bash:
- Describe the substance of each change in 2-3 words based on what the diff actually does (e.g. "fix turret recall timing", "add target-point selection", "tighten mine detonation radius") — NEVER vague verbs like "update X" or "refactor X" that don't say what changed.
- The ENTIRE message must be on a SINGLE line with NO line breaks at all — separate distinct changes with `, ` instead of newlines or bullets, so it pastes cleanly into `git commit -m "..."`.
- Output ONLY the full command `git commit -m "..."` (with the message inside the quotes), nothing before or after it (no explanation, no preamble, no closing remarks).
- Wrap it in a fenced code block so it can be copied cleanly.
- Use plain LF line endings (Unix style), never CRLF.
- Do NOT add any trailing co-author/attribution lines.
