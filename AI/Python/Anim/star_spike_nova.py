"""Star boss "spike nova": the star clenches its spikes away, then throws them all out at once and settles back.

One curve drives the whole star — negative while it winds in, positive while the spikes are out, zero at rest —
and every part reads that same curve a frame or two late. The needles lead, the spike ring follows, the body's
waist drags behind both, so the body is still starved from the wind-up on the frame the spikes land and only
swells once they are already out.

The wind-up accelerates and buzzes while a needle fires out of one tip after another around the star, turning
against the way the body winds; the sweep is sized to bring its last needle home on the frame the star stops
dead, and that stillness is what makes the hit read. The spikes cross from clenched to past-full in two frames
and hold. The release is a damped spring that crosses rest, swings a little short of it and settles, so the star
recoils rather than parking.

Drives the bones the rig already has and never edits the skeleton or the skin weights. Each tip bone carries one
of the three stacked vertices its spike is built from, and the other two hide it from an orthographic camera
looking down Z, so a tip bone can only push a needle out past its spike — pulling the spikes in is the ring
bone's scale. Bones are discovered by name and each tip's outward direction is read from where it sits.

Z never reads under that camera, so nothing here touches it.

Run via mcp-unreal execute_script. Re-runnable: rewrites the same asset in place.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/Star/SK_Star"
MESH_PATH = "/Game/Characters/Meshes/Star/SKM_Star"
ANIM_PACKAGE = "/Game/Characters/Anim/Star"
SEQ_NAME = "SK_Star_Sequence_SpikeNova"
REPORT = ("C:/Users/Felou/AppData/Local/Temp/claude/C--GeoTrinity/"
          "35cec9f6-d360-4be8-9a7e-b2f43048da1c/scratchpad/spike_nova.txt")

SPIKE_PREFIX = "apexe_outside_"
RING_BONE = "apexes_outside"
WAIST_BONE = "apexes_inside"
BODY_BONE = "Root"

FPS = 30
FLARE = 4           # frames flaring out before winding in — the breath the wind-up starts on
FLARE_REACH = 0.14  # how far out that breath goes
COIL = 22           # frames winding in, accelerating the whole way
COIL_EASE = 2.2     # above 1 the wind-up accelerates instead of drifting in
TREMOR = 0.08       # buzz alternating every frame, growing as the wind-up tightens
SWEEP_REACH = 70.0  # units a tip bone travels on its turn in the sweep, well short of what the burst takes
SWEEP_OUT = 2       # frames a needle takes to fire out
SWEEP_BACK = 6      # its slower retract, still running as the next three fire
SWEEP_STEP = 2      # frames between one needle and the next; sized so the last one is home when the star freezes
COIL_HOLD = 5       # frames at the deepest coil; the parts that lag are still for the last three
SNAP = [0.45, 1.12, 1.0]  # the mid-flight frame, the overshoot that lands the hit, then full extension
HOLD = 4            # frames held with every spike out
RETURN = 10         # frames of recoil back to rest
DAMP = 2.2          # how fast the recoil dies
SPRING = 4.6        # how far past rest it swings on the way

# (frames this part lags the curve, its value at the deepest coil, its value at full extension)
NEEDLE = (0, 0.0, 110.0)   # units a tip bone travels; its vertex moves half of that
RING = (1, 0.30, 1.30)     # scale of the spike ring: spikes swallowed into the body, then grown
BODY = (1, 0.88, 1.12)     # scale of the whole star
WAIST = (2, 0.68, 1.08)    # scale of the valley ring between the spikes
BODY_YAW = (1, -16.0, 12.0)  # degrees the star winds against the burst, then snaps with it

FRAMES = COIL + COIL_HOLD + len(SNAP) + HOLD + RETURN - 1 + max(part[0] for part in
                                                                (NEEDLE, RING, BODY, WAIST, BODY_YAW))

LOG = []


def reference_pose():
    """Bone name -> (local transform, component transform) for the skeleton's reference pose."""
    ref = APE.get_reference_pose(unreal.load_asset(SKELETON_PATH))
    return {str(bone): (APE.get_bone_pose(ref, str(bone), unreal.AnimPoseSpaces.LOCAL),
                        APE.get_bone_pose(ref, str(bone), unreal.AnimPoseSpaces.WORLD))
            for bone in APE.get_bone_names(ref)}


