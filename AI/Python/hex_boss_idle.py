"""Hex boss idle: three hexagons turning against each other while the outer one breathes.

Seen from the orthographic camera down Z, the core creeps one way, the middle ring turns the other while
straying a little off the axis, and the outer ring only breathes in and out. Every spike stays folded into the
hexagon it grows from except for one ripple around the outer ring near the end of the clip — the only moment
the boss shows its teeth while idling.

Each hexagon turns a whole number of sixths of a turn over the clip. A sixth maps a hexagon onto itself, so the
last frame is indistinguishable from the first however far the ring has actually travelled, and the loop closes
on a rotation that never stops. The wander, the breath and the ripple all return to the reference pose at both
ends, so the clip blends in and out of anything that leaves the spikes folded.

A spike is hidden by scale rather than by moving it: its bone sits at the centre of its own base, buried inside
solid hexagon, so scaling its length to nothing collapses it into that geometry. Only X scales, so the blade
slides out of its face at full width and height instead of shrinking.

Nothing moves along Z and nothing scales on it: under an orthographic camera that motion is spent for nothing.

Run via mcp-unreal execute_script. Re-runnable: rewrites the sequence in place.
Report written to REPORT.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions
MATH = unreal.MathLibrary

SKELETON_PATH = "/Game/Characters/Meshes/HexBoss/SK_HexBoss"
MESH_PATH = "/Game/Characters/Meshes/HexBoss/SKM_HexBoss"
ANIM_PACKAGE = "/Game/Characters/Anim/HexBoss"
SEQ_NAME = "SK_HexBoss_Idle"
REPORT = ("C:/Users/Felou/AppData/Local/Temp/claude/C--GeoTrinity/"
          "0b3b8214-c633-4920-a4b1-c68d4e9de203/scratchpad/hex_boss_idle.txt")

CORE, MID, OUTER = "HexCore", "HexMid", "HexOuter"
SPIKE = "HexOuter_Spike_{}"
SPIKES = 6

FPS = 30
FRAMES = 240        # 8s, and divisible by both the breaths and the spikes so neither beat falls between samples
SAMPLE = 12         # frames between report rows

CORE_SIXTHS = 2     # sixths of a turn the core makes over the clip, counter-clockwise
MID_SIXTHS = -1     # sixths of a turn the middle ring makes, the other way
WANDER = 14.0       # units the middle ring strays from the axis
BREATH = 0.035      # fraction of itself the outer ring swells and shrinks, on X and Y
BREATHS = 2         # breaths over the clip

SPIKE_PEEK = 0.45   # X scale at the crest: an outer spike reaches 132 units in bone space and is buried 12, so
                    # its tip lands just short of the corners it stands between rather than past them
SPIKE_RIPPLE = 48   # frames one spike takes to come out and go back
SPIKE_LAG = 6       # frames each spike waits behind the one before it round the ring
SPIKE_START = 141   # first spike leaves at this frame, putting the ripple's middle on the second breath's crest

LOG = []


def snapshot(pose):
    """Every bone's local transform as plain numbers -> {bone: ((x, y, z), yaw, (sx, sy, sz))}.

    Copied out rather than held as transforms: every evaluated pose reuses one buffer. Yaw alone stands for the
    rotation because every bone of this rig is placed with one.
    """
    table = {}
    for bone in APE.get_bone_names(pose):
        local = APE.get_bone_pose(pose, str(bone), unreal.AnimPoseSpaces.LOCAL)
        table[str(bone)] = ((local.translation.x, local.translation.y, local.translation.z),
                            local.rotation.rotator().yaw,
                            (local.scale3d.x, local.scale3d.y, local.scale3d.z))
    return table


def component_transforms(pose):
    """Every bone's component-space transform, copied out of the pose's shared buffer.

    Composing a transform takes the location first, then the rotation and the scale.
    """
    table = {}
    for bone in APE.get_bone_names(pose):
        world = APE.get_bone_pose(pose, str(bone), unreal.AnimPoseSpaces.WORLD)
        table[str(bone)] = unreal.Transform(world.translation, world.rotation.rotator(), world.scale3d)
    return table


def turn(frame, sixths):
    """Yaw in degrees at `frame` for a hexagon making `sixths` sixths of a turn over the clip."""
    return 60.0 * sixths * frame / float(FRAMES)


def wander(frame):
    """Where the middle ring's centre sits at `frame` -> (x, y).

    A figure of eight rather than a circle so it crosses the axis at both ends of the clip, which is what puts
    the ring back on the reference pose there.
    """
    phase = 2.0 * math.pi * frame / float(FRAMES)
    return WANDER * math.sin(phase), WANDER * 0.6 * math.sin(2.0 * phase)


def breath(frame):
    """The outer ring's scale factor at `frame`."""
    return 1.0 + BREATH * math.sin(2.0 * math.pi * BREATHS * frame / float(FRAMES))


