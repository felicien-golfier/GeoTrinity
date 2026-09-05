# MCP / Python Editor Automation

All editor automation goes through `mcp-unreal` `execute_script` (Python), or a C++ `UEditorUtilityObject` shim
for what Python cannot reach. Blueprint CDO properties, asset creation and tag reading go straight through
Python; a protected C++ property, a template API or an editor subsystem with no Python binding needs the shim —
see `MCP_EditorUtility.md`.

## Prerequisites
- The Unreal Editor must be open **before** starting Claude Code — MCP tools register at session start.
- Tags added to `Config/Tags/GeoGameplayTags.ini` need an **editor restart** to resolve in Python.
- A new `UFUNCTION`, new type or header change needs a full build with the editor closed; implementation-only
  `.cpp` changes use Live Coding — see `MCP_LiveCodingAndConnect.md`.
- Asset-mutating MCP tools change assets in memory only. Save dirty packages after, repeating until none
  remain (`AI/Python/Asset/save_dirty_assets.py`); that pass also writes packages the editor itself dirtied, so save
  by path when only the script's own assets should move.

## Reference files
| Topic | File |
|---|---|
| Blueprint asset creation, CDO properties, instanced subobjects, curve assets, GameplayTag, components | `MCP_Blueprint.md` |
| Material creation, node wiring, hard-edge fill patterns | `MCP_Material.md` |
| Niagara emitter stack editing (modules, static switches, input values, dynamic inputs) | `MCP_Niagara.md` |
| StateTree editing | `MCP_StateTree.md` |
| Reading/authoring skeletal animation, montage structure, rig editing | `MCP_Animation.md` |
| C++ editor utility pattern | `MCP_EditorUtility.md` |
| New enemy ability end-to-end (tag → BP → AbilityInfo → ASC → StateTree) | `MCP_NewEnemyAbility.md` |
| Widget Blueprint creation, widget tree shim, WidgetComponent setup | `MCP_UI.md` |
| Config-backed project settings and per-level settings (World Settings, GameMode override) | `MCP_Settings.md` |
| Reading the running game in PIE | `MCP_PIE.md` |
| Judging a visual change in the editor world, without PIE | `MCP_Preview.md` |
| Live Coding builds with the editor open, connecting the MCP bridge | `MCP_LiveCodingAndConnect.md` |
| Doc style rules for `.md` files in this folder | `MCP_DocStyle.md` |

## Python scripts
Multi-step or reusable operations go in `AI/Python/` as `.py` — reference by path, never paste inline. Run one
by compiling and executing the file's own source in the editor rather than resending its text. Scripts sit in a
topic subfolder, each one indexed with a one-line purpose in `AI/Python/CLAUDE.md`; read that before writing a
new one, and put the new one in the folder matching what it touches.

- Nothing a script prints comes back through the tool, and a script that raises still reports success — its
  traceback goes to the editor log's Python category. A script with results to report writes them to a file the
  caller reads.
- A script that writes its assets before it reports leaves them written when the report raises, so a failed
  report is not a failed build, and the traceback in the report file is the only sign of one.
- A long script holds the editor's game thread, so every call times out until it returns — wait on the file it
  writes rather than sending it again.
- Every script is re-runnable, which means freeing the target path first: creating an asset over a name already
  in use breaks in the editor rather than overwriting. A path the editor still holds refuses deletion, so free
  it by renaming it aside; an asset whose own editor window is open refuses both, and only closing that window
  frees it. That window closes from a script; the running editor itself never does. An asset whose path cannot
  be freed at all is still rewritable in place — strip what it holds and add the new contents — which needs no
  free path and keeps every reference to it intact.
- Comment only what the code cannot say: a constraint, an ordering, a bound. A few words, never prose, never
  rationale. Docstrings are one line; a second only where a caller would otherwise get it wrong.

## Before editing
Read `MCP_DocStyle.md` before editing docs here, and the motion rules in `MCP_Animation.md` before authoring an
animation.
