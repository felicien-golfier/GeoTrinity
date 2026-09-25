"""Class badge deploys, one clip per badge, all on one beat: winding up to throw the deployable, the throw, the recoil.

    Square    SK_SquareBadge_Montage_Deploy     GA_Square_Special_Mine
              the mandibles move forward and roll about the aim faster and faster, stretching along X and vibrating
              harder; the throw stops them dead and crushes them and the block along X against their own backs
    Triangle  SK_TriangleBadge_Montage_Deploy   GA_LaunchTurret
              the badge rears back, the plug drawn onto the back rim of its hole and the needle lifted and shrunk,
              trembling; on the throw the badge lunges, the plug slams the front rim and the needle stakes out ahead
    Circle    SK_CircleBadge_Montage_Deploy     GA_DeployHealingZone
              the hourglass leaves its bite and stands up to show its waist, the disc swelling and trembling; on the
              throw it is flung out, the disc pulses, and it lies flat again before it flies home

Each montage plays two sections from one sequence, authored at 30 fps and played at 2x, so one frame lands on each
displayed frame at 60 fps:

    Start   the wind-up          18 frames, stretched to the 0.3 s charge window
    End     the throw, held,     7 frames, 0.12 s, or the Square's 9 opening on its held extension; the release
            sprung home          jumps here from anywhere in Start

A deploy is mostly a tap released about 0.1 s in, so every wind-up does most of its moving in its first frames and a
tap still reads; the whole clip never runs past 0.45 s. The throw is End's first frames, so a release before full
charge cuts straight to it. Every montage goes in the full-body slot, over the auto-fire and any body turn: the
ability system then leaves auto-fire's own montage unplayed until this one is done. Nothing moves the root, whose fire
sockets must stay put, and nothing kicks the whole badge back, which the character's game feel already does.

Run AFTER AI/Python/Mesh/rig_class_badges.py, via mcp-unreal execute_script, then AI/Python/Anim/class_badge_wiring.py.
Re-runnable: rewrites the sequences and montages in place. Report written to Saved/class_badge_deploy.txt.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions

MESH_FOLDER = "/Game/Characters/Meshes/Class"
ANIM_FOLDER = "/Game/Characters/Anim/ClassBadge"
GENERATOR = "AI/Python/Mesh/generate_class_badge_meshes.py"
REPORT = unreal.Paths.project_saved_dir() + "class_badge_deploy.txt"

ROOT, BODY, TOP = "Root", "Bottom", "Top"
SLOT = "DefaultSlot"

FPS = 30
START = 18       # Start's frames, played at 2x to fill the 0.3 s charge window
WOUND = 14       # the last frame a held wind-up still moves on; the Square's keeps going to the throw
MID = START + 1  # the one frame in flight
LAND = MID + 1   # the extreme, overshot
HOLD_END = LAND + 1
FRAMES = HOLD_END + 4
MID_FLIGHT = 0.4  # share of the way to the extreme crossed on the frame in flight
OVERSHOOT = 1.1
SETTLE_POWER = 1.5   # how fast the swing back dies
SETTLE_SPRING = 3.4  # sets where it crosses rest: once, a little past halfway home
TREMBLE_FROM = 2     # first frame of the wind-up that trembles
BLEND = 0.03         # seconds in and out: longer would eat the spring

# Per badge: each channel's value at rest, wound and thrown, and how far it trembles on the wind-up. A channel a clip
# cannot push past a stop is clamped by its key function, not here.
CLIPS = {
    "Triangle": {
        # the whole badge's forward shove; the plug's slide; the needle's forward, lift and scale; the badge's size
        "rest":   {"shove": 0.0, "plug": 0.0, "needle": 0.0, "lift": 0.0, "needle_size": 1.0, "size": 1.0},
        "wound":  {"shove": -6.0, "plug": -1.0, "needle": 0.0, "lift": 10.0, "needle_size": 0.6, "size": 0.94},
        "thrown": {"shove": 6.0, "plug": 1.0, "needle": 34.0, "lift": 0.0, "needle_size": 1.0, "size": 1.06},
        "tremble": {"shove": 0.8, "needle_side": 1.5},
    },
    "Circle": {
        # the hourglass's forward and scale; the disc's scale; the whole badge's sideways shake and size
        "rest":   {"glass": 0.0, "glass_size": 1.0, "disc": 1.0, "shake": 0.0, "size": 1.0},
        "wound":  {"glass": 16.0, "glass_size": 1.1, "disc": 1.05, "shake": 0.0, "size": 0.95},
        "thrown": {"glass": 26.0, "glass_size": 1.3, "disc": 1.12, "shake": 0.0, "size": 1.06},
        "tremble": {"shake": 1.2},
    },
}

# Triangle: the plug's slide that lays its tip on either rim of its hole, one unit of the "plug" channel.
RIM = 10.8
# Circle: the hourglass stands up to show its waist on the wind-up and holds it through the throw, then lies flat
# while still out, and only then flies home: turning closer in would put a corner into the bite. The move out leads
# the stand-up for the same reason.
STAND = 90.0
GLASS_LEAD = 6       # frames the hourglass takes to leave its bite
PITCH_DELAY = 3      # frames the stand-up reads behind the move out
FLAT_END = HOLD_END + 2

# Square: the mandibles move forward and out toward the ledge and roll about the aim, faster and faster and vibrating
# harder, stretching along X the whole while, until the throw stops them dead at full extension on the ledge, whatever
# the charge reached, and holds them there. The recoil then crushes them straight from that along X against their own backs, which stay put through
# stretch and crush alike so the bar reads as compressed rather than moved, the block following a little the same way;
# then both spring back. The hold makes its sequence SQ_SHOT_HOLD frames longer than the others.
SQ_TURNS = 2           # whole turns rolled over the wind-up, so the blend out never shows one
SQ_ROLL_BASE = 6.0     # the roll's rate on its first frame, in frames' worth of its climb: rolling from the start
SQ_STROBE = 90.0       # half the bar's half-turn symmetry: rolled further in a frame, it reads as turning back
SQ_FORWARD = 6.0       # units the mandibles move forward as the charge begins
SQ_LEAVE = 4           # frames they take to get there
SQ_STRETCH = 1.6       # a mandible's X scale grown to over the whole charge
SQ_SHOT_STRETCH = 2.0  # its X scale on the shot, whatever the charge reached: End opens on it
SQ_SHOT_HOLD = 2       # frames the shot's full extension holds dead still before the recoil
SQ_SHAKE = (0.8, 1.2)  # forward and outward units the mandibles shake by as the roll peaks; outward only, never in
SQ_BODY_SHAKE = 0.4    # forward units the block shakes by with them
SQ_CRUSH = 0.55        # a mandible's X scale held through the recoil
SQ_BODY_CRUSH = 0.9    # the block's
SQ_BULGE = 0.35        # Y scale gained per unit of X scale lost
JAWS = {"MandibleLeft": -1.0, "MandibleRight": 1.0}

LOG = []
GEOMETRY = {}  # the Square's backs, filled before its build; the Triangle's arrowhead, before its report


def clamp(alpha):
    return min(1.0, max(0.0, alpha))


def smoothstep(alpha):
    alpha = clamp(alpha)
    return alpha * alpha * (3.0 - 2.0 * alpha)


def ease_out(alpha):
    alpha = clamp(alpha)
    return 1.0 - (1.0 - alpha) ** 2


def settle(alpha):
    """Share of the throw still held `alpha` of the way home: crosses rest once and is still at 1."""
    return 0.0 if alpha >= 1.0 else (1.0 - alpha) ** SETTLE_POWER * math.cos(SETTLE_SPRING * alpha)


def thrown(frame):
    """Share of the throw from rest at an End frame: overshot on landing, held, then sprung home past rest."""
    if frame == MID:
        return MID_FLIGHT
    if frame == LAND:
        return OVERSHOOT
    if frame <= HOLD_END:
        return 1.0
    return settle((frame - HOLD_END) / float(FRAMES - HOLD_END))


def tremble(frame, last):
    """-1..1 every other frame, growing from TREMBLE_FROM to its fullest on `last`, still after it."""
    if not TREMBLE_FROM <= frame <= last:
        return 0.0
    return (frame - TREMBLE_FROM + 1) / float(last - TREMBLE_FROM + 1) * (1.0 if frame % 2 else -1.0)


def mix(rest, pose, amount):
    return {channel: rest[channel] + (pose[channel] - rest[channel]) * amount for channel in rest}


def wind(frame, delay=0, lead=WOUND):
    """0 at rest to 1 wound, read `delay` frames late, over `lead` frames, fastest at first so a tap reads."""
    return ease_out((frame - delay) / float(lead))


def state(clip, frame):
    """Every channel of `clip` at a sequence frame, the tremble included."""
    rest, wound, thrown_pose = clip["rest"], clip["wound"], clip["thrown"]
    if frame <= START:
        values = mix(rest, wound, wind(frame))
        for channel, amount in clip["tremble"].items():
            values[channel] = values.get(channel, 0.0) + amount * tremble(frame, WOUND - 1)
        return values
    if frame == MID:
        return mix(wound, thrown_pose, MID_FLIGHT)
    return mix(rest, thrown_pose, thrown(frame))


def sized(scale, size, x=1.0, y=1.0):
    """A layer bone's scale at the badge's size, in the view plane only so its children's heights stay put."""
    return unreal.Vector(scale.x * size * x, scale.y * size * y, scale.z)


