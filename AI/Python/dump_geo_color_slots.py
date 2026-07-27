"""
Records the EGeoColor slot of every authored FGeoColorParam in /Game to Saved/GeoColorSlots.json.

Run this on the CURRENT build, BEFORE rebuilding with the merged EGeoColor. Enum properties serialize by
name, so once DirectDamage/DamageOverTime/DirectHeal/HealOverTime/BothHealAndDamageOverTime/Telegraph are
gone from the enum the assets holding them can no longer be read back — this snapshot is what
apply_geo_color_slots.py replays afterwards.
"""
import json

import unreal

OUTPUT_PATH = r"C:\GeoTrinity\Saved\GeoColorSlots.json"

# Every property an FGeoColorParam is reachable through. A leaf name means the param sits directly on the
# CDO; a (struct, [members]) pair means it sits inside that struct; a list-valued entry is a TArray.
DIRECT_PARAMS = ["BeamColor", "BeamInitColor", "AOEColor", "TelegraphColor", "GaugeFullColor"]
ARRAY_PARAMS = ["BuffColors"]
STRUCT_PARAMS = {"DefaultParams": ["HeadColor", "TrailColor"], "ProjectileParams": ["HeadColor", "TrailColor"]}


def read_param(owner, name):
    """Returns the GeoColorParam under `name`, or None when the owner has no such property."""
    try:
        value = owner.get_editor_property(name)
    except Exception:
        return None
    return value if isinstance(value, unreal.GeoColorParam) else None


def slot_name(param):
    return param.get_editor_property("Color").name


records = []
registry = unreal.AssetRegistryHelpers.get_asset_registry()

for asset_data in registry.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "Blueprint"), True):
    if not str(asset_data.package_name).startswith("/Game"):
        continue

    blueprint = asset_data.get_asset()
    generated_class = blueprint.generated_class() if blueprint else None
    if not generated_class:
        continue
    cdo = unreal.get_default_object(generated_class)

    entries = []
    for name in DIRECT_PARAMS:
        param = read_param(cdo, name)
        if param:
            entries.append({"path": [name], "slot": slot_name(param)})

    for name in ARRAY_PARAMS:
        try:
            array = cdo.get_editor_property(name)
        except Exception:
            continue
        for index, param in enumerate(array or []):
            if isinstance(param, unreal.GeoColorParam):
                entries.append({"path": [name, index], "slot": slot_name(param)})

    for struct_name, members in STRUCT_PARAMS.items():
        try:
            struct = cdo.get_editor_property(struct_name)
        except Exception:
            continue
        for member in members:
            param = read_param(struct, member)
            if param:
                entries.append({"path": [struct_name, member], "slot": slot_name(param)})

    if entries:
        records.append({"asset": str(asset_data.package_name), "params": entries})

with open(OUTPUT_PATH, "w") as f:
    json.dump(records, f, indent=2)

unreal.log("Dumped {} assets holding FGeoColorParam to {}".format(len(records), OUTPUT_PATH))