def spike_axes(rest):
    """Tip bone name -> (outward unit x, outward unit y, place in the sweep), read from where each bone sits."""
    names = sorted((name for name in rest if name.startswith(SPIKE_PREFIX)),
                   key=lambda name: int(name[len(SPIKE_PREFIX):]))
    axes = {}
    for order, name in enumerate(names):
        position = rest[name][1].translation
        length = math.hypot(position.x, position.y)
        axes[name] = (position.x / length, position.y / length, order)
    return axes


def drive(frame):
    """-1 fully clenched, +1 every spike out, 0 at rest — the one curve every part follows."""
    if frame <= 0:
        return 0.0
    if frame < FLARE:
        return FLARE_REACH * math.sin(math.pi * frame / FLARE)
    if frame < COIL:
        alpha = (frame - FLARE) / float(COIL - 1 - FLARE)
        return -(alpha ** COIL_EASE) - TREMOR * alpha ** 3 * (1 - 2 * (frame % 2))
    frame -= COIL
    if frame < COIL_HOLD:
        return -1.0
    frame -= COIL_HOLD
    if frame < len(SNAP):
        return SNAP[frame]
    frame -= len(SNAP)
    if frame < HOLD:
        return SNAP[-1]
    alpha = (frame - HOLD + 1) / float(RETURN)
    return 0.0 if alpha >= 1.0 else math.exp(-DAMP * alpha) * math.cos(SPRING * alpha) * (1.0 - alpha)


def read(frame, part, rest):
    """What `part` is worth on `frame`: the curve it lags, spread between its coiled and its burst value."""
    lag, coiled, burst = part
    value = drive(frame - lag)
    return rest + value * ((rest - coiled) if value < 0.0 else (burst - rest))


def sweep(frame, order):
    """0 to 1 for one needle — a fast push out and a slower retract, one tip after another around the star."""
    elapsed = frame - order * SWEEP_STEP
    if frame >= COIL or elapsed <= 0.0 or elapsed >= SWEEP_OUT + SWEEP_BACK:
        return 0.0
    alpha = elapsed / float(SWEEP_OUT) if elapsed < SWEEP_OUT \
        else 1.0 - (elapsed - SWEEP_OUT) / float(SWEEP_BACK)
    return alpha * alpha * (3.0 - 2.0 * alpha)


def sample(frame, bone, rest_local, axes):
    """Frame and bone -> the local (translation, rotation, scale) to key."""
    translation = unreal.Vector(rest_local.translation.x, rest_local.translation.y, rest_local.translation.z)
    scale = unreal.Vector(rest_local.scale3d.x, rest_local.scale3d.y, rest_local.scale3d.z)
    rotation = rest_local.rotation

    if bone in axes:
        x, y, order = axes[bone]
        # Whichever is further out: this needle's turn in the sweep, or the burst that takes them all.
        reach = max(SWEEP_REACH * sweep(frame, order), read(frame, NEEDLE, 0.0))
        translation.x += x * reach
        translation.y += y * reach
    elif bone in (RING_BONE, WAIST_BONE, BODY_BONE):
        factor = read(frame, {RING_BONE: RING, WAIST_BONE: WAIST, BODY_BONE: BODY}[bone], 1.0)
        scale.x *= factor
        scale.y *= factor
        if bone == BODY_BONE:
            rotator = rotation.rotator()
            rotator.yaw += read(frame, BODY_YAW, 0.0)
            rotation = rotator.quaternion()

    return translation, rotation, scale


