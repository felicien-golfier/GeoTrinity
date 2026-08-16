# Builds the Star boss one-shot pike burst: fire the apexes outward in all directions, then retract, once.
#
# The Star's attack is pure scale — Root 1.00->1.50 and mid 1.00->3.17, where mid is what drives the apexes
# outward through skinning. apexes_inside / apexes_outside / up / joint* are vestigial and never move.
# Rather than invent values, this retimes the artist's own curves: SK_Star_Sequence_Start compressed into a
# fast burst, then SK_Star_Sequence_End at its authored speed. Start ends exactly where End begins, so the
# splice is continuous.
#
# Produces:
#   SK_Star_Sequence_PikeBurst  — 1.0s AnimSequence (pure Python)
#   SK_Star_PikeBurst_Montage   — Start -> Fire -> End, no loop (needs UGeoAnimBuilderUtil compiled)
import unreal

APE = unreal.AnimPoseExtensions
PKG = "/Game/Characters/Anim/Star"
SEQ_NAME = "SK_Star_Sequence_PikeBurst"
MONTAGE_NAME = "SK_Star_PikeBurst_Montage"
SEQ_PATH = "{}/{}".format(PKG, SEQ_NAME)
MONTAGE_PATH = "{}/{}".format(PKG, MONTAGE_NAME)

FPS = 30
BURST_FRAMES = 14    # 0.467s — the 3.03s windup compressed into a snap
RETRACT_FRAMES = 16  # 0.533s — End at its authored speed
FRAMES = BURST_FRAMES + RETRACT_FRAMES

# Section starts, matching the motion: launch, full extension, retract.
# Pattern.cpp jumps Start->Fire on activation and ->End when the pattern finishes.
SECTIONS = [("Start", 0.0, "Fire"), ("Fire", 0.2, "End"), ("End", 0.5, "None")]

SKELETON = unreal.load_asset("/Game/Characters/Meshes/Star/SK_Star")
START = unreal.load_asset(PKG + "/SK_Star_Sequence_Start")
END = unreal.load_asset(PKG + "/SK_Star_Sequence_End")
OPTS = unreal.AnimPoseEvaluationOptions()
start_len = START.get_editor_property("sequence_length")
end_len = END.get_editor_property("sequence_length")


def source_pose(frame):
    """Frame -> pose from the artist's Start (compressed) or End (1:1)."""
    if frame <= BURST_FRAMES:
        return APE.get_anim_pose_at_time(START, start_len * frame / float(BURST_FRAMES), OPTS)
    return APE.get_anim_pose_at_time(END, end_len * (frame - BURST_FRAMES) / float(RETRACT_FRAMES), OPTS)


def get_or_create(path, name, asset_class, factory):
    # Never delete-then-recreate: delete_asset on a loaded animation asset trips ForceDeleteObjects'
    # package-unload ensure and leaves the package unloadable.
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, PKG, asset_class, factory)


def build_sequence():
    seq_factory = unreal.AnimSequenceFactory()
    seq_factory.set_editor_property("target_skeleton", SKELETON)
    seq = get_or_create(SEQ_PATH, SEQ_NAME, unreal.AnimSequence, seq_factory)

    bones = [str(b) for b in APE.get_bone_names(APE.get_reference_pose(SKELETON))]
    # Extract per frame, never caching AnimPose objects: they alias one buffer, so a list of them
    # collapses to whichever was evaluated last.
    keys = {b: ([], [], []) for b in bones}
    for f in range(FRAMES + 1):
        pose = source_pose(f)
        for bone in bones:
            tr = APE.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.LOCAL)
            pos, rot, scl = keys[bone]
            pos.append(unreal.Vector(tr.translation.x, tr.translation.y, tr.translation.z))
            rot.append(tr.rotation.rotator().quaternion())
            scl.append(unreal.Vector(tr.scale3d.x, tr.scale3d.y, tr.scale3d.z))

    ctrl = seq.get_editor_property("controller")
    ctrl.open_bracket("Build pike burst")
    ctrl.set_frame_rate(unreal.FrameRate(FPS, 1))
    ctrl.set_number_of_frames(unreal.FrameNumber(FRAMES))
    for bone in bones:
        pos, rot, scl = keys[bone]
        # The track must exist first: set_bone_track_keys returns True but silently no-ops when the FK
        # control rig section has no control for that bone. add_bone_curve returns False for a track that
        # already exists, so its result is not an error signal.
        ctrl.add_bone_curve(bone)
        ctrl.set_bone_track_keys(bone, pos, rot, scl)
    ctrl.close_bracket()
    unreal.EditorAssetLibrary.save_asset(SEQ_PATH)
    return seq


def build_montage(seq):
    if not hasattr(unreal, "GeoAnimBuilderUtil"):
        print("SKIPPED montage: UGeoAnimBuilderUtil not compiled yet — build the editor target, then re-run.")
        return None

    montage_factory = unreal.AnimMontageFactory()
    montage_factory.set_editor_property("target_skeleton", SKELETON)
    montage = get_or_create(MONTAGE_PATH, MONTAGE_NAME, unreal.AnimMontage, montage_factory)

    util = unreal.get_default_object(unreal.GeoAnimBuilderUtil)
    util.set_montage_slot_segment(montage, seq, "DefaultSlot")
    util.set_montage_sections(montage, [unreal.Name(n) for n, _, _ in SECTIONS],
                              [t for _, t, _ in SECTIONS],
                              [unreal.Name(nx) for _, _, nx in SECTIONS])
    util.inspect_montage(montage)
    return montage


sequence = build_sequence()
build_montage(sequence)

length = sequence.get_editor_property("sequence_length")
print("{} len={:.3f} frames={}".format(SEQ_NAME, length, FRAMES))
for i in range(11):
    t = length * i / 10.0
    p = APE.get_anim_pose_at_time(sequence, t, OPTS)
    print("  t={:.2f} Root={:.2f} mid={:.2f}".format(
        t, APE.get_bone_pose(p, "Root", unreal.AnimPoseSpaces.LOCAL).scale3d.x,
        APE.get_bone_pose(p, "mid", unreal.AnimPoseSpaces.LOCAL).scale3d.x))