def peek(frame, order):
    """Spike `order`'s X scale at `frame`: 0 folded away, SPIKE_PEEK fully out.

    Each spike reads the one before it SPIKE_LAG frames late, so the ripple travels round the ring instead of
    every spike moving together.
    """
    elapsed = frame - SPIKE_START - order * SPIKE_LAG
    if elapsed <= 0 or elapsed >= SPIKE_RIPPLE:
        return 0.0
    return SPIKE_PEEK * (0.5 - 0.5 * math.cos(2.0 * math.pi * elapsed / float(SPIKE_RIPPLE)))


def key(frame, bone, rest, ripple):
    """The bone's local transform at `frame` -> (translation, yaw, scale)."""
    (x, y, z), yaw, (sx, sy, sz) = rest[bone]
    if bone == CORE:
        yaw += turn(frame, CORE_SIXTHS)
    elif bone == MID:
        yaw += turn(frame, MID_SIXTHS)
        offset_x, offset_y = wander(frame)
        x, y = x + offset_x, y + offset_y
    elif bone == OUTER:
        sx, sy = sx * breath(frame), sy * breath(frame)
    elif "_Spike_" in bone:
        sx = peek(frame, ripple[bone]) if bone in ripple else 0.0
    return (x, y, z), yaw, (sx, sy, sz)


def build(rest, ripple):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    path = "{}/{}".format(ANIM_PACKAGE, SEQ_NAME)
    sequence = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(SEQ_NAME, ANIM_PACKAGE, unreal.AnimSequence, factory)

    controller = sequence.get_editor_property("controller")
    controller.open_bracket("Build hex boss idle")
    controller.set_frame_rate(unreal.FrameRate(FPS, 1))
    controller.set_number_of_frames(unreal.FrameNumber(FRAMES))
    for bone in rest:
        positions, rotations, scales = [], [], []
        for frame in range(FRAMES + 1):  # a sequence holds one more key than its frame count
            translation, yaw, scale = key(frame, bone, rest, ripple)
            positions.append(unreal.Vector(*translation))
            rotations.append(unreal.Rotator(yaw=yaw).quaternion())
            scales.append(unreal.Vector(*scale))
        # Unconditional and result ignored: the adder reports failure for an existing track, and the key setter
        # reports success whether or not a track is there.
        controller.add_bone_curve(bone)
        controller.set_bone_track_keys(bone, positions, rotations, scales)
    controller.close_bracket()
    # The keys above land in the raw model; this builds the data that actually plays back from them.
    unreal.AnimationLibrary.finalize_bone_animation(sequence)
    unreal.EditorAssetLibrary.save_asset(path)
    return sequence


def rest_vertices():
    """The mesh's vertices grouped by the bone they hang off -> {bone: [position in that bone's space]}.

    Positions come from the editor shim and weights from the modifier; both walk the mesh description cloned
    from LOD 0, which is what makes them line up index for index.
    """
    mesh = unreal.load_asset(MESH_PATH)
    positions = unreal.get_default_object(unreal.GeoAnimBuilderUtil).get_skeletal_mesh_vertex_positions(mesh)
    modifier = unreal.SkinWeightModifier()
    modifier.set_skeletal_mesh(mesh)
    reference = component_transforms(APE.get_reference_pose(unreal.load_asset(SKELETON_PATH)))

    bound = {}
    for index, position in enumerate(positions):
        weights = modifier.get_vertex_weights(index)
        if len(weights) != 1:
            raise RuntimeError("vertex {} is split across {} bones — this rig binds each to one".format(
                index, len(weights)))
        bone = str(next(iter(weights)))
        bound.setdefault(bone, []).append(MATH.inverse_transform_location(reference[bone], position))
    return bound


