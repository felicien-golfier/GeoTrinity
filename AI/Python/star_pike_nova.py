# Builds SK_Star_Sequence_PikeNova — the Star boss firing its spikes one at a time around itself — plus
# the montage that plays it, wired into the pillar-spawn pattern.
#
# Rig facts this is built on, read out of the mesh's skin weights and the shipped attack animation:
#   - Only five bones skin anything: apexes_inside (26 verts), apexes_outside (26), up (19), Root (17) and
#     mid (16). Every joint* bone influences nothing.
#   - apexes_outside drives the eight tips and apexes_inside the valleys between them; mid is a minor
#     11%-of-the-mesh influence, not the apex owner. Root is the whole star.
#   - There is one bone per ring and no per-apex bone, so radial scale extends all eight tips together.
#     Translation is the only per-direction control: offsetting a ring pushes the tips on that side out
#     and sinks the opposite ones below the valley radius, where they vanish into the body. Sweeping that
#     offset direction around the star is what fires the spikes one at a time, each retracting as the
#     next leaves.
#   - The eight tips sit at exactly 0/45/…/315 degrees, radius 70.71, with the valley ring at 50. The
#     sweep therefore lines up with a tip eight times per revolution, and the revolution is timed so those
#     eight beats land on whole frames — off-frame beats read as flicker rather than as eight even pops.
#   - apexes_outside and apexes_inside only ever yaw in the shipped animation, at different rates. That
#     differential twist is kept here as an accent.
#   - The star is flat in XY, so Z scale stays at rest; only X/Y read under the orthographic camera.
import math

import unreal

APE = unreal.AnimPoseExtensions
PKG = "/Game/Characters/Anim/Star"
SEQ_NAME = "SK_Star_Sequence_PikeNova"
MONTAGE_NAME = "SK_Star_PikeNova_Montage"
SEQ_PATH = "{}/{}".format(PKG, SEQ_NAME)
MONTAGE_PATH = "{}/{}".format(PKG, MONTAGE_NAME)
PATTERN_BP = "/Game/AbilitySystem/Abilities/Enemy/SpawnPillar/BP_SpawnPillarPattern"

FPS = 30
FRAMES = 52  # 1.733s
TURN_START = 5  # anticipation bottoms out and the sweep begins
TURN_END = 45  # one full revolution, 40 frames — eight tips at a whole 5 frames each

# The lean has to beat the valley radius, or the trailing tips never drop out of the silhouette and every
# spike reads as permanently out: at the held extension a tip sits at 70.71 * TIP_BASE, so a lean larger
# than that minus 50 sinks the trailing ones into the body.
TIP_OFFSET = 52.0  # how far the tip ring leans towards the spike currently firing
TIP_BASE = 1.10  # tip extension held all through the sweep
TIP_PEAK = 1.55  # tip extension as the sweep lines up with a tip, eight times per revolution
TIP_SWIRL = 60.0  # degrees the tip ring turns across the revolution
VALLEY_OFFSET = 14.0  # the valley ring follows the lean, but far less, so the rings decouple
VALLEY_SWIRL = 35.0  # and lags the tips, reproducing the shipped animation's twist
MID_SWELL = 1.35
ROOT_SWELL = 1.10
ROOT_OFFSET = 6.0  # the body leans away from whichever spike is out

# frame -> how far into the burst the star is; negative is the anticipation squeeze.
ENVELOPE = [(0, 0.0), (TURN_START, -0.35), (10, 1.0), (40, 1.0), (TURN_END, 0.55), (FRAMES, 0.0)]

# Sections cut on the beats above: Start is the anticipation, which UPattern stretches to the ability's
# FireDelay, so the first spike must leave in Fire rather than at the tail of the wind-up.
SECTION_NAMES = ["Start", "Fire", "End"]
SECTION_STARTS = [0.0, TURN_START / float(FPS), TURN_END / float(FPS)]
SECTION_NEXT = ["Fire", "End", "None"]

SKELETON = unreal.load_asset("/Game/Characters/Meshes/Star/SK_Star")


def smoothstep(a, b, t):
    t = max(0.0, min(1.0, t))
    return a + (b - a) * t * t * (3.0 - 2.0 * t)


def envelope(frame):
    for i in range(len(ENVELOPE) - 1):
        f0, v0 = ENVELOPE[i]
        f1, v1 = ENVELOPE[i + 1]
        if f0 <= frame <= f1:
            return smoothstep(v0, v1, 0.0 if f1 == f0 else float(frame - f0) / float(f1 - f0))
    return ENVELOPE[-1][1]


def turn_ratio(frame):
    """0..1 across the revolution, linear so the sweep turns at a constant speed."""
    return max(0.0, min(1.0, float(frame - TURN_START) / float(TURN_END - TURN_START)))