def square_roll(frame):
    """Degrees the mandibles have rolled: rolling from the first frame, the rate climbing every frame up to the throw,
    which stops them dead on a pose looking as they rested."""
    if frame >= START:
        return 0.0

    def climbed(upto):
        return upto * (upto + 1) / 2.0 + SQ_ROLL_BASE * upto

    return 360.0 * SQ_TURNS * climbed(frame) / climbed(START)


def square_state(frame):
    """The Square's channels at a sequence frame: roll, forward move, shake, and the stretched or crushed scales."""
    if frame < START:
        grown = math.sqrt(frame / float(START))
        jaw_x, spread = 1.0 + (SQ_STRETCH - 1.0) * grown, grown
        forward, shake, body_x = SQ_FORWARD * ease_out(frame / float(SQ_LEAVE)), tremble(frame, START - 1), 1.0
    elif frame <= START + SQ_SHOT_HOLD:
        jaw_x, spread, forward, shake, body_x = SQ_SHOT_STRETCH, 1.0, SQ_FORWARD, 0.0, 1.0
    else:
        # The shared throw, run late by the hold. The mandibles stay on the ledge through the crush, and the spring
        # carries them home.
        recoil = frame - SQ_SHOT_HOLD
        share = thrown(recoil)
        if recoil == MID:
            jaw_x = SQ_SHOT_STRETCH + (SQ_CRUSH - SQ_SHOT_STRETCH) * MID_FLIGHT
        else:
            jaw_x = 1.0 - share * (1.0 - SQ_CRUSH)
        spread = min(1.0, share) if recoil != MID else 1.0
        forward = SQ_FORWARD * (1.0 - MID_FLIGHT) if recoil == MID else 0.0
        shake, body_x = 0.0, 1.0 - share * (1.0 - SQ_BODY_CRUSH)
    return {"roll": square_roll(frame), "forward": forward, "spread": spread, "shake": shake,
            "jaw_x": jaw_x, "jaw_y": 1.0 + SQ_BULGE * (1.0 - jaw_x),
            "body_x": body_x, "body_y": 1.0 + SQ_BULGE * (1.0 - body_x)}


