# Testing the running game with the built-in MCP only

Driving a PIE session through `mcp-epic` tools alone (no Python server). Server setup: `MCP_UnrealBuiltin.md`. The Python-side view of the same session: `MCP_PIE.md`.

## Setup
- `mcp-epic` is disconnected by default. Whenever a task needs it, ask the user for access first (an editor serving, then `/mcp` reconnect); never assume it is connected, never fall back to `mcp-unreal`.
- The editor must serve before Claude Code connects; after an editor restart the user reconnects with `/mcp` again.
- Toolsets are plugins of the project: a toolset missing from `list_toolsets` is a plugin missing from the `.uproject`, not a server fault.
- Edit level actors in the editor world with PIE stopped, save by asset path; a PIE edit never reaches the asset.
- PIE net mode comes from the editor play settings default object: set it by property, in memory, and restore it after. The game's solo flow is a listen host, so test in listen-server mode: a client-only mode runs the server without a local player, and server-side HUD actions (Cat lines) then warn "no local player HUD".

## Cheap and reliable
| To test | How |
|---|---|
| A room or step in isolation | Teleport the pawn: set the transform of the server-world pawn (found by class); PIE start also takes a spawn transform |
| Trigger volumes, arena tags, respawn, death rules | Teleport onto / over them; a fall past the kill height is a real death |
| Encounter step flow | Raise the encounter log category to Verbose, read it with a regex; the current step is not a property |
| Player state | GAS inspector tools: granted abilities, attributes, active effects, tags |
| Keyboard input (dash, menu keys, Esc) | Slate key press: reaches the game viewport and the menus, even while paused |
| Several reads at once | The programmatic batch tool: one round trip, helpers at the top, no sleep |
| What the player sees | Editor-image capture, decoded locally (below) |

## Limits
- Mouse buttons cannot be sent: the game viewport has no Slate ref. Held keys cannot be sent either (press and release only).
- A tool call runs on the game thread, so waiting happens between calls: poll the log, never sleep inside a script.
- Log timestamps are UTC.
- A step that waits on a timed Cat line needs real seconds; read the line time from the build script rather than guessing.
- Encounters are chained (a room's end starts the next), so test in order and satisfy each room's completion by teleport or key press.

## Walking the tutorial (2026-10-19)
- Server down (`ECONNREFUSED`): the user runs `ModelContextProtocol.StartServer` in the editor console, then `/mcp`; Remote Control refuses console commands.
- `execute_tool_script` has no `time`: poll between calls. Its tool names are the full dotted ones.
- PIE actor paths: `/Game/Maps/UEDPIE_0_<Map>.<Map>:PersistentLevel.<Name>`. `find_actors` with a class path lists PIE actors.
- A room passed = its encounter's `rule N fired` / `enter step` log lines (N = rule index in the step) plus the pads' `bOpen`.
- Teleport and kill by setting an actor's transform (a real pad teleport, a death below `KillZ`).
- An unlock opens the ability page, which pauses the world: `SlateInspectorToolset` `PressKey` `Escape` closes it. With no page open the same key opens the pause menu, which pauses the world (zones stop healing): press it once per page, and a stalled value means the menu is open.
- Test a room in isolation: PIE stopped, `bStartOnBeginPlay` true on its encounter in the editor world; restore it, never save the map.
- Mouse buttons go as keys: `PressKey` `RightMouseButton` (deployable) / `LeftMouseButton` (basic) / `LeftShift` (Special) fire the real ability (checked: cooldown effect + deployable spawned). Press and release only: a tap, never a hold.
- The cursor rewrites the pawn's yaw every tick, so aim is set on the game, not on the pawn: on the pawn's `GeoInputComponent` set `bTestAimActive` true and `TestAimWorld` to the world point (`set_properties`, `Input/CLAUDE.md`). A tap deploys 140 cm from the pawn toward that point: stand within that distance of the target first. Set it back to false when done.

## Pictures
Image results are base64 in the tool result; over the size limit they land in a file. Decode it with `AI/Python/Runtime/decode_tool_image.py` and Read the PNG; capture only to judge a look, since every picture is a large result.