def sample(frame, bone, rest_local):
    """Frame and bone -> the local (translation, rotation, scale) to key."""
    extension = envelope(frame)
    ratio = turn_ratio(frame)
    angle = 2.0 * math.pi * ratio
    translation = unreal.Vector(rest_local.translation.x, rest_local.translation.y, rest_local.translation.z)
    scale = unreal.Vector(rest_local.scale3d.x, rest_local.scale3d.y, rest_local.scale3d.z)
    rotator = rest_local.rotation.rotator()

    if bone == "apexes_outside":
        aligned = 0.5 + 0.5 * math.cos(8.0 * angle)
        factor = 1.0 + extension * ((TIP_BASE - 1.0) + (TIP_PEAK - TIP_BASE) * aligned)
        translation.x += extension * TIP_OFFSET * math.cos(angle)
        translation.y += extension * TIP_OFFSET * math.sin(angle)
        scale.x *= factor
        scale.y *= factor
        rotator.yaw += extension * TIP_SWIRL * ratio
    elif bone == "apexes_inside":
        translation.x += extension * VALLEY_OFFSET * math.cos(angle)
        translation.y += extension * VALLEY_OFFSET * math.sin(angle)
        rotator.yaw += extension * VALLEY_SWIRL * ratio
    elif bone == "mid":
        factor = 1.0 + extension * (MID_SWELL - 1.0)
        scale.x *= factor
        scale.y *= factor
    elif bone == "Root":
        factor = 1.0 + extension * (ROOT_SWELL - 1.0)
        translation.x -= extension * ROOT_OFFSET * math.cos(angle)
        translation.y -= extension * ROOT_OFFSET * math.sin(angle)
        scale.x *= factor
        scale.y *= factor

    return translation, rotator.quaternion(), scale


def get_or_create(path, name, asset_class, factory):
    # Never delete-then-recreate: delete_asset on a loaded animation asset trips ForceDeleteObjects'
    # package-unload ensure and leaves the package permanently unloadable.
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, PKG, asset_class, factory)


def build_sequence():
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", SKELETON)
    seq = get_or_create(SEQ_PATH, SEQ_NAME, unreal.AnimSequence, factory)

    ref = APE.get_reference_pose(SKELETON)
    rest = {}
    for bone in APE.get_bone_names(ref):
        rest[str(bone)] = APE.get_bone_pose(ref, str(bone), unreal.AnimPoseSpaces.LOCAL)

    controller = seq.get_editor_property("controller")
    controller.open_bracket("Build pike nova")
    controller.set_frame_rate(unreal.FrameRate(FPS, 1))
    controller.set_number_of_frames(unreal.FrameNumber(FRAMES))
    for bone, rest_local in rest.items():
        positions, rotations, scales = [], [], []
        for frame in range(FRAMES + 1):  # a sequence holds one more key than its frame count
            translation, rotation, scale = sample(frame, bone, rest_local)
            positions.append(translation)
            rotations.append(rotation)
            scales.append(scale)
        # The track must exist first: set_bone_track_keys reports success but writes nothing when it does
        # not. add_bone_curve reports failure for a track already there, so its result is not an error.
        controller.add_bone_curve(bone)
        controller.set_bone_track_keys(bone, positions, rotations, scales)
    controller.close_bracket()
    unreal.EditorAssetLibrary.save_asset(SEQ_PATH)
    return seq


def build_montage(seq):
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("target_skeleton", SKELETON)
    # source_animation is the one part of a montage's layout Python reaches: the factory builds the
    # DefaultSlot track and its segment itself. Sections have no scripting path and need the C++ shim.
    factory.set_editor_property("source_animation", seq)
    montage = get_or_create(MONTAGE_PATH, MONTAGE_NAME, unreal.AnimMontage, factory)

    if hasattr(unreal, "GeoAnimBuilderUtil"):
        util = unreal.get_default_object(unreal.GeoAnimBuilderUtil)
        util.set_montage_slot_segment(montage, seq, "DefaultSlot")
        util.set_montage_sections(montage, [unreal.Name(n) for n in SECTION_NAMES], SECTION_STARTS,
                                  [unreal.Name(n) for n in SECTION_NEXT])
    unreal.EditorAssetLibrary.save_asset(MONTAGE_PATH)
    return montage


def link_to_pattern(montage):
    """Wire the montage into the pillar pattern, but only once it carries the sections UPattern jumps
    between: a missing Start section makes InitPattern compute a play rate of 0 and freeze the montage."""
    sections = [str(montage.get_section_name(i)) for i in range(montage.get_num_sections())]
    if not set(SECTION_NAMES).issubset(sections):
        print("NOT linked — montage sections are {}, need {}.".format(sections, SECTION_NAMES))
        return
    pattern = unreal.load_asset(PATTERN_BP)
    unreal.get_default_object(pattern.generated_class()).set_editor_property("AnimMontage", montage)
    unreal.EditorAssetLibrary.save_loaded_asset(pattern)
    print("linked {} -> {}".format(MONTAGE_NAME, PATTERN_BP))


sequence = build_sequence()
link_to_pattern(build_montage(sequence))

length = sequence.get_editor_property("sequence_length")
options = unreal.AnimPoseEvaluationOptions()
print("{} len={:.3f} frames={}".format(SEQ_NAME, length, FRAMES))
print("frame  tip.S  lean  bearing   tip.yaw  reach   sunk")
peaks, previous, rising = 0, None, False
for frame in range(FRAMES + 1):
    pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
    tip = APE.get_bone_pose(pose, "apexes_outside", unreal.AnimPoseSpaces.LOCAL)
    lean = math.hypot(tip.translation.x, tip.translation.y)
    bearing = math.degrees(math.atan2(tip.translation.y, tip.translation.x)) % 360.0
    # Tip radius is 70.71 and the valley ring 50: a tip below 50 has sunk into the body.
    print("{:5d}  {:5.2f}  {:4.0f}  {:7.1f}  {:7.1f}  {:5.1f}  {:5.1f}".format(
        frame, tip.scale3d.x, lean, bearing, tip.rotation.rotator().yaw,
        70.71 * tip.scale3d.x + lean, 70.71 * tip.scale3d.x - lean))
    if previous is not None:
        if tip.scale3d.x > previous:
            rising = True
        elif tip.scale3d.x < previous and rising:
            rising, peaks = False, peaks + 1
    previous = tip.scale3d.x
print("tip-extension peaks across the clip: {} (one per spike)".format(peaks))
