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
- Asset-mutating MCP tools change assets in memory only — save dirty packages after, repeat until none remain (`AI/Python/save_dirty_assets.py`); that pass also writes packages the editor itself dirtied, so save by path when only the script's own assets should move.

## Reference files
| Topic | File |
|---|---|
| Blueprint asset creation, CDO properties, GameplayTag, components | `MCP_Blueprint.md` |
| Material creation, node wiring, hard-edge circle fill | `MCP_Material.md` |
| Niagara emitter stack editing (modules, static switches, input values, dynamic inputs) | `MCP_Niagara.md` |
| StateTree editing | `MCP_StateTree.md` |
| Reading/authoring skeletal animation, montage structure, rig editing | `MCP_Animation.md` |
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
Run one by compiling and executing the file's own source in the editor rather than resending its text.
A script that raises still reports success to the caller; its traceback goes to the editor log under the Python category.
Nothing a script prints comes back through the tool, so a script with results to report writes them to a file the caller reads.
Every script is re-runnable: a script that writes an asset frees the path first, since creating one over a name already in use breaks in the editor rather than overwriting.
A path the editor still holds refuses deletion, so free it by renaming it aside; an asset whose own editor window is open refuses both, and only closing that asset's editor frees it.
A long script holds the editor's game thread, so every call times out until it returns — wait on the file it writes rather than sending it again.
A script that writes its assets before it reports leaves them written when the report raises, so a failed report is not a failed build and the traceback in the report file is the only sign of one.
Comment only what the code cannot say: a constraint, an ordering, a bound. A few words, never prose, and never rationale or intent.
Docstrings are one line; a second only where a caller would otherwise get it wrong.

## Animation
Always read the motion rules in `MCP_Animation.md` before authoring an animation.

## Doc style
Always read `MCP_DocStyle.md` before editing docs here.
