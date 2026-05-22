# MCP Doc Style Rules

Rules for writing `.md` files in the `AI/MCP/` folder.

- Only document how things work, not what was tried or what failed.
- No failure history, no pitfall tables, no "we tried X first" context.
- Be succinct: one sentence per constraint, no elaboration.
- Be generic — no specific class or function names in prose; those belong in code snippets.
- No C++ code in `.md` files — it belongs in `.cpp`/`.h`. Reference the source file instead.
- No Python scripts or script excerpts inline — put them in `AI/Python/` as `.py` files and reference by path.
- One-line method table pointing to the header — no signatures.
