# MCP Settings & Level Configuration

Reading and writing project settings (`UDeveloperSettings` / config UPROPERTY) and per-level settings through
`execute_script`.

## Project settings

Load the settings class by its script path and read its CDO with the property getter. Container properties come
back as proxy objects — iterate them rather than calling `len`/`dict`.

The property setter updates the live value, but the settings CDO exposes no Python save method, so the change is
not persisted on its own: write the durable value into the relevant `Config/Default*.ini` section, which is the
source of truth.

The editor loads each config section only at startup, so an ini edit made while it runs is ignored — run the
`ReloadConfig` console command on the settings class to make it re-read the section. Per-user editor settings
(the saved-config hierarchy, e.g. play-session settings) are re-saved from live values when sessions change
state, so disk edits are overwritten and a reload does not apply them; automating one needs a C++ shim.

A config map is written as `Prop=((Key, Value))` with the enum or name literal as the key. Verify the live value
after a reload, since a malformed key silently maps to the wrong slot.

## Level settings

Load the level first and save it after — World Settings edits are not persisted on their own. World Settings is
the level's `AWorldSettings` instance read off the editor world; a `GameMode` override is a property on it, and
`PlayerControllerClass` a property on that GameMode's CDO. Class-valued properties take a Blueprint's generated
class.

Make "where this happens" a per-map decision by adding a UPROPERTY to the project's `AWorldSettings` subclass
and reading it at runtime.

See `AI/Python/Level/level_settings.py` for the load → set → save pattern.