def build_sequence(rest, axes):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    path = "{}/{}".format(ANIM_PACKAGE, SEQ_NAME)
    sequence = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(SEQ_NAME, ANIM_PACKAGE, unreal.AnimSequence, factory)

    controller = sequence.get_editor_property("controller")
    controller.open_bracket("Build spike nova")
    controller.set_frame_rate(unreal.FrameRate(FPS, 1))
    controller.set_number_of_frames(unreal.FrameNumber(FRAMES))
    for bone, (rest_local, _) in rest.items():
        positions, rotations, scales = [], [], []
        for frame in range(FRAMES + 1):  # a sequence holds one more key than its frame count
            translation, rotation, scale = sample(frame, bone, rest_local, axes)
            positions.append(translation)
            rotations.append(rotation)
            scales.append(scale)
        # Unconditional and result ignored: the adder reports failure for an existing track, and the key setter
        # reports success whether or not a track is there.
        controller.add_bone_curve(bone)
        controller.set_bone_track_keys(bone, positions, rotations, scales)
    controller.close_bracket()
    # The keys above are the raw model; this builds the data that actually plays back from them.
    unreal.AnimationLibrary.finalize_bone_animation(sequence)
    unreal.EditorAssetLibrary.save_asset(path)
    return sequence


def skinned_radii(sequence, rest):
    """Frame -> (needle, spike, waist) radius, skinning the mesh the way the renderer does.

    What the player sees is the vertex, not the bone: every vertex here is split across two bones, so it travels a
    fraction of what its bone does and no bone track reads as the silhouette on its own.
    """
    mesh = unreal.load_asset(MESH_PATH)
    positions = unreal.get_default_object(unreal.GeoAnimBuilderUtil).get_skeletal_mesh_vertex_positions(mesh)
    modifier = unreal.SkinWeightModifier()
    modifier.set_skeletal_mesh(mesh)
    vertices = [(positions[index], {str(bone): weight
                                    for bone, weight in modifier.get_vertex_weights(index).items() if weight > 0.001})
                for index in range(modifier.get_num_vertices())]
    reference = {bone: unreal.Transform(component.translation, component.rotation.rotator(), component.scale3d)
                 for bone, (_, component) in rest.items()}

    options = unreal.AnimPoseEvaluationOptions()
    radii = []
    for frame in range(FRAMES + 1):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        posed = {}
        for bone in rest:
            component = APE.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD)
            posed[bone] = unreal.Transform(component.translation, component.rotation.rotator(), component.scale3d)
        groups = {"needle": 0.0, "spike": 0.0, "waist": 0.0}
        for position, weights in vertices:
            moved = unreal.Vector(0.0, 0.0, 0.0)
            for bone, weight in weights.items():
                local = unreal.MathLibrary.inverse_transform_location(reference[bone], position)
                moved = moved + unreal.MathLibrary.transform_location(posed[bone], local) * weight
            group = "needle" if any(bone.startswith(SPIKE_PREFIX) for bone in weights) else \
                "spike" if RING_BONE in weights else "waist"
            groups[group] = max(groups[group], math.hypot(moved.x, moved.y))
        radii.append((groups["needle"], groups["spike"], groups["waist"]))
    return radii


def report(sequence, radii):
    LOG.append("{} at {} fps: {} frames, {:.3f}s, {} playable keys (expect {})".format(
        SEQ_NAME, FPS, FRAMES, FRAMES / float(FPS),
        sequence.get_editor_property("number_of_sampled_keys"), FRAMES + 1))
    LOG.append("wind-up f0-f{}, still f{}-f{}, spikes out f{}-f{}, recoil f{}-f{}".format(
        COIL - 1, COIL, COIL + COIL_HOLD - 1, COIL + COIL_HOLD, COIL + COIL_HOLD + len(SNAP) + HOLD - 1,
        COIL + COIL_HOLD + len(SNAP) + HOLD, FRAMES))
    LOG.append("")
    LOG.append("frame  drive   needle   spike   waist   silhouette")
    for frame, (needle, spike, waist) in enumerate(radii):
        LOG.append("%5d  %+.3f  %7.1f %7.1f %7.1f   %s" % (
            frame, drive(frame), needle, spike, waist, "#" * int(round(needle / 3.0))))


rest = reference_pose()
axes = spike_axes(rest)
if not axes:
    LOG.append("ABORTED: no bone named {}N in the skeleton".format(SPIKE_PREFIX))
else:
    sequence = build_sequence(rest, axes)
    report(sequence, skinned_radii(sequence, rest))

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
