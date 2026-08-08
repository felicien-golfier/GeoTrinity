"""
Replays Saved/DeployableCues.json into the new FGeoCueParam properties, then saves every asset it touched.

Run this AFTER rebuilding with FGeoCueParam, on the snapshot dump_deployable_cues.py took before it. The old
`<Moment>GameplayCueTag` / `<Moment>GameplayCueColor` pairs are gone, so each dumped moment is rebuilt as one
struct (SoundTag stays unset — nothing authored it before).
"""
import json
import traceback

import unreal

INPUT_PATH = r"C:\GeoTrinity\Saved\DeployableCues.json"
RESULT_PATH = r"C:\GeoTrinity\Saved\DeployableCuesResult.txt"


def cpp_enum_name(python_name):
    """PyUnreal hands out enum names upper-snake (ALLY_DAMAGE); ImportText wants the C++ one (AllyDamage)."""
    return "".join(part.capitalize() for part in python_name.split("_"))


def make_cue(cue):
    """Builds an FGeoCueParam from a dumped {CueTag, Color} entry. EditDefaultsOnly struct fields reject the
    property setter, so the struct is built by importing its exported text in one go."""
    param = unreal.GeoCueParam()
    param.import_text(
        '(CueTag=(TagName="{}"),Color={})'.format(cue["CueTag"], cpp_enum_name(cue["Color"]))
    )
    return param


report = []
try:
    with open(INPUT_PATH) as f:
        records = json.load(f)

    for record in records:
        blueprint = unreal.load_asset(record["asset"])
        cdo = unreal.get_default_object(blueprint.generated_class())

        for name, cue in record["cues"].items():
            cdo.set_editor_property(name, make_cue(cue))
            report.append("{} {} = {} / {}".format(record["asset"], name, cue["CueTag"], cue["Color"]))

        saved = unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
        report.append("  saved={} {}".format(saved, record["asset"]))
except Exception:
    report.append(traceback.format_exc())

with open(RESULT_PATH, "w") as f:
    f.write("\n".join(report))
