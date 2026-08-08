"""
Records every authored deployable / pattern gameplay-cue tag + EGeoColor slot to Saved/DeployableCues.json.

Run this on the CURRENT build, BEFORE rebuilding with FGeoCueParam. The per-moment `<Moment>GameplayCueTag` /
`<Moment>GameplayCueColor` pairs (and the patterns' Init/Start/Direction cue tags) become fields of the new
struct, so the old properties disappear and the assets holding them can no longer be read back — this snapshot
is what apply_deployable_cues.py replays afterwards.
"""
import json

import unreal

OUTPUT_PATH = r"C:\GeoTrinity\Saved\DeployableCues.json"

# Old property name -> new FGeoCueParam property, per owner type.
DEPLOYABLE_MOMENTS = {
    "SpawnGameplayCue": "SpawnCue",
    "RecallGameplayCue": "RecallCue",
    "BlinkingGameplayCue": "BlinkingCue",
    "ExplodeGameplayCue": "ExplodeCue",
    "ExpireGameplayCue": "ExpireCue",
}
PATTERN_CUES = {"InitGameplayCueTag": "InitCue", "StartGameplayCueTag": "StartCue", "DirectionCue": "DirectionCue"}


def read(owner, name):
    """Returns the property value under `name`, or None when the owner has no such property."""
    try:
        return owner.get_editor_property(name)
    except Exception:
        return None


def tag_name(tag):
    """Returns the tag's name, or an empty string for an unset tag."""
    if tag is None:
        return ""
    name = str(tag.get_editor_property("TagName"))
    return "" if name == "None" else name


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

    cues = {}
    if isinstance(cdo, unreal.GeoDeployableBase):
        for old, new in DEPLOYABLE_MOMENTS.items():
            tag = tag_name(read(cdo, old + "Tag"))
            color = read(cdo, old + "Color")
            if tag:
                cues[new] = {"CueTag": tag, "Color": color.name if color else "NEUTRAL"}
    elif isinstance(cdo, unreal.Pattern):
        for old, new in PATTERN_CUES.items():
            tag = tag_name(read(cdo, old))
            if tag:
                cues[new] = {"CueTag": tag, "Color": "NEUTRAL"}

    if cues:
        records.append({"asset": str(asset_data.package_name), "cues": cues})

with open(OUTPUT_PATH, "w") as f:
    json.dump(records, f, indent=2)

unreal.log("Dumped cues of {} assets to {}".format(len(records), OUTPUT_PATH))
