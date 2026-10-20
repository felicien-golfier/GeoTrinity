# MCP / Python editor automation

Two servers: UE 5.8 built-in `mcp-epic` (default for reads and every toolset-covered write: Blueprint graphs, material graphs, properties, tables, tags) and `mcp-unreal` `execute_script` (Python in `AI/Python/`, plus C++ `UEditorUtilityObject` shims for what Python cannot reach). Which one: `MCP_UnrealBuiltin.md` → "Which server".

## Prerequisites
- Editor open before Claude Code starts (tools register at session start). `mcp-epic` is disconnected by default: ask the user for access whenever a task needs it (`MCP_EpicTesting.md`, "Setup").
- MCP client down → POST scripts to the bridge on the `.mcp.json` port (`MCP_LiveCodingAndConnect.md`).
- New tags in `Config/Tags/GeoGameplayTags.ini` → editor restart.
- New `UFUNCTION` / type / header change → full build, editor closed. Implementation-only `.cpp` → Live Coding.
- MCP asset writes are in memory: save dirty packages after (`AI/Python/Asset/save_dirty_assets.py`), repeat until none remain; save by path when only the script's assets should move.

## Script rules
- Multi-step or reusable work = `.py` in `AI/Python/<Topic>/`, referenced by path; run by compiling the file's own source.
- One-off change (set a value, flip a flag, bind one parameter, rename, wire one asset) = no script: `mcp-epic` property / Blueprint / material / table tools (`MCP_UnrealBuiltin.md` → "Which server"), or an inline `execute_script` snippet that is not saved.
- A script left that did its one change and is not re-run on rebuild (a rebuild path, a regenerated asset, a recurring maintenance pass) is deleted in the same session: file, its row in the folder `CLAUDE.md`, every reference. The technique worth keeping moves to the matching `MCP_<Topic>.md`.
- A script with results writes them to a file the caller reads.
- Re-runnable: free the target path first (rename aside; close its editor window; else rewrite in place).
- Compile a Blueprint before saving class-default writes (`MCP_Blueprint.md`).
- Comments: only what code cannot say (constraint, ordering, bound); docstrings one line.

## Topic docs
| Topic | File |
|---|---|
| Blueprint assets, CDO, instanced subobjects, curves, tags, components | `MCP_Blueprint.md` |
| Materials: creation, wiring, layers, dry-run | `MCP_Material.md` |
| Niagara emitter stack | `MCP_Niagara.md` |
| StateTree | `MCP_StateTree.md` |
| Skeletal animation, montages, rigs | `MCP_Animation.md` |
| C++ editor utility pattern | `MCP_EditorUtility.md` |
| New enemy ability end to end | `MCP_NewEnemyAbility.md` |
| Widget Blueprints, widget tree shim | `MCP_UI.md` |
| Project / per-level settings | `MCP_Settings.md` |
| Reading the running game in PIE | `MCP_PIE.md` |
| Judging a visual change without PIE | `MCP_Preview.md` |
| Testing the running game with `mcp-epic` only: teleport, log, GAS, key input, limits | `MCP_EpicTesting.md` |
| Live Coding, connecting the bridge | `MCP_LiveCodingAndConnect.md` |
| UE built-in MCP: enable, connect, toolsets | `MCP_UnrealBuiltin.md` |
| Audacity | `Audacity/CLAUDE.md`, `GameSoundDesign.md`, `ListeningToSound.md` |
| GIMP | `Gimp/CLAUDE.md`, `ImageAuthoring.md` |
| FFmpeg video | `Video/CLAUDE.md`, `VideoEditing.md` |
| DaVinci Resolve | `Resolve/CLAUDE.md`, `ResolveTools.md` |
| Doc style for this folder | `MCP_DocStyle.md` (read before editing docs here) |

Motion rules in `MCP_Animation.md` before authoring an animation. Script index: `AI/Python/CLAUDE.md`.

## Hazards (silent or fatal)
- **Frozen editor**: the user's editor runs under Rider's debugger, so a failed `check` / `ensure` freezes it on a breakpoint (0% CPU, Not Responding, MCP calls time out, log stops at "Script Stack (0 frames)") rather than crashing; `CrashReportClientEditor` running proves nothing. Read Rider's window title and stack; never resume, stop or detach the debugger for the user, report the stopped line.
- **Structs write through**: `get_editor_property("struct")` is bound to its object; every member `set_editor_property` writes and notifies, so a multi-member edit exposes inconsistent states the engine `check()`s on (material layers: `Blends.Num() == Layers.Num() - 1`). `.copy()`, edit, assign once.
- **GameplayTag repr**: a tag (or a struct holding one) prints `{}` set or not; read with `export_text()` and cross-check `git show HEAD:<asset>` before dropping a property.
- **T3D export**: export the Material itself to read constant inputs; never a subobject (`EditorOnlyData`) — fatal.
- **Static mesh rebuild**: call `get_num_vertices(0)` before `build_from_static_mesh_descriptions` on a loaded mesh (waits out the async compile; otherwise Fatal "FRenderResource deleted without being released").
- **Registry after save**: a re-saved asset still lists its old references until `scan_files_synchronous([file], True)`; rescan between save and a delete-if-unreferenced check.
- **Blueprint node deletion**: `delete_node` over the bridge is blocked by the permission classifier; dump the graph, write the C++, hand the user the exact nodes to delete.
- **Server RPCs run locally** from editor Python: `MCP_PIE.md`.
- **Hand edits**: scripts never overwrite what the user changed by hand (`AI/UI.md`, `asset_guard`).
- **MCP client down**: probe the bridge port before reporting automation unavailable (`MCP_LiveCodingAndConnect.md`).
