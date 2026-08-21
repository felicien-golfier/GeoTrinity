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
| `UGeoNiagaraBuilderUtil` | `Source/GeoTrinityEditor/Public/Tool/GeoNiagaraBuilderUtil.h` | Add/toggle a module, set static switches, set input values, nest dynamic inputs, set user parameters, dump a stage |

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

## Value encoding

Values are written as a comma separated component list. Bools, ints and enums occupy the same four bytes a
float does, so component counts are uniform across types. Bool true is the VM's own bitmask, not one. An enum
component accepts either its integer or an entry's display name.

Static switch pin defaults store the enum *entry* name, not the display name; resolve display names through
the enum rather than writing them onto a pin. A user-defined enum's display names are readable over the
Remote Control property API from its display-name map.

## Unexported symbols

Three stack utilities are declared without the editor module's API macro and cannot be linked against:
remove-module-from-stack, ordered-module-nodes, and find-static-switch-input-pin. Disable a module instead of
removing it, filter graph nodes by their output node's usage instead of asking for ordered nodes, and match
switch pins by iterating a node's pins.

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
