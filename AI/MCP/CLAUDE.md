# MCP / Python Editor Automation

All editor automation in this project goes through `mcp-unreal` `execute_script` (Python) or C++ `UEditorUtilityObject` shims for operations Python can't reach directly.

---

## When to use each approach

| Situation | Approach |
|---|---|
| Setting Blueprint CDO properties, creating assets, reading tags | Python `execute_script` directly |
| Protected C++ property, template API, editor subsystem with no Python binding | C++ `UEditorUtilityObject` shim — see `MCP_EditorUtility.md` |

---

## Prerequisites

- Unreal Editor must be open before starting Claude Code — MCP tools are registered at session start.
- Tags added to `Config/Tags/GeoGameplayTags.ini` require an **editor restart** to resolve in Python.
- Adding a new `UFUNCTION` to a shim requires a **full build** (close editor). Implementation-only `.cpp` changes can use Live Coding.

---

## Reference files

| Topic | File |
|---|---|
| Blueprint asset creation, CDO properties, GameplayTag, components | `MCP_Blueprint.md` |
| Material creation, node wiring, hard-edge circle fill | `MCP_Material.md` |
| StateTree editing (add/remove states, transitions) | `MCP_StateTree.md` |
| C++ editor utility pattern | `MCP_EditorUtility.md` |
| New enemy ability end-to-end (tag → BP → AbilityInfo → ASC → StateTree) | `MCP_NewEnemyAbility.md` |

---

## Python scripts

Multi-step or reusable operations go in `AI/Python/` as a `.py` file. Reference it by path in the relevant `.md` — never paste the full script inline.

---

## Doc style rules (for writing new `.md` files in this folder)

- Only document how things work, not what was tried or what failed.
- No failure history, no pitfall tables, no "we tried X first" context.
- Be succinct: one sentence per constraint, no elaboration.
- Python call sites only in code snippets — C++ patterns belong in `.cpp`.
- One-line method table pointing to the header — no signatures.
