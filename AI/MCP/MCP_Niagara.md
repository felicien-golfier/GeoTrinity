# MCP Niagara Stack Editing

Editing an emitter's module stack — adding modules, switching shapes, setting input values, nesting dynamic
inputs. For renderers, materials, asset paths and the systems themselves see `AI/VFX.md`.

## Why a C++ shim

The stack view models only exist while the asset editor window is open, and the Python bindings refuse the
properties that would replace them: a system's emitter handles, a script source's node graph and a script's
rapid-iteration parameter store are all protected. Every stack edit goes through the editor utility shim.

| Class | Header | What it does |
|---|---|---|
| `UGeoNiagaraBuilderUtil` | `Source/GeoTrinityEditor/Public/Tool/GeoNiagaraBuilderUtil.h` | Add an emitter or a renderer, write an emitter property, add/toggle a module, set static switches, set input values, nest dynamic inputs, set user parameters, dump a stage |

The ops route (`/api/niagara/ops`, `NiagaraRoutes.cpp`) accepts only `spawn_system`, `set_parameter`,
`get_system_info`, `add_emitter`, `remove_emitter`, `activate` and `deactivate`; any other name is rejected. It
reaches no module stack. `get_system_info` lists emitters and user parameters through `GetUserParameters()`, so
it skips stale redirect aliases.

`NiagaraEditor` must stay in `PrivateDependencyModuleNames` unconditionally — moving it inside
`bBuildWithEditorOnlyData` causes `C1083` on `NiagaraScriptSource.h`.

## Addressing

Modules and dynamic inputs are the same kind of node and are addressed identically, by the function name in
their rapid-iteration constants (`Constants.<Emitter>.<FunctionName>.<InputName>`). Nesting needs no special
call: the name returned when a dynamic input is attached is itself a valid target, to any depth. Each node gets
a unique function name, so constants stay flat rather than nesting with the graph.

### Inputs are not pins

A function call node carries pins only for its static switches; value inputs are not pins at all. A value
input's name and type are readable only from the rapid-iteration constant the compiler emits for it, so a newly
added function must compile once before any of its values can be set. Dump a stage first — the dump lists
switches with their current values and value inputs with their types and component counts.

- A constant exists only for an input the stack actually overrides, so the dump reports what a stack sets, never
  a module's full input surface: an input on its script default and one fed by a link are both absent. Read the
  remaining names from the module asset itself, where input names are stored as plain strings under a `Module.`
  prefix and a name without that prefix is a static switch — a text search over the asset file reaches them with
  the editor closed.
- A constant left over from a branch the stack has since switched away from still appears, so confirm an input
  belongs to the current branch by writing it rather than by reading the dump.
- Only enum switches are addressable. A bool static switch is invisible to the dump and refused by the setter,
  so a module whose behaviour hangs on one can only be used in the branch it already sits in.
- A dynamic input attaches only where the stack holds a plain value. An input a template already feeds cannot be
  rewired: add a second instance of the module, disable the first, and build on the fresh one.
- An input's component count follows the module, not the name — the same name is a float on one module and a
  vector on another — and a write whose count does not match is refused.
- Setting a static switch changes which inputs the branch exposes, so switches and module additions belong in
  the same phase, ahead of the compile that makes any value addressable. A switch is a pin and is addressable
  the moment its module is added.
- A dynamic input script may drive one of its own inputs from the graph, which leaves that input unaddressable
  and makes its siblings the only controls over it.

### Value encoding

Values are written as a comma separated component list. Bools, ints and enums occupy the same four bytes a float
does, so component counts are uniform across types. Bool true is the VM's own bitmask, not one. An enum
component accepts either its integer or an entry's display name.

Static switch pin defaults store the enum *entry* name, not the display name, so resolve display names through
the enum rather than writing them onto a pin. A user-defined enum's display names are readable over the Remote
Control property API from its display-name map, and the enum object lists none of them to Python; the switch
setter reports whether a display name matched, so a short list of candidates can be probed against it.

