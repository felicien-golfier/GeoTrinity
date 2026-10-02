# Built-in Unreal MCP (UE 5.8)

UE 5.8 ships its own MCP server, the experimental **Unreal MCP** plugin (`ModelContextProtocol`). It runs inside the
editor as a streamable-HTTP MCP endpoint, and its tools come from **toolset** plugins registered with the Toolset
Registry. It lives alongside `mcp-unreal`; neither replaces the other (see "Which server" below).

Engine sources: `Engine/Plugins/Experimental/ModelContextProtocol`, `Engine/Plugins/Experimental/ToolsetRegistry`,
`Engine/Plugins/Experimental/Toolsets/*`.

## Enabling

- Enable `ModelContextProtocol` plus the toolset plugins you want in `GeoTrinity.uproject`, each with an `Editor`
  target allow-list like the `MCPUnreal` entry. `AllToolsets` pulls in every toolset at once.
- All of them ship precompiled in the launcher engine, are editor-only and marked NoRedist, so they never reach a
  package or the dedicated server.
- A plugin change takes effect on the next editor start; ask the user to restart, never do it yourself.

Toolsets worth enabling for this project:

| Plugin | What it gives |
|---|---|
| `EditorToolset` | Assets, actors, Blueprints (graph DSL + layout), materials, data tables, curve tables, meshes, textures, scene; editor app (selection, camera, viewport/asset/editor capture, open asset, start/stop PIE); output-log reading; the batching tool below |
| `GASToolsets` | Live ASC state of an actor (attributes, active effects, granted abilities, tags), attribute-set and cue lookup, cue notify creation |
| `GameplayTagsToolset` | Read and manage gameplay tags |
| `StateTreeToolset` | StateTree inspection |
| `NiagaraToolsets` | Niagara system/emitter editing |
| `UMGToolSet` | Widget creation and editing through reflection |
| `ConfigSettingsToolset` | List, inspect and edit config sections |
| `AutomationTestToolset` | Discover and run automation tests |
| `LiveCodingToolset` | Live Coding compile — a build under the project rules, so only on the user's explicit request |
| `SlateInspectorToolset` | Slate UI inspection and automation |

## Starting the server

The server does not start by default. Any one of these starts it:

- `bAutoStartServer=True` under `[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]` in
  `Config/DefaultEditorPerProjectUserSettings.ini` (Editor Preferences → Model Context Protocol writes the same key per user).
- `-ModelContextProtocolStartServer` on the editor command line.
- Console `ModelContextProtocol.StartServer [port]`; `ModelContextProtocol.StopServer` stops it.

It serves `http://127.0.0.1:8000/mcp` by default. `ServerPortNumber` and `ServerUrlPath` in the same section change it,
and `-ModelContextProtocolPort=` overrides the port per launch. Port 8000 does not collide with the `mcp-unreal`
ports. A commandlet never auto-starts the server unless passed the start flag. The server only accepts requests with
no `Origin` header or a localhost one.

## Connecting Claude Code

The entry in `.mcp.json` is an HTTP server, named `unreal-mcp`, next to the existing `mcp-unreal` entry:

```json
"unreal-mcp": { "type": "http", "url": "http://127.0.0.1:8000/mcp" }
```

Console `ModelContextProtocol.GenerateClientConfig ClaudeCode` writes that entry into the project's `.mcp.json`, keeping
every other entry. The editor must be serving before the Claude Code session starts, or the server registers as failed;
`/mcp` reconnects it. Its tools then appear as `mcp__unreal-mcp__<tool>`.

## Calling tools

With tool search on (the default, `bEnableToolSearch`), the server lists only three tools:

1. `list_toolsets` — every registered toolset name with its description.
2. `describe_toolset` (`toolset_name`) — that toolset's JSON schema: each tool's name, description and input schema.
3. `call_tool` (`toolset_name`, `tool_name`, `arguments`) — runs one tool; `arguments` must match its input schema.

- Use toolset names exactly as `list_toolsets` prints them, and tool names without a toolset prefix.
- Describe a toolset once per session before calling into it; the schema is the only reference for argument names.
- Objects and assets are passed as path strings; results come back as JSON text.
- Turning tool search off registers every toolset tool natively — hundreds of tools in the session, so leave it on.
- Tools run on the editor's game thread, so a long tool holds the editor like a long script does.
- `ModelContextProtocol.RefreshTools` rebuilds the tool list after a toolset changes; the server notifies connected
  clients itself.

## Batching tool calls

The programmatic toolset in `EditorToolset` chains many tool calls in one round trip:

- Call `get_execution_environment` once per session and follow its `instructions` field.
- `execute_tool_script` takes Python source defining `run()`, which returns a dict with string keys; that dict is the result.
- Inside the script, `execute_tool` is the only way to reach the editor. The `unreal` module is not importable — only
  `json`, `math`, `datetime`, `copy`, `re` and `time`.
