"""
Replays Saved/GeoColorSlots.json onto the merged EGeoColor, then saves every asset it touched.

Run this AFTER rebuilding with the merged enum, on the snapshot dump_geo_color_slots.py took before it.
Only the merged/renamed slots are rewritten; every other slot already resolved by name on load and is left
alone. BeamInitColor / TelegraphColor no longer exist — a telegraph now renders in the color of the effect
it announces — so their dumped entries are dropped.
"""
import json
import traceback

import unreal

INPUT_PATH = r"C:\GeoTrinity\Saved\GeoColorSlots.json"
RESULT_PATH = r"C:\GeoTrinity\Saved\GeoColorRemapResult.txt"

# Dumped slot name (PyUnreal upper-snake) -> merged slot on the new enum.
SLOT_REMAP = {
    "DIRECT_DAMAGE": "DAMAGE",
    "DAMAGE_OVER_TIME": "DAMAGE",
    "DIRECT_HEAL": "HEAL",
    "HEAL_OVER_TIME": "HEAL",
    "BOTH_HEAL_AND_DAMAGE_OVER_TIME": "BOTH_HEAL_AND_DAMAGE",
}
REMOVED_PARAMS = {"BeamInitColor", "TelegraphColor"}


def set_slot(owner, name, new_slot, index=None):
    """Writes new_slot into the GeoColorParam under `name` (or `name[index]`), copy-back included —
    PyUnreal hands out struct and array values by value, so the mutated copy must be pushed back up."""
    value = owner.get_editor_property(name)
    param = value if index is None else value[index]
    param.set_editor_property("Color", getattr(unreal.GeoColor, new_slot))
    if index is not None:
        value[index] = param
    owner.set_editor_property(name, value)


report = []
try:
    with open(INPUT_PATH) as f:
        records = json.load(f)

    for record in records:
        entries = [e for e in record["params"] if e["slot"] in SLOT_REMAP and e["path"][0] not in REMOVED_PARAMS]
        if not entries:
            continue

        blueprint = unreal.load_asset(record["asset"])
        cdo = unreal.get_default_object(blueprint.generated_class())

        for entry in entries:
            path = entry["path"]
            new_slot = SLOT_REMAP[entry["slot"]]
            if len(path) == 1:
                set_slot(cdo, path[0], new_slot)
            elif isinstance(path[1], int):
                set_slot(cdo, path[0], new_slot, index=path[1])
            else:
                struct = cdo.get_editor_property(path[0])
                set_slot(struct, path[1], new_slot)
                cdo.set_editor_property(path[0], struct)
            report.append("{} {} {} -> {}".format(record["asset"], path, entry["slot"], new_slot))

        # Force the write: an array element edit leaves the package clean (PyUnreal returns TArray as a
        # write-through view, so the copy-back in set_slot is a no-op), and a dirty-only save skips it.
        saved = unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
        report.append("  saved={} {}".format(saved, record["asset"]))
except Exception:
    report.append(traceback.format_exc())

with open(RESULT_PATH, "w") as f:
    f.write("\n".join(report))
