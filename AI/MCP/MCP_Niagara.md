# MCP Niagara Stack Editing

Editing an emitter's module stack — adding modules, switching shapes, setting input values, nesting dynamic
inputs. For renderers, materials and asset duplication see `AI/VFX.md`.

## Why a C++ shim

The stack view models only exist while the asset editor window is open, and the Python bindings refuse the
properties that would replace them: a system's emitter handles, a script source's node graph and a script's
rapid-iteration parameter store are all protected. Every stack edit therefore goes through the editor utility
shim; see `MCP_EditorUtility.md` for the pattern.

| Class | Header | What it does |
|---|---|---|
| `UGeoNiagaraBuilderUtil` | `Source/GeoTrinityEditor/Public/Tool/GeoNiagaraBuilderUtil.h` | Add an emitter, add/toggle a module, set static switches, set input values, nest dynamic inputs, set user parameters, dump a stage |

## Addressing

Modules and dynamic inputs are the same kind of node and are addressed identically, by the function name they
carry in their rapid-iteration constants (`Constants.<Emitter>.<FunctionName>.<InputName>`). Nesting needs no
special call: the name returned when a dynamic input is attached is itself a valid target, to any depth.

Each node gets a unique function name, so constants stay flat rather than nesting with the graph.

## Inputs are not pins

A function call node carries pins only for its static switches; value inputs are not pins at all. A value
input's name and type are only readable from the rapid-iteration constant the compiler emits for it, so a
newly added function must compile once before any of its values can be set. Dump a stage first — the dump
lists switches with their current values and value inputs with their types and component counts.

A constant only exists for an input the stack actually overrides, so the dump reports what a stack sets, never
a module's full input surface: an input still on its script default, and one fed by a link or a dynamic input,
are both absent. Read the remaining names out of the module asset itself — its input names are stored as plain
strings under a `Module.` prefix, and a name carrying no such prefix is a static switch.

Only enum switches are addressable. A bool static switch is invisible to the dump and refused by the setter,
so a module whose behaviour hangs on one can only be used in whichever branch it already sits.

A dynamic input attaches only where the stack holds a plain value. An input a template already feeds therefore
cannot be rewired: add a second instance of the module, disable the first, and build on the fresh one, whose
inputs all start as plain values.

An input's component count follows the module, not the name — the same name is a single float on one module
and a vector on another — and a write whose count does not match is refused. Dump the stage before writing.

Setting a static switch changes which inputs the branch exposes, so compile after it before addressing them.
A switch is a pin and is addressable the moment its module is added, so switches and module additions belong
in the same phase, ahead of the compile that makes any value addressable.

A dynamic input script may drive one of its own inputs from the graph, which leaves that input unaddressable
and makes its siblings the only controls over it.

## Module order within a stage

A module's position in the stack decides whether the stage's own solver reads it or overwrites it. Insert a
force above the solver that integrates it, and anything that writes a position below the solver, which
otherwise recomputes that position from velocity and discards the write. A module that re-derives a whole
shape each frame has to sit above anything displacing that shape, or the re-derivation wins.

## Modules a template does not carry

Emitter templates carry different modules, so a stack edit written for one is not portable to another —
a ribbon template has no burst to disable, and a sprite template has no beam. Addressing a module a template
lacks fires the shim's ensure, which halts the editor's game thread outright while a debugger is attached.
An ensure fires once per call site per editor session, so resuming lets the rest of the run complete and the
remaining calls of that kind return false instead of breaking again.

## Value encoding

Values are written as a comma separated component list. Bools, ints and enums occupy the same four bytes a
float does, so component counts are uniform across types. Bool true is the VM's own bitmask, not one. An enum
component accepts either its integer or an entry's display name.

Static switch pin defaults store the enum *entry* name, not the display name; resolve display names through
the enum rather than writing them onto a pin. A user-defined enum's display names are readable over the
Remote Control property API from its display-name map.

## Emitter handles

Adding or removing an emitter handle is only half the operation: the system's own spawn/update graph has to be
rebuilt around the new handle list and its overview graph resynchronised. The ops route does neither, so an
emitter added that way is never driven — the system completes on its first tick and nothing ever spawns — and
opening that system in the asset editor asserts on the missing overview node. Add through the builder utility,
which takes the editor's own path and repairs the whole system, so handles removed over the route only need one
add after them. An added emitter takes its handle name from the source asset, the only way to name it.

## Unexported symbols

Four stack utilities are declared without the editor module's API macro and cannot be linked against:
remove-module-from-stack, ordered-module-nodes, find-static-switch-input-pin, and the system-emitter-node
rebuild. Disable a module instead of removing it, filter graph nodes by their output node's usage instead of
asking for ordered nodes, and match switch pins by iterating a node's pins. The overview-graph resynchronise
next to that rebuild *is* exported, so only the rebuild itself blocks a shim-side add-emitter.

## Reading current values

The dump reports types but not the value of a value input. The Remote Control property API does reach the
rapid-iteration store, returning a name-to-offset table plus the raw byte array; component sizes come from the
gap to the next offset, since the dump carries no explicit size. Protected properties stay refused unless the
Remote Control ignore-protected setting is enabled.

Fetch the store from outside the editor — a synchronous self-request blocks the editor's own game thread.

## Scripts

| Task | Script |
|---|---|
| Duplicate a system, add a module with switches, set its inputs, nest dynamic inputs, dump a stage | `AI/Python/niagara_stack_edit.py` |
| Decode a rapid-iteration store fetched from the Remote Control property API | `AI/Python/decode_niagara_parameters.py` |