def square_key(frame, bone, rest_local):
    translation, rotation, scale = rest_local.translation, rest_local.rotation, rest_local.scale3d
    values = square_state(frame)
    if bone == BODY:
        # Crushed along X about its back, which stays where it rests.
        pinned = -GEOMETRY["block_back"] * (1.0 - values["body_x"])
        return (unreal.Vector(translation.x + pinned + SQ_BODY_SHAKE * values["shake"], translation.y, translation.z),
                rotation, unreal.Vector(scale.x * values["body_x"], scale.y * values["body_y"], scale.z))
    if bone in JAWS:
        pinned = -GEOMETRY["jaw_back"] * (1.0 - values["jaw_x"])
        return (unreal.Vector(translation.x + values["forward"] + pinned + SQ_SHAKE[0] * values["shake"],
                              translation.y + JAWS[bone] * (ledge_out(values["jaw_y"]) * values["spread"]
                                                            + SQ_SHAKE[1] * max(0.0, values["shake"])),
                              translation.z),
                unreal.Rotator(roll=values["roll"]).quaternion(),
                unreal.Vector(scale.x * values["jaw_x"], scale.y * values["jaw_y"], scale.z))
    return translation, rotation, scale


def triangle_key(frame, bone, rest_local):
    translation, rotation, scale = rest_local.translation, rest_local.rotation, rest_local.scale3d
    values = state(CLIPS["Triangle"], frame)
    if bone in (BODY, TOP):
        # Both layers stand on the badge's centre, so one shove and one size carry them together.
        return (unreal.Vector(translation.x + values["shove"], translation.y, translation.z), rotation,
                sized(scale, values["size"]))
    if bone == "Plug":
        # Its hole's rims are hard stops: the overshoot never pushes it past one.
        slide = RIM * max(-1.0, min(1.0, values["plug"]))
        return unreal.Vector(translation.x + slide, translation.y, translation.z), rotation, scale
    if bone == "Needle":
        # Behind its rest it would sink into the arrowhead, which thickens toward the back; below it, likewise.
        return (unreal.Vector(translation.x + max(0.0, values["needle"]), translation.y + values.get("needle_side", 0.0),
                              translation.z + max(0.0, values["lift"])),
                rotation, scale * values["needle_size"])
    return translation, rotation, scale


