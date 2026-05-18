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
| StateTree editing (add/remove states, transitions) | `MCP_StateTree.md` |
| C++ editor utility pattern | `MCP_EditorUtility.md` |
| New enemy ability end-to-end (tag → BP → AbilityInfo → ASC → StateTree) | `MCP_NewEnemyAbility.md` |

---

## Doc style rules (for writing new `.md` files in this folder)

- Only document what isn't obvious from the code — point to files, don't repeat bodies.
- Only document what succeeded. No failure history or pitfall tables.
- Python call sites only in code snippets — C++ patterns belong in `.cpp`.
- One-line method table pointing to the header — no signatures.
- Constraints as tight bullets under "Key Constraints". One sentence each.