- `open` is read-only and confined to the project directory.
- A script that raises returns the error with its traceback, unlike `mcp-unreal`, which reports success.

## Which server

Both servers talk to the same open editor, so choose per task, not per session. **The built-in server is the default
for reads and for typed writes; `mcp-unreal` is the Python server** — it is for work that needs the editor's full
Python API or an existing script in `AI/Python/`.

**Reads go to the built-in server.** A typed read returns structured JSON in one call, raises real errors, and needs
no script, no result file and no traceback handler. Use it to inspect before any change and to verify after one.

| To read | Toolset → tools |
|---|---|
| Material graph: nodes, what feeds each node and each material output, parameter groups, materials using a function | Material → `get_expressions`, `get_expression_inputs`, `get_property_input`, `get_expression_input_names`/`_output_names`, `list_parameter_groups`, `get_referencing_materials` |
| Material instance parameters and overrides | MaterialInstance → `list_parameters`, `get_*_parameter` |
| Blueprint graphs, nodes, pins, variables, functions, events, class defaults, parent | Blueprint → `list_graphs`, `get_graph`, `read_graph_dsl`, `find_nodes`, `get_node_infos`, `get_pin_value`, `list_variables`, `list_functions`, `list_events`, `get_default_object`, `get_parent` |
| Any object's properties and class | Object → `list_properties`, `get_properties`, `get_class`, `search_subclasses` |
| Finding assets, referencers, dependencies, asset tags | Asset → `find_assets`, `get_referencers`, `get_dependencies`, `get_asset_tags`, `get_asset_class` |
| Data tables, curve tables, meshes, textures, actors in the level | the matching EditorToolset toolset |
| Gameplay tags; StateTree structure; Niagara systems | GameplayTags, StateTree, Niagara toolsets |
| Live GAS state of an actor in PIE | GAS → attributes, active effects, granted abilities, active tags |
| Output log; viewport, asset or editor-window picture | Logs and EditorApp toolsets |

**Writes a toolset covers go to the built-in server too:**
- Blueprint graphs — create, delete and wire nodes, set pin values, add variables, functions and events, or write a
  whole graph from the graph DSL (`get_graph_dsl_docs` first), which compiles the Blueprint after writing.
- Material graphs — add, wire and delete expressions, connect material outputs, lay out and recompile; a bad pin
  name raises instead of silently leaving the input at its default.
- Material instances — parent and every parameter type.
- Object properties through reflection, class defaults and instanced subobjects included (`set_properties` with a JSON
  value; a subobject takes its class path).
- Data tables, curve tables, string tables, gameplay tags, actors, asset create/duplicate/move/save.
- PIE start and stop; discovering and running automation tests.

**`mcp-unreal` keeps:**
- every pipeline that already exists as a script in `AI/Python/` — rerunning a proven script beats rebuilding it as
  tool calls;
- anything no tool reaches: animation authoring, mesh building, sound import, Niagara stack internals, editor
  subsystems with only a Python binding;
- C++ editor-utility shims (`MCP_EditorUtility.md`);
- a long rebuild that would be dozens of round trips — one script on the game thread instead;
- the bridge's HTTP fallback when a client is down (`MCP_LiveCodingAndConnect.md`).

**Rules**
- One asset's multi-step edit stays on one server from start to finish.
- A built-in tool that errors or lacks an argument moves that step to `mcp-unreal`; never write around the gap with
  `execute_tool_script`, which cannot reach the `unreal` module.
- `execute_tool_script` is not one undo transaction: a script that fails midway leaves its earlier calls applied.
- Run tool calls one at a time — they all queue on the game thread.
- Save before a bulk change and after it; dirtied assets save the same way on either server (`CLAUDE.md` → Prerequisites).
- The live-coding compile tool and skill creation run only on the user's explicit request.

## Extending with project toolsets

- **C++**: a class deriving the Toolset Registry's toolset-definition base, whose static functions are marked
  `AICallable` (non-tool functions `AIIgnore`), registered with the registry from an editor module's startup —
  `GeoTrinityEditor` is the place. The class tooltip becomes the toolset description and each function's doc comment
  becomes its tool description. See `ToolsetRegistry/Public/ToolsetRegistry/ToolsetDefinition.h`.
- **Python**: a toolset class with `tool_call`-decorated static methods, registered through the registry's
  `Registration` helper from an `init_unreal.py`. `EditorToolset/Content/Python/editor_toolset/toolsets/` is the
  model.
- **Agent skills**: instruction assets the agent reads through the skill tools (list, get, create, update). Create or
  update one only on the user's explicit request.

## Filtering

Editor Preferences → Plugins → Toolset Registry holds allow and block lists for toolset/tool names and skill paths
(plain text matches as a case-insensitive substring, `/regex/` as a regex; block wins over allow). It also holds block
lists of classes and `Class.Property` pairs the property-setting tool must refuse.
