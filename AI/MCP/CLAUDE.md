# MCP / Python Editor Automation

All editor automation goes through `mcp-unreal` `execute_script` (Python), or a C++ `UEditorUtilityObject` shim for what Python can't reach.

## When to use each approach
| Situation | Approach |
|---|---|
| Blueprint CDO properties, creating assets, reading tags | Python `execute_script` directly |
| Protected C++ property, template API, editor subsystem with no Python binding | C++ shim — see `MCP_EditorUtility.md` |

## Prerequisites
- Unreal Editor must be open before starting Claude Code — MCP tools register at session start.
- Tags added to `Config/Tags/GeoGameplayTags.ini` need an **editor restart** to resolve in Python.
- New `UFUNCTION` on a shim needs a **full build** (close editor); implementation-only `.cpp` changes can use Live Coding.
- Asset-mutating MCP tools change assets in memory only — save dirty packages after, repeat until none remain (`AI/Python/save_dirty_assets.py`).

## Reference files
| Topic | File |
|---|---|
| Blueprint asset creation, CDO properties, GameplayTag, components | `MCP_Blueprint.md` |
| Material creation, node wiring, hard-edge circle fill | `MCP_Material.md` |
| StateTree editing | `MCP_StateTree.md` |
| C++ editor utility pattern | `MCP_EditorUtility.md` |
| New enemy ability end-to-end (tag → BP → AbilityInfo → ASC → StateTree) | `MCP_NewEnemyAbility.md` |
| Widget Blueprint creation, widget tree shim, WidgetComponent setup | `MCP_UI.md` |
| Level settings (World Settings, GameMode override, PlayerControllerClass) | `MCP_Level.md` |
| Reading the running game in PIE | `MCP_PIE.md` |
| Live Coding builds with editor open, connecting MCP bridge | `MCP_LiveCodingAndConnect.md` |
| Reading/writing config-backed project settings | `MCP_Settings.md` |
| Doc style rules for `.md` files in this folder | `MCP_DocStyle.md` |

## Python scripts
Multi-step/reusable operations go in `AI/Python/` as `.py` — reference by path, never paste inline.

## Doc style
Always read `MCP_DocStyle.md` before editing docs here.
