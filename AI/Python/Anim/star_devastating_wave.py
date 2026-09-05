"""Star boss "devastating wave": the star winds itself into a small thin spinning knot, then blows every spike out.

The charge is a spin that accelerates the whole way while the star shrinks and its arms thin out, and a crest of
needles travels around the tips against the spin, so two turning motions pull opposite ways. The crest dies out
over the last frames of the charge, the spin stops dead, and the star sits perfectly still for a beat — then
every spike goes out at once, further than the spike nova takes them.

Once the hit lands the star doesn't just hold — it dances: the body breathes a few times while each spike sways
side to side, the sway's phase offset per spike so the wobble ripples slowly around the ring instead of moving as
one block. Both are enveloped to fade in from the landed hit and fade back out into the recoil, rather than
cutting on or off. Then the recoil settles it back.

The spin covers exactly one turn, so the clip ends on the orientation it started from and blends out without
unwinding. The crest advances less than one tip per frame and the spin stays under half a tip per frame, or
either turning motion would strobe backwards instead of turning.

Same rig reading and the same one-curve-with-delays timing as `star_spike_nova.py`: negative while the star
charges, positive while the spikes are out, and each part reads it a frame or two late so the light needles lead
and the body's waist drags. Never edits the skeleton or the skin weights, and leaves Z alone — the camera is
orthographic down Z, so only what the projection shows is worth moving.

Run via mcp-unreal execute_script. Re-runnable: rewrites the same asset in place.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/Star/SK_Star"
MESH_PATH = "/Game/Characters/Meshes/Star/SKM_Star"
ANIM_PACKAGE = "/Game/Characters/Anim/Star"
SEQ_NAME = "SK_Star_Sequence_DevastatingWave"
REPORT = ("C:/Users/Felou/AppData/Local/Temp/claude/C--GeoTrinity/"
          "35cec9f6-d360-4be8-9a7e-b2f43048da1c/scratchpad/devastating_wave.txt")

SPIKE_PREFIX = "apexe_outside_"
RING_BONE = "apexes_outside"
WAIST_BONE = "apexes_inside"
BODY_BONE = "Root"

FPS = 30
CHARGE = 48         # frames of spin-up, accelerating the whole way
CHARGE_EASE = 2.4   # above 1 the charge accelerates instead of turning at a fixed speed
SPIN = 360.0        # degrees the star turns while charging; a whole turn, so it ends where it started
WAVE_TURNS = 1.25   # turns the needle crest makes around the tips, against the spin
WAVE_REACH = 110.0  # units a tip bone travels under the crest, before its ramp and the star's shrink take their cut
CREST = 4.0         # higher narrows the crest to fewer needles out at once
STILL = 4           # frames stopped dead at the tightest point of the charge
BURST = [0.45, 1.15, 1.0]  # the mid-flight frame, the overshoot that lands the hit, then full extension
DANCE_FRAMES = 32   # frames of pulsing sway held with every spike out, before the recoil takes it
DANCE_PULSES = 4    # body/ring breaths across the dance window; DANCE_FRAMES / DANCE_PULSES must be a whole number
DANCE_SWAY_TURNS = 1.0  # turns the sway ripple travels around the ring across the dance, so spikes lead/lag each other
PULSE_AMOUNT = 0.05      # fraction the body/ring breathe by, at the dance's peak
SWAY_AMOUNT = 10.0       # units a tip sways tangentially, at the dance's peak
RETURN = 14         # frames of recoil back to rest
DAMP = 2.0          # how fast the recoil dies
SPRING = 4.8        # how far past rest it swings on the way

# (frames this part lags the curve, its value at the tightest charge, its value at full extension)
NEEDLE = (0, 0.0, 180.0)    # units a tip bone travels; its vertex moves half of that
RING = (1, 1.05, 1.55)      # scale of the spike ring: arms held long while the star shrinks, then thrown out
BODY = (1, 0.55, 1.28)      # scale of the whole star
WAIST = (2, 0.40, 1.15)     # scale of the valley ring: what thins the arms down to spindles
YAW_KICK = (1, 0.0, 22.0)   # degrees the burst whips on past the spin before the recoil takes it back

DANCE_START = CHARGE + STILL + len(BURST)  # first frame of the post-hit dance window

FRAMES = CHARGE + STILL + len(BURST) + DANCE_FRAMES + RETURN - 1 + max(part[0] for part in
                                                                        (NEEDLE, RING, BODY, WAIST, YAW_KICK))

LOG = []


def reference_pose():
    """Bone name -> (local transform, component transform) for the skeleton's reference pose."""
    ref = APE.get_reference_pose(unreal.load_asset(SKELETON_PATH))
    return {str(bone): (APE.get_bone_pose(ref, str(bone), unreal.AnimPoseSpaces.LOCAL),
                        APE.get_bone_pose(ref, str(bone), unreal.AnimPoseSpaces.WORLD))
            for bone in APE.get_bone_names(ref)}


def spike_axes(rest):
    """Tip bone name -> (outward unit x, outward unit y, place in the crest), read from where each bone sits."""
    names = sorted((name for name in rest if name.startswith(SPIKE_PREFIX)),
                   key=lambda name: int(name[len(SPIKE_PREFIX):]))
    axes = {}
    for order, name in enumerate(names):
        position = rest[name][1].translation
        length = math.hypot(position.x, position.y)
        axes[name] = (position.x / length, position.y / length, order)
    return axes


def charged(frame):
    """0 to 1 across the charge, accelerating; 0 before it starts and 1 once the star has stopped."""
    return max(0.0, min(1.0, frame / float(CHARGE - 1))) ** CHARGE_EASE


