"""Circle badge charge beam: the hourglass whips once round the badge, tumbling, then swells in front of it and
shakes harder the longer the charge is held; releasing flattens it really wide before it springs back into its bite.

GA_Circle_ChargeBeam plays Start on press, stretched to the whole charge window, and jumps to End on release, which
fires the beam from anim_socket_0, which the rig holds still at the front. When the charge runs full, Start simply
runs into End. So Start has no stillness until its very end — a release may land anywhere in it — and the further
in, the bigger and the harder shaking the hourglass it releases from:

    Start   0 .. ORBIT        out of the bite and once round the badge, turning Top
            ORBIT .. SETTLE   back in to hang in front
            SETTLE .. end     swelling and shaking, the tumble accelerating; the last STILL frames dead still
    End     flattened really wide, held, then sprung back into place

The earliest release the ability allows falls at the end of the orbit, so the hourglass is in front for every shot.
The orbit runs out wide enough that the hourglass clears the disc all the way round.

Authored at the 30 fps the engine samples sequences at and sped up to the charge window by the play rate; End runs
at that same rate. Everything is keyed on the Top branch and the montage goes in the Top slot, so the body's own
layer is left alone. The tumble is about the hourglass's own X axis, which shows its pinched waist edge-on.

Run AFTER AI/Python/Mesh/rig_class_badges.py, via mcp-unreal execute_script. Re-runnable: rewrites the sequence and
the montage in place. Report written to Saved/class_badge_circle_charge.txt.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/Class/SK_CircleBadge"
MESH_PATH = "/Game/Characters/Meshes/Class/SKM_CircleBadge"
GENERATOR = "AI/Python/Mesh/generate_class_badge_meshes.py"
ANIM_PACKAGE = "/Game/Characters/Anim/ClassBadge/Circle"
SEQUENCE_NAME = "SK_CircleBadge_Sequence_Charge"
MONTAGE_NAME = "SK_CircleBadge_Montage_Charge"
REPORT = unreal.Paths.project_saved_dir() + "class_badge_circle_charge.txt"

TOP, PART = "Top", "Hourglass"
SLOT = "Top"

FPS = 30
CHARGE = 60        # Start's frames, sped up to the ability's charge window
ORBIT = 8          # frames for the one turn round the badge: 13% of the window, its earliest release
SETTLE = 12        # frame the hourglass hangs in front by
STILL = 3          # frames dead still once fully charged
RELEASE = 24       # End's frames

ORBIT_OUT = 38.0   # units the hourglass moves out to go round the disc clear of it
FRONT = 20.0       # units ahead of its bite it hangs before it swells
RADIUS = 15.75     # the hourglass's own radius: it moves out by what it swells, so its back edge holds still
GROW = 1.9         # its scale fully charged
CHARGE_EASE = 1.3
SHAKE = 3.0        # units it shakes at full charge
SHAKE_EASE = 1.5

ORBIT_TUMBLE = 360.0   # degrees it tumbles going round
CHARGE_TUMBLE = 1080.0  # degrees it tumbles while charging, accelerating
RELEASE_TUMBLE = 360.0  # degrees the blast spins it through on the way home, dying away
STROBE = 90.0           # half the hourglass's own symmetry: past this per frame a tumble reads as going backwards

# End, frame by frame until the spring takes over: forward offset (fraction of the charged one), X scale, Y scale.
BLAST = [
    (1.00, GROW, GROW),
    (0.92, 1.35, 2.80),   # mid-flight
    (0.86, 0.85, 3.70),   # really wide, overshooting
    (0.88, 0.90, 3.45),
    (0.90, 0.92, 3.35),
    (0.90, 0.95, 3.30),   # held wide
]
SPRING_POWER = 1.6
SPRING = 3.6
REACH_LAG = 3      # frames the way home trails the narrowing, so it is thin before it passes the disc

FRAMES = CHARGE + RELEASE
SECTIONS = [("Start", 0, "End"), ("End", CHARGE, "None")]

LOG = []


def smoothstep(alpha):
    alpha = min(1.0, max(0.0, alpha))
    return alpha * alpha * (3.0 - 2.0 * alpha)


def charge_amount(frame):
    """0 when the hourglass first hangs in front, 1 once fully charged, then held."""
    return min(1.0, max(0.0, frame - SETTLE) / float(CHARGE - STILL - SETTLE))


def charge_tumble(frame):
    """Degrees tumbled since the orbit, the rate climbing linearly and stopping dead at full charge."""
    span = CHARGE - STILL - SETTLE
    done = min(span, max(0, frame - SETTLE))
    return CHARGE_TUMBLE * done * (done + 1) / float(span * (span + 1))


def spring_home(frame, span):
    """Frames since the hold -> fraction of the way still to go over `span` frames, crossing rest once and settling."""
    alpha = frame / float(span)
    return 0.0 if alpha >= 1.0 else (1.0 - alpha) ** SPRING_POWER * math.cos(SPRING * alpha)


def charged_reach(scale):
    """How far ahead of its bite the hourglass hangs at a given scale."""
    return FRONT + RADIUS * (scale - 1.0)


def state(frame):
    """(Top yaw, hourglass forward offset, sideways shake, X scale, Y scale, tumble degrees) at a sequence frame."""
    if frame >= CHARGE:
        local = frame - CHARGE
        tumble = ORBIT_TUMBLE + CHARGE_TUMBLE + RELEASE_TUMBLE * (1.0 - (1.0 - min(1.0, local / 12.0)) ** 2)
        if local < len(BLAST):
            reach, scale_x, scale_y = BLAST[local]
            return 360.0, charged_reach(GROW) * reach, 0.0, scale_x, scale_y, tumble
        since, span = local - len(BLAST) + 1, RELEASE - len(BLAST) + 1
        remaining = spring_home(since, span)
        trailing = spring_home(max(0, since - REACH_LAG), span - REACH_LAG)
        reach, scale_x, scale_y = BLAST[-1]
        return (360.0, charged_reach(GROW) * reach * trailing, 0.0, 1.0 + (scale_x - 1.0) * remaining,
                1.0 + (scale_y - 1.0) * remaining, tumble)

    if frame <= ORBIT:
        yaw = 360.0 * smoothstep(frame / float(ORBIT))
        out = ORBIT_OUT * (1.0 - (1.0 - min(1.0, frame / 3.0)) ** 2)
        return yaw, out, 0.0, 1.0, 1.0, ORBIT_TUMBLE * frame / float(ORBIT)

    charged = charge_amount(frame)
    scale = 1.0 + (GROW - 1.0) * charged ** CHARGE_EASE
    reach = ORBIT_OUT + (charged_reach(scale) - ORBIT_OUT) * smoothstep((frame - ORBIT) / float(SETTLE - ORBIT))
    shaking = SHAKE * charged ** SHAKE_EASE if frame < CHARGE - STILL else 0.0
    return (360.0, reach + (shaking * 0.5 if frame % 4 < 2 else -shaking * 0.5), shaking * (1 if frame % 2 else -1),
            scale, scale, ORBIT_TUMBLE + charge_tumble(frame))


def key(frame, bone, rest_local):
    translation, scale = rest_local.translation, rest_local.scale3d
    yaw, reach, shake, scale_x, scale_y, tumble = state(frame)
    if bone == TOP:
        return translation, unreal.Rotator(yaw=yaw).quaternion(), scale
    if bone != PART:
        return translation, rest_local.rotation, scale
    # Y and Z scale alike, so the tumble shows the hourglass and never a squashed one.
    return (unreal.Vector(translation.x + reach, translation.y + shake, translation.z),
            unreal.Rotator(roll=tumble).quaternion(),
            unreal.Vector(scale.x * scale_x, scale.y * scale_y, scale.z * scale_y))


def build(toolkit):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    sequence = toolkit["get_or_create_asset"](ANIM_PACKAGE, SEQUENCE_NAME, unreal.AnimSequence, factory)
    toolkit["write_bone_tracks"](sequence, SKELETON_PATH, FPS, FRAMES, key, "Build circle badge charge")

    montage = toolkit["build_montage"](sequence, ANIM_PACKAGE, MONTAGE_NAME, [name for name, _, _ in SECTIONS],
                                       [start / float(FPS) for _, start, _ in SECTIONS],
                                       [following for _, _, following in SECTIONS], SLOT)
    for name, seconds in (("blend_in", 0.05), ("blend_out", 0.1)):
        blend = montage.get_editor_property(name)
        blend.set_editor_property("blend_time", seconds)
        montage.set_editor_property(name, blend)
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(ANIM_PACKAGE, MONTAGE_NAME))
    return sequence, montage


def body_outline():
    """The bitten disc's world outline, from the generator that built it."""
    path = unreal.Paths.project_dir() + GENERATOR
    generator = {"__name__": "badge_generator"}
    exec(compile(open(path).read(), path, "exec"), generator)
    return generator["to_world"]("SM_CircleBadge")[0].outline