def circle_state(frame):
    """Circle's channels and the hourglass's pitch: it leads its stand-up out of the bite, and lies flat again before
    it comes home."""
    clip = CLIPS["Circle"]
    thrown_pose = clip["thrown"]
    values = state(clip, frame)
    if frame <= START:
        values["glass"] = clip["wound"]["glass"] * wind(frame, 0, GLASS_LEAD)
        values["pitch"] = STAND * wind(frame, PITCH_DELAY, WOUND - PITCH_DELAY)
    elif frame <= HOLD_END:
        values["pitch"] = STAND
    else:
        home = smoothstep((frame - FLAT_END) / float(FRAMES - FLAT_END))
        values["glass"] = thrown_pose["glass"] * (1.0 - home)
        values["glass_size"] = thrown_pose["glass_size"] + (1.0 - thrown_pose["glass_size"]) * home
        values["pitch"] = STAND * (1.0 - clamp((frame - HOLD_END) / float(FLAT_END - HOLD_END)))
    return values


def circle_key(frame, bone, rest_local):
    translation, rotation, scale = rest_local.translation, rest_local.rotation, rest_local.scale3d
    values = circle_state(frame)
    if bone == BODY:
        return (unreal.Vector(translation.x, translation.y + values["shake"], translation.z), rotation,
                sized(scale, values["size"], values["disc"], values["disc"]))
    if bone == TOP:
        return (unreal.Vector(translation.x, translation.y + values["shake"], translation.z), rotation,
                sized(scale, values["size"]))
    if bone == "Hourglass":
        return (unreal.Vector(translation.x + values["glass"], translation.y, translation.z),
                unreal.Rotator(pitch=values["pitch"]).quaternion(), scale * values["glass_size"])
    return translation, rotation, scale


