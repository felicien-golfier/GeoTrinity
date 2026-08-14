"""Fill BP_GeoCam's Backdrop component with the plane mesh and the three MI_ParallaxStars layers.

Set on the Blueprint every camera in the game derives from, not per placed actor: a level whose camera
has no layers is a level with no sky, and nothing in it would say so.

Layers is ordered farthest first — entry 0 is the one with the highest Parallax, drawn behind the rest.

Requires UGeoBackdropComponent to exist, so the project has to be built first.
"""
import unreal

CAMERA_BP_PATH = "/Game/Camera/BP_GeoCam"
PLANE_MESH_PATH = "/Engine/BasicShapes/Plane"
LAYER_PATHS = (
    "/Game/VFX/Generic/Materials/MI_ParallaxStars_Far",
    "/Game/VFX/Generic/Materials/MI_ParallaxStars_Mid",
    "/Game/VFX/Generic/Materials/MI_ParallaxStars_Near",
)

camera_bp = unreal.EditorAssetLibrary.load_asset(CAMERA_BP_PATH)
assert camera_bp, f"{CAMERA_BP_PATH}: would not load"
backdrop = unreal.get_default_object(camera_bp.generated_class()).get_editor_property("Backdrop")
assert backdrop, "BP_GeoCam has no Backdrop component — build the project first"

plane = unreal.EditorAssetLibrary.load_asset(PLANE_MESH_PATH)
assert plane, f"{PLANE_MESH_PATH}: would not load"
layers = []
for path in LAYER_PATHS:
    layer = unreal.EditorAssetLibrary.load_asset(path)
    assert layer, f"{path}: would not load — run make_parallax_stars_material.py first"
    layers.append(layer)

backdrop.set_editor_property("LayerMesh", plane)
backdrop.set_editor_property("Layers", layers)

assert backdrop.get_editor_property("LayerMesh") == plane, "BP_GeoCam: LayerMesh would not take"
assert len(backdrop.get_editor_property("Layers")) == len(layers), "BP_GeoCam: Layers would not take"
assert unreal.EditorAssetLibrary.save_loaded_asset(camera_bp, only_if_is_dirty=False), \
    f"{CAMERA_BP_PATH}: would not save"

unreal.log(f"BACKDROP::{CAMERA_BP_PATH} -> {[str(m.get_name()) for m in layers]}")
