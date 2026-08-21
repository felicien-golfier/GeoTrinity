"""Snapshots BP_GeoPlayableCharacter's ClassData map (soon-to-be-removed property) to a text file
before the C++ rebuild that moves FPlayerClassData into UPlayerClassDataAsset drops the property.
Run via execute_script, then read the output file — nothing is printed back through the tool."""
import unreal

OUT_PATH = "C:/GeoTrinity/AI/Python/class_data_snapshot.txt"

bp = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Playable/BP_GeoPlayableCharacter")
cdo = unreal.get_default_object(bp.generated_class())

class_data = cdo.get_editor_property("ClassData")

lines = []
for player_class, entry in class_data.items():
    lines.append(f"=== {player_class} ===")
    lines.append(entry.export_text())
    lines.append("")

with open(OUT_PATH, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))

unreal.log(f"ClassData snapshot written to {OUT_PATH}")