KEYS = {"Square": square_key, "Triangle": triangle_key, "Circle": circle_key}
STATES = {"Square": square_state,
          "Triangle": lambda frame: state(CLIPS["Triangle"], frame),
          "Circle": circle_state}
LENGTHS = {"Square": FRAMES + SQ_SHOT_HOLD, "Triangle": FRAMES, "Circle": FRAMES}


def build(toolkit, badge):
    skeleton_path = "{}/SK_{}Badge".format(MESH_FOLDER, badge)
    package = "{}/{}".format(ANIM_FOLDER, badge)
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(skeleton_path))
    sequence = toolkit["get_or_create_asset"](package, "SK_{}Badge_Sequence_Deploy".format(badge), unreal.AnimSequence,
                                              factory)
    toolkit["write_bone_tracks"](sequence, skeleton_path, FPS, LENGTHS[badge], KEYS[badge],
                                 "Build {} badge deploy".format(badge.lower()))

    montage_name = "SK_{}Badge_Montage_Deploy".format(badge)
    montage = toolkit["build_montage"](sequence, package, montage_name, ["Start", "End"], [0.0, START / float(FPS)],
                                       ["End", "None"], SLOT)
    for name in ("blend_in", "blend_out"):
        blend = montage.get_editor_property(name)
        blend.set_editor_property("blend_time", BLEND)
        montage.set_editor_property(name, blend)
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(package, montage_name))
    return sequence, montage


def measure_square(toolkit):
    """How far behind its bone the block's back and a mandible's back sit, which the stretch and the crush pin; where
    the ledge is, the block's side at its front corner; and a mandible's half width and resting place across."""
    groups = toolkit["rigid_vertex_groups"](MESH_FOLDER + "/SKM_SquareBadge", MESH_FOLDER + "/SK_SquareBadge")
    reference = toolkit["_component_transforms"](APE.get_reference_pose(unreal.load_asset(MESH_FOLDER +
                                                                                           "/SK_SquareBadge")))
    front = max(vertex.x for vertex in groups[BODY])
    GEOMETRY.update({"block_back": -min(vertex.x for vertex in groups[BODY]),
                     "jaw_back": -min(vertex.x for vertex in groups["MandibleRight"]),
                     "ledge": max(vertex.y for vertex in groups[BODY] if vertex.x > front - 1.0)
                     + reference[BODY].translation.y,
                     "jaw_half": max(vertex.y for vertex in groups["MandibleRight"]),
                     "jaw_across": reference["MandibleRight"].translation.y})
    LOG.append("ledge at y %.2f" % GEOMETRY["ledge"])


def ledge_out(jaw_y):
    """Units out that lay a mandible `jaw_y` wide with its outer side on the ledge."""
    return GEOMETRY["ledge"] - GEOMETRY["jaw_half"] * jaw_y - GEOMETRY["jaw_across"]


def load_generator():
    path = unreal.Paths.project_dir() + GENERATOR
    generator = {"__name__": "badge_generator"}
    exec(compile(open(path).read(), path, "exec"), generator)
    return generator


def clearances(toolkit, badge, placed, posed, reference, body):
    """What each clip can collide on a frame, in units: below 0 overlaps, 0 for sunk depth is clear."""
    hull = {bone: toolkit["convex_hull"](points) for bone, points in placed.items() if bone != BODY}
    if badge == "Square":
        return {"to block": min(toolkit["outline_separation"](hull[jaw], body) for jaw in JAWS),
                "between": toolkit["outline_separation"](hull["MandibleLeft"], hull["MandibleRight"]),
                "past ledge": max(vertex.y for vertex in placed["MandibleRight"]) - GEOMETRY["ledge"]}
    if badge == "Triangle":
        return {"plug to hole": toolkit["outline_separation"](hull["Plug"], body),
                "needle sunk": toolkit["sunk_depth"](placed["Needle"], GEOMETRY["outline"], 0.0, 0.0, reference[BODY],
                                                     posed[BODY], GEOMETRY["half_height"])}
    return {"to disc": toolkit["outline_separation"](hull["Hourglass"], body)}


