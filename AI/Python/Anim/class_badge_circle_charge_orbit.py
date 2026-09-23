"""Circle badge charge beam, orbit cut: the hourglass leaves its bite and circles the badge, faster and faster for as
long as the charge is held, the whole badge swelling and trembling under it. On release it is in front at once, turned
to show its pinched waist, and holds there through the shot while the disc kicks back, then comes home to its bite.

An alternative to class_badge_circle_charge.py on the same section contract:

    Start   out of the bite and round the badge TURNS times, accelerating; stretched to the whole charge window
    End     in front, turned, held through the shot, then home — the release jumps here from anywhere in Start

The orbit turns Top, so the hourglass is anywhere round the badge when the beam fires; the rig keeps this badge's fire
socket on the root, at the front, for that. The montage goes in the top slot: the disc and the root move too, which
shows while the bottom slot plays nothing and gives way to whatever it plays.

Authored at 30 fps and sped up to the charge window by the play rate; End runs at that same rate, so its frames are
worth a quarter of a displayed one at a 0.5 s window.

Run AFTER AI/Python/Mesh/rig_class_badges.py, via mcp-unreal execute_script. Re-runnable: rewrites the sequence and
the montage in place. Report written to Saved/class_badge_circle_charge_orbit.txt.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/Class/SK_CircleBadge"
MESH_PATH = "/Game/Characters/Meshes/Class/SKM_CircleBadge"
GENERATOR = "AI/Python/Mesh/generate_class_badge_meshes.py"
ANIM_PACKAGE = "/Game/Characters/Anim/ClassBadge/Circle"
SEQUENCE_NAME = "SK_CircleBadge_Sequence_ChargeOrbit"
MONTAGE_NAME = "SK_CircleBadge_Montage_ChargeOrbit"
REPORT = unreal.Paths.project_saved_dir() + "class_badge_circle_charge_orbit.txt"

ROOT, BODY, TOP, PART = "Root", "Bottom", "Top", "Hourglass"
SLOT = "Top"

FPS = 30
CHARGE = 60        # Start's frames, sped up to the ability's charge window
RELEASE = 56       # End's frames

TURNS = 2          # times round the badge over a full charge, from standing to its fastest
ORBIT_OUT = 38.0   # units the hourglass moves out to circle the disc clear of it
OUT_FRAMES = 6     # frames it takes to leave the bite, before the orbit has picked up
CHARGED_SCALE = 1.15  # the hourglass fully charged
SWELL = 1.04       # the disc fully charged
SHAKE = 1.5        # units the whole badge trembles at full charge

LEAP = 22.0        # units ahead of its bite the hourglass holds after the release
TILT = 90.0        # degrees it is pitched there, showing its waist
# The landing after the release: forward offset (fraction of LEAP) and scale, then held until HOLD_END.
LAND = [(1.10, 1.45), (1.04, 1.38), (1.00, 1.33)]
HELD_SCALE = 1.3
HOLD_END = 20      # 5 displayed frames at 30 fps
TREMBLE = 1.0      # units the hourglass trembles through the shot
UNTILT = 0.45      # fraction of the way home by which it has turned flat again, before it nears the bite
DEPART = 0.15      # fraction of the way home it waits before moving back
ARRIVE = 0.85      # fraction of the way home it lands in its bite on, squashing
ARRIVE_SQUASH = 0.1

KICKED = (-6.0, 0.86, 1.08)  # the disc under the shot: forward offset, X scale, Y scale
KICK_HELD = 0.6              # how much of the kick is left when the hourglass starts home
SPRING_POWER = 1.5
SPRING = 3.4

FRAMES = CHARGE + RELEASE
SECTIONS = [("Start", 0, "End"), ("End", CHARGE, "None")]

LOG = []


def smoothstep(alpha):
    alpha = min(1.0, max(0.0, alpha))
    return alpha * alpha * (3.0 - 2.0 * alpha)


def kicked_body(amount):
    """The disc `amount` of the way into its kick -> (forward offset, X scale, Y scale)."""
    forward, scale_x, scale_y = KICKED
    return forward * amount, 1.0 + (scale_x - 1.0) * amount, 1.0 + (scale_y - 1.0) * amount


def state(frame):
    """Every animated value at a sequence frame, as a dict: Top yaw, the hourglass, the disc and the root."""
    if frame < CHARGE:
        charged = frame / float(CHARGE)
        # Constant acceleration from standing: the turn rate climbs linearly all the way to the release.
        shaking = SHAKE * charged ** 2 * (1.0 if frame // 2 % 2 else -1.0)
        scale = 1.0 + (CHARGED_SCALE - 1.0) * charged
        swell = 1.0 + (SWELL - 1.0) * charged
        return {"yaw": 360.0 * TURNS * charged ** 2,
                "forward": ORBIT_OUT * (1.0 - (1.0 - min(1.0, frame / float(OUT_FRAMES))) ** 2),
                "side": 0.0, "pitch": 0.0, "scale": (scale, scale),
                "body": (0.0, swell, swell), "root": shaking}

    local = frame - CHARGE
    if local <= HOLD_END:
        reach, scale = LAND[local] if local < len(LAND) else (1.0, HELD_SCALE)
        side = TREMBLE * (1.0 if local // 2 % 2 else -1.0) if len(LAND) <= local < HOLD_END else 0.0
        kick = 1.0 - (1.0 - KICK_HELD) * local / float(HOLD_END)
        return {"yaw": 0.0, "forward": LEAP * reach, "side": side, "pitch": TILT, "scale": (scale, scale),
                "body": kicked_body(kick),
                "root": 0.0}

    alpha = (local - HOLD_END) / float(RELEASE - HOLD_END)
    scale = HELD_SCALE + (1.0 - HELD_SCALE) * smoothstep(alpha / ARRIVE)
    squash = ARRIVE_SQUASH * math.sin(math.pi * (alpha - ARRIVE) / (1.0 - ARRIVE)) if alpha > ARRIVE else 0.0
    kick = KICK_HELD * (1.0 - alpha) ** SPRING_POWER * math.cos(SPRING * alpha)
    return {"yaw": 0.0, "forward": LEAP * (1.0 - smoothstep((alpha - DEPART) / (ARRIVE - DEPART))), "side": 0.0,
            "pitch": TILT * (1.0 - smoothstep(alpha / UNTILT)), "scale": (scale - squash, scale + squash * 0.8),
            "body": kicked_body(kick), "root": 0.0}


def key(frame, bone, rest_local):
    translation, scale = rest_local.translation, rest_local.scale3d
    values = state(frame)
    if bone == ROOT:
        return unreal.Vector(translation.x, translation.y + values["root"], translation.z), rest_local.rotation, scale
    if bone == BODY:
        forward, scale_x, scale_y = values["body"]
        return (unreal.Vector(translation.x + forward, translation.y, translation.z), rest_local.rotation,
                unreal.Vector(scale.x * scale_x, scale.y * scale_y, scale.z))
    if bone == TOP:
        return translation, unreal.Rotator(yaw=values["yaw"]).quaternion(), scale
    if bone == PART:
        scale_x, scale_y = values["scale"]
        # Z scales with X: pitched, the hourglass's depth is what lies along the beam.
        return (unreal.Vector(translation.x + values["forward"], translation.y + values["side"], translation.z),
                unreal.Rotator(pitch=values["pitch"]).quaternion(),
                unreal.Vector(scale.x * scale_x, scale.y * scale_y, scale.z * scale_x))
    return translation, rest_local.rotation, scale


def build(toolkit):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    sequence = toolkit["get_or_create_asset"](ANIM_PACKAGE, SEQUENCE_NAME, unreal.AnimSequence, factory)
    toolkit["write_bone_tracks"](sequence, SKELETON_PATH, FPS, FRAMES, key, "Build circle badge orbit charge")

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


def report(toolkit, sequence, montage, groups, reference):
    outline = body_outline()
    options = unreal.AnimPoseEvaluationOptions()
    socket = unreal.load_asset(MESH_PATH).find_socket("anim_socket_0")
    LOG.append("fire socket on %s at %s" % (socket.get_editor_property("bone_name"),
                                           socket.get_socket_local_transform().translation))
    LOG.append("")
    LOG.append("frame section  top yaw  centre x  centre y  pitch     sx     sy  to disc")
    gaps = []
    for frame in range(FRAMES + 1):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        posed = toolkit["_component_transforms"](pose)
        hull = toolkit["convex_hull"](toolkit["place_groups"](groups, posed)[PART])
        gap = toolkit["outline_separation"](hull, toolkit["move_outline"](outline, reference[BODY], posed[BODY]))
        gaps.append(gap)
        values, centre = state(frame), posed[PART].translation
        LOG.append("%5d %-7s  %7.1f  %8.1f  %8.1f  %5.1f  %5.2f  %5.2f  %+7.2f" % (
            frame, "Start" if frame < CHARGE else "End", values["yaw"], centre.x, centre.y, values["pitch"],
            values["scale"][0], values["scale"][1], gap))

    LOG.append("")
    LOG.append("closest to the disc: %.2f (-1 = overlapping)" % min(gaps))
    steps = [state(f)["yaw"] - state(f - 1)["yaw"] for f in range(1, CHARGE)]
    LOG.append("orbit: %.1f deg/frame at the start, %.1f at the end (%.0f deg a displayed frame at a 0.5 s window)"
               % (steps[0], steps[-1], steps[-1] * CHARGE / (0.5 * FPS)))
    LOG.append("sequence ends on rest: %s" % (state(FRAMES)["forward"] == 0.0 and state(FRAMES)["pitch"] == 0.0))


try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    groups = toolkit["rigid_vertex_groups"](MESH_PATH, SKELETON_PATH)
    reference = toolkit["_component_transforms"](APE.get_reference_pose(unreal.load_asset(SKELETON_PATH)))
    sequence, montage = build(toolkit)
    LOG.append("{} - {} keys for {} frames, sections {}".format(
        SEQUENCE_NAME, toolkit["playable_key_count"](sequence), FRAMES, toolkit["montage_sections"](montage)))
    report(toolkit, sequence, montage, groups, reference)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
