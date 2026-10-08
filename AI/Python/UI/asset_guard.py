"""
The one rule every builder script follows: a script may overwrite a value only while it still holds what a script last
wrote there. A value changed by hand since — a font, a colour, an offset — is never overwritten: the write is skipped
and reported, so the change can be carried into the script or discussed with the user.

    asset_guard.write(asset_path, obj, "font", value)    # instead of obj.set_editor_property("font", value)
    asset_guard.report()                                  # at the end of the run: what was kept by hand, as a list

Each written value is recorded in asset_values.json (beside this file, committed) by object path and property. A
value with no record yet is written only when the object was created by this run (created()), still holds its class
default, already equals what the script writes, or equals a value an older script wrote (`earlier`); anything else is
treated as a hand change.
"""
import json
import os
import struct

_created = set()
_kept = []


def _project_folder():
    try:
        return os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
    except NameError:
        import unreal
        return os.path.normpath(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))


def _ledger_file():
    return os.path.join(_project_folder(), "AI", "Python", "UI", "asset_values.json")


def _read_ledger():
    path = _ledger_file()
    return json.load(open(path)) if os.path.exists(path) else {}


def _text(value):
    if hasattr(value, "export_text"):
        return value.export_text()
    if hasattr(value, "get_path_name"):
        return value.get_path_name()
    if isinstance(value, dict) or type(value).__name__ == "Map":
        return "{" + ",".join(sorted(f"{_text(key)}:{_text(item)}" for key, item in value.items())) + "}"
    if isinstance(value, (list, tuple)) or type(value).__name__ == "Array":
        return "[" + ",".join(_text(item) for item in value) + "]"
    if isinstance(value, float):
        # Properties hold 32-bit floats: compare 0.12 as the 0.11999999731779099 it is stored as.
        return repr(struct.unpack("f", struct.pack("f", value))[0])
    return repr(value)


def created(asset_path):
    """Marks an asset this run has just created: every value in it is the script's to write."""
    _created.add(asset_path)


def write(asset_path, obj, name, value, earlier=()):
    """Sets obj's property `name` to value unless it was changed by hand since a script last wrote it. Returns whether
    it wrote. `earlier`: values an older script wrote there before this ledger existed, which are a script's too."""
    import unreal
    ledger = _read_ledger()
    key = f"{obj.get_path_name()}:{name}"
    current, new = _text(obj.get_editor_property(name)), _text(value)
    last = ledger.get(key)
    if last is not None:
        owned = current in [last, new]
    else:
        default = _text(unreal.get_default_object(obj.get_class()).get_editor_property(name))
        owned = asset_path in _created or current in [new, default] + [_text(item) for item in earlier]
    if not owned:
        _kept.append(f"{key}: kept {current} (script: {new})")
        return False

    obj.set_editor_property(name, value)
    ledger[key] = _text(obj.get_editor_property(name))
    with open(_ledger_file(), "w", newline="\n") as handle:
        json.dump(ledger, handle, indent=1, sort_keys=True)
    return True


def hand_edits(asset_path):
    """Every value in asset_path changed by hand since a script wrote it, listed in report() too: what restructuring the
    asset would lose. A script empties a layout it is replacing only when this is empty."""
    import unreal
    changed = []
    for key, last in _read_ledger().items():
        object_path, name = key.rsplit(":", 1)
        obj = unreal.find_object(None, object_path) if object_path.startswith(asset_path + ".") else None
        if obj and _text(obj.get_editor_property(name)) != last:
            changed.append(f"{key}: {_text(obj.get_editor_property(name))} (script: {last})")
    _kept.extend(changed)
    return changed


def legacy_rebuild(asset_path):
    """For a script that still rebuilds whole widgets, which would wipe hand changes: loads the asset only when this run
    created it, else stops — convert the script to write() before running it on an existing asset."""
    import unreal
    if asset_path not in _created and unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        raise RuntimeError(f"{asset_path} exists and this script rebuilds whole widgets: convert it to "
                           f"asset_guard.write so hand changes survive, then re-run")
    return unreal.load_asset(asset_path)


def report():
    """Every value kept because it was changed by hand, then forgets them for the next run."""
    kept = list(_kept)
    _kept.clear()
    _created.clear()
    return kept