### Reading current values

The dump reports an input's type but not its value. The Remote Control property API does reach the
rapid-iteration store, returning a name-to-offset table plus the raw byte array, with component sizes coming
from the gap to the next offset. Protected properties stay refused unless the ignore-protected setting is
enabled. Fetch the store from outside the editor — a synchronous self-request blocks the game thread.

## Module order within a stage

A module's position decides whether the stage's solver reads it or overwrites it. Insert a force above the
solver that integrates it, and anything writing a position below the solver, which otherwise recomputes that
position from velocity and discards the write. A module re-deriving a whole shape each frame has to sit above
anything displacing that shape, or the re-derivation wins.

## Modules a template does not carry

Emitter templates carry different modules, so a stack edit written for one is not portable: a ribbon template
has no burst to disable, a sprite template no beam. Addressing a module a template lacks fires the shim's
ensure, which halts the game thread outright while a debugger is attached. An ensure fires once per call site
per session, so resuming lets the rest of the run complete and the remaining calls of that kind return false.

## Emitter handles

Adding or removing a handle is only half the operation: the system's spawn/update graph has to be rebuilt around
the new handle list and its overview graph resynchronised. The ops route does neither, so an emitter added that
way is never driven — the system completes on its first tick — and opening it in the asset editor asserts on the
missing overview node. Add through the builder utility, which takes the editor's own path and repairs the whole
system, so handles removed over the route need only one add after them. An added emitter takes its handle name
from the source asset, the only way to name it.

Between a removal and that add the system is inconsistent and anything playing it goes on ticking: a component
in the open level brings the editor down. Delete the actors playing a system before stripping it, strip and
refill one system per call, and never leave one empty across a pause.

A handle name is not the name of the emitter object inside the system, so a renderer or property resolved by
object name there can belong to a different copy — set renderer properties on the source emitter before adding
it, where the name is unambiguous. Stack edits are unaffected, since those address the handle.

## Properties Outside the Stack

An emitter property that has moved into versioned data is refused by both the Python bindings, which report it
deprecated for reading as much as for writing, and the property API, which reports the owning struct private.
The shim reaches one by reflection on that struct and lets the property parse its own value from text, so local
space, persistent IDs, determinism and fixed bounds all arrive through the same call.

A renderer arrives the same way, created from a class path, because the renderer list shares that versioned data
and Python wraps no renderer class but the sprite one. Every property on it is plain Python afterwards: an
object whose class has no binding comes back wrapped as its nearest exposed base and still resolves properties
against the real class. An enum property is the exception — it has no conversion without a binding.

Write both on the emitter asset, before it is copied into a system.

## Unexported symbols

Six utilities are declared without their module's API macro and cannot be linked against: remove-module-from-
stack, ordered-module-nodes, find-static-switch-input-pin, the system-emitter-node rebuild, remove-emitters-by-
handle-id, and the emitter's own versioned-property change notify. Disable a module instead of removing it,
filter graph nodes by their output node's usage instead of asking for ordered nodes, match switch pins by
iterating a node's pins, and strip emitters over the ops route rather than from the shim. The overview-graph
resynchronise next to that rebuild *is* exported, so only the rebuild itself blocks a shim-side add-emitter.

After writing a versioned property by reflection, mark the graph source out of sync and call the object's
generic post-edit-change, which reaches the emitter's own handler and updates its change id; without it the
compiled result stays stale.

## Scripts

| Task | Script |
|---|---|
| Duplicate a system, add a module with switches, set its inputs, nest dynamic inputs, dump a stage, probe a switch's accepted entries | `AI/Python/Niagara/niagara_stack_edit.py` |
| Decode a rapid-iteration store fetched from the Remote Control property API | `AI/Python/Niagara/decode_niagara_parameters.py` |
