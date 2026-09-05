"""Create MPC_Camera and assign it on BP_GeoCam — the view state the parallax backdrop layers read.

Orthographic projection has no perspective divide, so distance buys a backdrop nothing: a layer one
metre behind the floor and one ten kilometres behind it translate on screen by exactly the same
amount when the camera moves, and AGeoGameCamera never rotates, so there is no sky-sphere spin to
fall back on either. Every bit of parallax has to be faked in the material against the camera's own
position, which is what this collection carries.

  CameraXY   (Vector, 0,0,0,0)  AGeoGameCamera's follow position this frame, in world XY.
  ZoomRatio  (Scalar, 1.0)      BaseOrthoWidth / CurrentOrthoWidth.

A layer then reads, with two 0..1 knobs of its own:

  UV = ( lerp(WorldXY, WorldXY - CameraXY, Parallax) * lerp(1, ZoomRatio, ZoomImmunity) ) / Tiling

Parallax 0 welds the layer to the floor, 1 pins it to the screen (infinitely far). ZoomImmunity
covers the other half of the illusion: the camera widens OrthoWidth from BaseOrthoWidth to
MaxOrthoWidth for couch coop, which shrinks a world-locked layer's features on screen and reads as
the background rushing at the players. A genuinely distant layer must not change size at all, so it
scales its UVs by ZoomRatio to cancel the widening exactly.

ZoomRatio DEFAULTS TO 1, NOT 0. The collection is read from the first frame a backdrop is drawn, and
AGeoGameCamera only publishes from its own Tick — a zero default multiplies the UVs to nothing for
that frame and every frame in a level with no camera at all (the main menu), so the layers sample a
single texel and the whole backdrop goes flat.

WORLD POSITION, NEVER TexCoord, on the reading side. The backdrop planes will carry arbitrary scales
and rotations like the floor pieces do, so a UV-space layer would run at a different star density on
every plane and break at each seam; absolute world position also puts the layer in the same space as
CameraXY, so neither side needs a transform. Same reasoning as make_background_lattice_material.py.

Idempotent: rebuilt in place every run, never deleted and recreated, because the layer materials take
hard references to it and deleting a referenced asset opens a modal nobody is there to answer.
"""
import unreal

PKG_PATH = "/Game/VFX/Generic/Materials"
MPC_NAME = "MPC_Camera"
CAMERA_XY_PARAM = "CameraXY"
ZOOM_RATIO_PARAM = "ZoomRatio"
CAMERA_BP_PATH = "/Game/Camera/BP_GeoCam"

at = unreal.AssetToolsHelpers.get_asset_tools()
ALWAYS = unreal.PropertyAccessChangeNotifyMode.ALWAYS
MPC_PATH = f"{PKG_PATH}/{MPC_NAME}"


def save(asset):
    """Write asset to disk, or fail loudly.

    save_asset defaults to only_if_is_dirty and reports the outcome only through a return value, so a
    freshly created asset whose package never got flagged dirty writes NOTHING and says nothing.
    """
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False), \
        f"{asset.get_path_name()}: would not save"


# --- MPC_Camera ----------------------------------------------------------------------------------
if unreal.EditorAssetLibrary.does_asset_exist(MPC_PATH):
    collection = unreal.EditorAssetLibrary.load_asset(MPC_PATH)
else:
    collection = at.create_asset(MPC_NAME, PKG_PATH, unreal.MaterialParameterCollection,
                                 unreal.MaterialParameterCollectionFactoryNew())

camera_xy = unreal.CollectionVectorParameter()
camera_xy.set_editor_property("parameter_name", CAMERA_XY_PARAM)
camera_xy.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 0.0))

zoom_ratio = unreal.CollectionScalarParameter()
zoom_ratio.set_editor_property("parameter_name", ZOOM_RATIO_PARAM)
zoom_ratio.set_editor_property("default_value", 1.0)

# Notify forced on: the collection assigns each parameter its Id in the post-change handler, and an
# entry that never gets one is invisible to every CollectionParameter node that names it.
collection.set_editor_property("vector_parameters", [camera_xy], ALWAYS)
collection.set_editor_property("scalar_parameters", [zoom_ratio], ALWAYS)

vectors = [str(n) for n in collection.get_vector_parameter_names()]
scalars = [str(n) for n in collection.get_scalar_parameter_names()]
assert vectors == [CAMERA_XY_PARAM], f"{MPC_NAME}: vector parameter would not register, got {vectors}"
assert scalars == [ZOOM_RATIO_PARAM], f"{MPC_NAME}: scalar parameter would not register, got {scalars}"
save(collection)
# BP_GeoCam is about to take a hard reference, so the collection has to be on disk before that — a
# reference to an unsaved package resolves to null on the next load.
assert unreal.EditorAssetLibrary.does_asset_exist(MPC_PATH), f"{MPC_NAME}: saved but is not on disk"

# --- BP_GeoCam -----------------------------------------------------------------------------------
# Set on the Blueprint every camera in the game derives from, not per placed actor: a level whose
# camera is missing the collection is a backdrop welded to the floor, and nothing in the level would
# say so.
camera_bp = unreal.EditorAssetLibrary.load_asset(CAMERA_BP_PATH)
assert camera_bp, f"{CAMERA_BP_PATH}: would not load"
cdo = unreal.get_default_object(camera_bp.generated_class())
cdo.set_editor_property("CameraParameters", collection)

assigned = cdo.get_editor_property("CameraParameters")
assert assigned and assigned.get_path_name() == collection.get_path_name(), \
    f"BP_GeoCam: CameraParameters would not take, got {assigned}"
save(camera_bp)

print(f"OK {MPC_PATH} -> vectors {vectors}, scalars {scalars}; assigned on {CAMERA_BP_PATH}")