def protrusion(sequence, bound):
    """How far each outer spike stands past the face it grows from, per sampled frame -> {frame: ([past], radius)}.

    The boss's outermost radius cannot answer this on its own: a hexagon's corners sit a long way further out
    than the middle of a face, so a blade clears its face well before it beats the corners it stands between.
    Projecting the spike's vertices and its ring's onto that spike's own outward direction compares the two
    where it matters. That direction is read off where the bone sits rather than from its axis, which the very
    scaling being measured would collapse.

    Every vertex is bound wholly to one bone, so putting it back out through that bone's posed transform is
    exactly where the renderer draws it.
    """
    options = unreal.AnimPoseEvaluationOptions()
    order = [SPIKE.format(index) for index in range(SPIKES)]
    reach = {}
    for frame in range(0, FRAMES + 1, SAMPLE):
        posed = component_transforms(APE.get_anim_pose_at_time(sequence, frame / float(FPS), options))
        placed = {bone: [MATH.transform_location(posed[bone], local) for local in locals]
                  for bone, locals in bound.items()}
        past = []
        for spike in order:
            axis = posed[spike].translation
            axis = (axis.x / math.hypot(axis.x, axis.y), axis.y / math.hypot(axis.x, axis.y))
            along = lambda group: max(v.x * axis[0] + v.y * axis[1] for v in placed[group])
            past.append(along(spike) - along(OUTER))
        reach[frame] = (past, max(math.hypot(v.x, v.y) for group in placed.values() for v in group))
    return reach


def report(sequence, bound):
    """Evaluate the finished sequence — the track list reads a legacy path and reports empty for every one."""
    options = unreal.AnimPoseEvaluationOptions()
    order = [SPIKE.format(index) for index in range(SPIKES)]
    reach = protrusion(sequence, bound)

    LOG.append("{} — {} frames at {} fps = {:.1f}s".format(SEQ_NAME, FRAMES, FPS, FRAMES / float(FPS)))
    LOG.append("core {:+.1f} deg/s over {} sixths, mid {:+.1f} deg/s over {} sixths".format(
        60.0 * CORE_SIXTHS * FPS / FRAMES, CORE_SIXTHS, 60.0 * MID_SIXTHS * FPS / FRAMES, MID_SIXTHS))
    LOG.append("mid wanders {:.0f} units, outer breathes +/-{:.1f}% {} times, spikes ripple from frame {}".format(
        WANDER, BREATH * 100.0, BREATHS, SPIKE_START))
    LOG.append("sampled keys {} against {} frames".format(
        sequence.get_editor_property("number_of_sampled_keys"), FRAMES))
    LOG.append("")
    LOG.append("spike columns are units of blade standing past its own face, negative meaning folded away")
    LOG.append("frame   core     mid    midX    midY   breath  " +
               " ".join("%6s" % ("sp" + name.rsplit("_", 1)[1]) for name in order) + "   radius")

    for frame in sorted(reach):
        past, radius = reach[frame]
        local = snapshot(APE.get_anim_pose_at_time(sequence, frame / float(FPS), options))
        LOG.append("%5d %7.1f %7.1f %7.2f %7.2f %8.4f  " % (
            frame, local[CORE][1], local[MID][1], local[MID][0][0], local[MID][0][1], local[OUTER][2][0]) +
            " ".join("%6.1f" % value for value in past) + " %8.2f" % radius)

    first = snapshot(APE.get_anim_pose_at_time(sequence, 0.0, options))
    last = snapshot(APE.get_anim_pose_at_time(sequence, FRAMES / float(FPS), options))
    gaps = []
    for bone, ((x, y, z), yaw, scale) in first.items():
        (lx, ly, lz), last_yaw, last_scale = last[bone]
        # A sixth of a turn leaves a hexagon looking exactly as it did, so yaw only has to close modulo one.
        gaps.append((math.dist((x, y, z), (lx, ly, lz)),
                     abs((yaw - last_yaw + 30.0) % 60.0 - 30.0),
                     max(abs(a - b) for a, b in zip(scale, last_scale))))
    LOG.append("")
    LOG.append("loop closure over all %d bones: translation %.4f units, yaw %.4f deg (mod 60), scale %.4f" % (
        len(first), max(g[0] for g in gaps), max(g[1] for g in gaps), max(g[2] for g in gaps)))
    LOG.append("blade past its face: %.1f folded, %.1f at the ripple's crest; boss radius %.1f to %.1f" % (
        min(min(past) for past, _ in reach.values()), max(max(past) for past, _ in reach.values()),
        min(radius for _, radius in reach.values()), max(radius for _, radius in reach.values())))


try:
    rest = snapshot(APE.get_reference_pose(unreal.load_asset(SKELETON_PATH)))
    ripple = {SPIKE.format(index): index for index in range(SPIKES)}
    missing = [bone for bone in [CORE, MID, OUTER] + list(ripple) if bone not in rest]
    if missing:
        raise RuntimeError("skeleton is missing {}".format(missing))
    bound = rest_vertices()
    report(build(rest, ripple), bound)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