def report(toolkit, badge, sequence, montage, generator):
    skeleton = unreal.load_asset("{}/SK_{}Badge".format(MESH_FOLDER, badge))
    groups = toolkit["rigid_vertex_groups"]("{}/SKM_{}Badge".format(MESH_FOLDER, badge),
                                            "{}/SK_{}Badge".format(MESH_FOLDER, badge))
    reference = toolkit["_component_transforms"](APE.get_reference_pose(skeleton))
    outline = generator["to_world"]("SM_{}Badge".format(badge))[0].outline
    if badge == "Triangle":
        back, front = min(p[0] for p in outline), max(p[0] for p in outline)
        body_half = 0.5 * generator["BODY_HEIGHT"]["SM_TriangleBadge"]
        GEOMETRY.update({"outline": outline,
                         "half_height": lambda x: body_half * generator["wedge"](x, back, front)})

    frames = LENGTHS[badge]
    LOG.append("{} - {} sampled keys for {} frames, sections {}, slots {}".format(
        sequence.get_name(), toolkit["playable_key_count"](sequence), frames, toolkit["montage_sections"](montage),
        [str(slot) for slot in unreal.AnimationLibrary.get_montage_slot_names(montage)]))
    options = unreal.AnimPoseEvaluationOptions()
    rest = toolkit["local_pose_table"](APE.get_reference_pose(skeleton))
    worst, ends = {}, {}
    for frame in range(frames + 1):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        if frame in (0, frames):
            ends[frame] = toolkit["local_pose_table"](pose)
        posed = toolkit["_component_transforms"](pose)
        placed = toolkit["place_groups"](groups, posed)
        body = toolkit["move_outline"](outline, reference[BODY], posed[BODY])
        measured = clearances(toolkit, badge, placed, posed, reference, body)
        for name, value in measured.items():
            worst[name] = (max if name in ("needle sunk", "past ledge") else min)(worst.get(name, value), value)
        channels = STATES[badge](frame)
        LOG.append("%3d %-5s %s | %s" % (
            frame, "Start" if frame < START else "End",
            " ".join("%s %+.2f" % (name, value) for name, value in sorted(channels.items())),
            " ".join("%s %+.2f" % item for item in measured.items())))
    LOG.append("worst: " + ", ".join("%s %+.2f" % item for item in worst.items()))
    if badge == "Square":
        steps = [square_roll(frame) - square_roll(frame - 1) for frame in range(1, START)]
        steps.append(360.0 * SQ_TURNS - square_roll(START - 1))
        LOG.append("fastest roll %.1f deg/frame (reads backwards past %.0f); %.0f deg rolled by a 0.1 s tap" % (
            max(steps), SQ_STROBE, square_roll(int(0.1 * 2 * FPS))))
    for frame, table in ends.items():
        off = max(abs(a - b) for bone in rest for part in (0, 2) for a, b in zip(table[bone][part], rest[bone][part]))
        turned = max(abs(math.remainder(table[bone][1] - rest[bone][1], 360.0)) for bone in rest)
        LOG.append("frame %d off the reference pose by %.4f units/scale, %.4f deg yaw" % (frame, off, turned))
    LOG.append("")


try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
    badge_generator = load_generator()
    measure_square(toolkit)
    for badge_name in KEYS:
        built_sequence, built_montage = build(toolkit, badge_name)
        report(toolkit, badge_name, built_sequence, built_montage, badge_generator)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