def report(toolkit, sequence, montage, groups, outline):
    options = unreal.AnimPoseEvaluationOptions()
    LOG.append("{} - {} frames at {} fps, {} sampled keys (expect {})".format(
        SEQUENCE_NAME, FRAMES, FPS, toolkit["playable_key_count"](sequence), FRAMES + 1))
    LOG.append("montage sections read back: {}".format(toolkit["montage_sections"](montage)))
    LOG.append("")
    LOG.append("frame section  top yaw  centre x  centre y     sx     sy  tumble  to body")
    gaps = []
    for frame in range(FRAMES + 1):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        posed = toolkit["_component_transforms"](pose)
        placed = toolkit["place_groups"](groups, posed)
        gap = toolkit["outline_separation"](toolkit["convex_hull"](placed[PART]), outline)
        gaps.append(gap)
        centre = posed[PART].translation
        yaw, _, _, scale_x, scale_y, tumble = state(frame)
        LOG.append("%5d %-7s %7.1f  %8.1f  %8.1f  %5.2f  %5.2f  %6.0f  %+6.2f" % (
            frame, "Start" if frame < CHARGE else "End", yaw, centre.x, centre.y, scale_x, scale_y, tumble, gap))

    LOG.append("")
    LOG.append("closest to the disc: %.2f (-1 = overlapping)" % min(gaps))
    steps = [abs(state(f)[5] - state(f - 1)[5]) for f in range(1, FRAMES + 1)]
    LOG.append("fastest tumble %.0f deg/frame (reads backwards past %.0f); tumble at the end %.0f (want a whole turn)"
               % (max(steps), STROBE, state(FRAMES)[5] % 360.0))
    LOG.append("orbit: top yaw moves up to %.0f deg/frame" % max(
        abs(state(f)[0] - state(f - 1)[0]) for f in range(1, ORBIT + 1)))


try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    groups = toolkit["rigid_vertex_groups"](MESH_PATH, SKELETON_PATH)
    sequence, montage = build(toolkit)
    report(toolkit, sequence, montage, groups, body_outline())
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