def drive(frame):
    """-1 wound tightest, +1 every spike out, 0 at rest — the one curve every part follows."""
    if frame < CHARGE:
        return -charged(frame)
    frame -= CHARGE
    if frame < STILL:
        return -1.0
    frame -= STILL
    if frame < len(BURST):
        return BURST[frame]
    frame -= len(BURST)
    if frame < DANCE_FRAMES:
        return BURST[-1]
    alpha = (frame - DANCE_FRAMES + 1) / float(RETURN)
    return 0.0 if alpha >= 1.0 else math.exp(-DAMP * alpha) * math.cos(SPRING * alpha) * (1.0 - alpha)


def dance_envelope(frame):
    """0 to 1 to 0 across the dance window — fades the wobble in from the landed hit and out into the recoil."""
    if frame < 0 or frame >= DANCE_FRAMES:
        return 0.0
    return math.sin(math.pi * frame / float(DANCE_FRAMES))


def pulse(frame):
    """The body/ring breath: enveloped sine, DANCE_PULSES cycles across the dance window."""
    return dance_envelope(frame) * math.sin(2.0 * math.pi * DANCE_PULSES * frame / float(DANCE_FRAMES))


def sway(frame, order, spikes):
    """One spike's tangential wobble — phase offset by its place in the ring so the sway ripples slowly around
    it rather than every spike moving as one block, enveloped like the pulse."""
    phase = 2.0 * math.pi * (DANCE_PULSES * frame / float(DANCE_FRAMES) + DANCE_SWAY_TURNS * order / float(spikes))
    return dance_envelope(frame) * math.sin(phase)


def read(frame, part, rest):
    """What `part` is worth on `frame`: the curve it lags, spread between its charged and its burst value."""
    lag, charged_value, burst = part
    value = drive(frame - lag)
    return rest + value * ((rest - charged_value) if value < 0.0 else (burst - rest))


def crest(frame, order, spikes):
    """0 to 1 for one needle — a crest travelling around the tips, fading out as the charge closes."""
    if frame >= CHARGE:
        return 0.0
    alpha = charged(frame)
    return alpha * (1.0 - alpha ** 6) * (0.5 + 0.5 * math.cos(
        2.0 * math.pi * (WAVE_TURNS * alpha + order / float(spikes)))) ** CREST


def sample(frame, bone, rest_local, axes):
    """Frame and bone -> the local (translation, rotation, scale) to key."""
    translation = unreal.Vector(rest_local.translation.x, rest_local.translation.y, rest_local.translation.z)
    scale = unreal.Vector(rest_local.scale3d.x, rest_local.scale3d.y, rest_local.scale3d.z)
    rotation = rest_local.rotation

    if bone in axes:
        x, y, order = axes[bone]
        # Whichever is further out: this needle's turn under the travelling crest, or the burst that takes them all.
        reach = max(WAVE_REACH * crest(frame, order, len(axes)), read(frame, NEEDLE, 0.0))
        # Tangential wobble during the post-hit dance, perpendicular to the spike's own outward axis.
        wobble = sway(frame - DANCE_START, order, len(axes)) * SWAY_AMOUNT
        translation.x += x * reach - y * wobble
        translation.y += y * reach + x * wobble
    elif bone in (RING_BONE, WAIST_BONE, BODY_BONE):
        factor = read(frame, {RING_BONE: RING, WAIST_BONE: WAIST, BODY_BONE: BODY}[bone], 1.0)
        factor *= 1.0 + PULSE_AMOUNT * pulse(frame - DANCE_START)
        scale.x *= factor
        scale.y *= factor
        if bone == BODY_BONE:
            rotator = rotation.rotator()
            rotator.yaw += SPIN * charged(frame) + read(frame, YAW_KICK, 0.0)
            rotation = rotator.quaternion()

    return translation, rotation, scale


def build_sequence(rest, axes):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    path = "{}/{}".format(ANIM_PACKAGE, SEQ_NAME)
    sequence = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(SEQ_NAME, ANIM_PACKAGE, unreal.AnimSequence, factory)

    controller = sequence.get_editor_property("controller")
    controller.open_bracket("Build devastating wave")
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
    LOG.append("charge f0-f{}, still f{}-f{}, spikes out f{}-f{}, dance f{}-f{}, recoil f{}-f{}".format(
        CHARGE - 1, CHARGE, CHARGE + STILL - 1, CHARGE + STILL, CHARGE + STILL + len(BURST) - 1,
        DANCE_START, DANCE_START + DANCE_FRAMES - 1, DANCE_START + DANCE_FRAMES, FRAMES))
    LOG.append("spin tops out at {:.1f} deg/frame, crest at {:.2f} tips/frame".format(
        SPIN * (charged(CHARGE - 1) - charged(CHARGE - 2)),
        8.0 * WAVE_TURNS * (charged(CHARGE - 1) - charged(CHARGE - 2))))
    LOG.append("")
    LOG.append("frame  drive    yaw   needle   spike   waist   silhouette")
    for frame, (needle, spike, waist) in enumerate(radii):
        LOG.append("%5d  %+.3f %6.1f  %7.1f %7.1f %7.1f   %s" % (
            frame, drive(frame), SPIN * charged(frame) + read(frame, YAW_KICK, 0.0),
            needle, spike, waist, "#" * int(round(needle / 4.0))))


rest = reference_pose()
axes = spike_axes(rest)
if not axes:
    LOG.append("ABORTED: no bone named {}N in the skeleton".format(SPIKE_PREFIX))
else:
    sequence = build_sequence(rest, axes)
    report(sequence, skinned_radii(sequence, rest))

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
