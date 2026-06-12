# MCP Settings — Config-Backed Developer Settings

Reading and writing project settings (`UDeveloperSettings` / config UPROPERTY) via `execute_script`.

---

## Reading

Load the settings class by its script path and read its CDO with `get_editor_property`. Container properties come back as proxy objects — iterate them rather than calling `len`/`dict`.

---

## Writing

`set_editor_property` on the CDO updates the live value, but the settings CDO exposes no Python save method, so the change is not persisted on its own.

Write the durable value to the relevant `Config/Default*.ini` section directly; that file is the source of truth.

---

## Applying an Ini Edit to a Running Editor

The editor loads each config section only at startup, so an ini edit made while it runs is ignored. Run the `ReloadConfig` console command on the settings class to make the running editor re-read the section.

Per-user editor settings (the saved-config hierarchy, e.g. play-session settings) are re-saved from live values when editor sessions change state, so disk edits get overwritten and a config reload does not apply them; automating such a setting needs a C++ shim.

---

## Map and Struct Properties

A config map is written as `Prop=((Key, Value))` in the ini, with the enum or name literal as the key. Verify the live value after a reload, since a malformed key silently maps to the wrong slot.
