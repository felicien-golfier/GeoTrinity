"""Triangle badge auto-fire, three heavy cuts of one shot. The badge carries two diamonds: the plug, just filling the hole
through the arrowhead, and the needle, its twin, standing above it clear of the body. From above they read as one until
the needle moves; the needle may shrink, and the plug may be released from its hole once lifted above the body.

    Fire            the plug shudders back into its hole until it knocks against the rim while the needle over it
                    shrinks and spins up; both stop dead, then the plug rams the front rim — the shot — as the needle
                    punches out ahead at full size; the kick jams the plug there and holds the needle out
    FireRatchet     the plug is cranked back in three clicks, the needle turning a sixth and shrinking a step on each,
                    the badge jolting on every one; on the shot the needle is flung out and snaps back, and the kick
                    twists the badge
    FireFormation   the needle jumps out to one side and the plug rises out of its hole and jumps to the other, each
                    round an arc of a circle that brings it in beside the arrowhead's tip, spinning once clear of the
                    body; they lie there as a wider arrowhead round the tip, trembling. The shot folds them back along
                    its edges, thrown back and spinning a half turn, and they open out again. Stopped, they fly home
                    round the same arcs, the plug first

GA_Triangle_AutoProjectile fires every FireDelay and plays one section per shot, stretched to that delay, with the shot
landing on the section's last frame from anim_socket_<n> on the root, which no clip moves:

    Fire1   the kick from the last shot, wound again, loops on itself while firing lasts
    End     the kick from the last shot, and home               (firing stopped)
    Start   wound from rest                                      -> End

The sequence holds them in that order, the one where each begins on the pose its neighbour ends on; the impact is the
first frame of Fire1 and End, and the last of Start and Fire1. A plug's roll and a needle's yaw only grow, by whole
quarter and half turns — what their square section and their rhombus outline cannot show — and End turns both on to a
whole turn, so the blend back out never shows one.

In its hole the plug only slides, shakes across it and rolls: CONTACT is the slide laying its tip on the rim, and a
shake is held to the room its slide leaves, since its sides run parallel to the hole's. The kick moves the arrowhead
and the top layer together, so it costs no clearance. Every frame is checked in three dimensions: each diamond against
the arrowhead's sloping top, and the two against each other.

A beat is authored at 30 fps and sped up to the ability's delay by the play rate; End runs at that same rate. The
montage goes in the Top slot.

Run AFTER AI/Python/Mesh/rig_class_badges.py, via mcp-unreal execute_script, then AI/Python/Anim/class_badge_wiring.py.
Re-runnable: rewrites the sequences and montages in place. Report written to Saved/class_badge_triangle_fire.txt.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/Class/SK_TriangleBadge"
MESH_PATH = "/Game/Characters/Meshes/Class/SKM_TriangleBadge"
GENERATOR = "AI/Python/Mesh/generate_class_badge_meshes.py"
ANIM_PACKAGE = "/Game/Characters/Anim/ClassBadge/Triangle"
REPORT = unreal.Paths.project_saved_dir() + "class_badge_triangle_fire.txt"

BODY, TOP, PLUG, NEEDLE = "Bottom", "Top", "Plug", "Needle"
SLOT = "Top"

FPS = 30
BEAT = 18        # sped up to the ability's 0.3 s FireDelay
WOUND = 13       # the wind-up's last frame; then STILL frames dead still, one mid-flight, and the impact on BEAT
STILL = 3
MID_FLIGHT = 0.4  # share of the way to the impact crossed on the one frame in flight
TOUCH = 0.05     # units short of the rim a touch stops, so it never reads as sunk into it
SHAKE = 0.8      # share of the room across the hole a shake uses
STROBE = {"roll": 45.0, "yaw": 90.0}  # half a turn's symmetry: past this per frame a spin reads as going backwards

SETTLE_POWER = 1.5   # how fast a kick's swing back dies
SETTLE_SPRING = 3.4  # sets where it crosses rest: once, a little past halfway home

SECTIONS = [("Fire1", "End"), ("End", "None"), ("Start", "End")]
FRAMES = BEAT * len(SECTIONS)
FOLLOWERS = {"Start": ["Fire1", "End"], "Fire1": ["Fire1", "End"]}

# A part's pose: forward, sideways and up from its rest, its yaw and roll in degrees, and its scale.
REST = (0.0, 0.0, 0.0, 0.0, 0.0, 1.0)

# Per clip:
#   kick      (backward offset, yaw) of the whole badge from the impact on, held on its last entry, then sprung home
#   plug      the plug's slide on the same frames, as a share of CONTACT: 1 is pressed on the front rim
#   windup    "shudder", "ratchet" or "formation"
#   roll      degrees the plug rolls per wind-up
#   knock     units the plug springs off the back rim between knocks, and the badge's jolt on each
#   needle    reach: units it is thrown ahead of the plug on the shot, out: its share of that on the kick's frames,
#             spin: degrees it turns per wind-up, small: the scale it winds down to, steps: its scale after each click
CLIPS = {
    "Fire": {
        "kick": [(0.0, 0.0), (-5.0, 0.0), (-7.7, 0.0), (-7.0, 0.0), (-7.0, 0.0)],
        "plug": [1.0, 1.0, 1.0, 1.0, 1.0],
        "windup": "shudder",
        "roll": {"Start": 180.0, "Fire1": 90.0},
        "knock": (0.7, -0.6),
        "needle": {"reach": 18.0, "out": [1.0, 1.0, 1.0, 1.0, 1.0], "spin": {"Start": 360.0, "Fire1": 180.0},
                   "small": 0.55},
    },
    "FireRatchet": {
        "kick": [(0.0, 0.0), (-4.0, 3.0), (-6.0, 4.8), (-5.4, 4.3), (-5.4, 4.3)],
        "plug": [1.0, 0.45, 0.1, 0.0, 0.0],
        "windup": "ratchet",
        "roll": {"Start": 90.0, "Fire1": 90.0},
        "clicks": {"Start": [3, 7, 11], "Fire1": [5, 8, 11]},
        "click_jolt": -1.4,
        "knock": (0.6, -0.5),
        "needle": {"reach": 12.0, "out": [1.0, 1.08, 0.6, 0.15, 0.0], "spin": {"Start": 180.0, "Fire1": 180.0},
                   "steps": [1.0, 0.82, 0.66, 0.5]},
    },
    "FireFormation": {
        "kick": [(0.0, 0.0), (-2.5, 0.0), (-3.4, 0.0), (-3.0, 0.0), (-3.0, 0.0)],
        "windup": "formation",
    },
}

# FireFormation. Each diamond lies with its inner tip FORM_TIP ahead of the arrowhead's tip and out to its side, turned
# back from there by an angle off the aim: FORM_OPEN wound, the widest it ever gets, and FORM_BLOWN once the shot has
# folded it back along the arrowhead's edge. The body hides whatever is over it, so a diamond only turns on itself
# once its flight has carried it clear.
FORM_TIP = (5.0, 6.5)
FORM_OPEN, FORM_BLOWN = 60.0, 45.0
FORM_HEIGHT = 10.0
FORM_JUMP = 30.0      # the height the plug leaves its hole to, clear of the body before it crosses it
FORM_TREMBLE = 0.8    # units each trembles outward, wound
FORM_THROW = (3.0, 5.0)  # units the shot throws each back and out, so its spin keeps clear of the arrowhead
FORM_PUSH = [0.6, 1.0, 0.7, 0.3, 0.0]   # share of that throw on the kick's frames
FORM_SPIN = [0.0, 0.3, 0.65, 0.9, 1.0]  # share of a half turn each spins on itself on the same frames
FORM_CLEAR = 0.3      # share of its flight a diamond spends over the body, where a turn would not show

LOG = []
GEOMETRY = {}   # filled from the generator and the rig before any key is written


def clamp(alpha):
    return min(1.0, max(0.0, alpha))


def smoothstep(alpha):
    alpha = clamp(alpha)
    return alpha * alpha * (3.0 - 2.0 * alpha)


def alternate(frame):
    return 1.0 if frame % 2 else -1.0


def lerp(a, b, alpha):
    return tuple(x + (y - x) * alpha for x, y in zip(a, b))


def track(keys, local):
    """A pose eased between (frame, pose) keys; held before the first and after the last."""
    if local <= keys[0][0]:
        return keys[0][1]
    for (before, start), (after, end) in zip(keys, keys[1:]):
        if before <= local <= after:
            return lerp(start, end, smoothstep((local - before) / float(after - before)))
    return keys[-1][1]


def settle(frames_since):
    """Share of a kick still held, `frames_since` its hold: crosses rest once and is still by WOUND."""
    alpha = frames_since / float(WOUND - len(CLIP["kick"]) + 1)
    return 0.0 if alpha >= 1.0 else (1.0 - alpha) ** SETTLE_POWER * math.cos(SETTLE_SPRING * alpha)


def kick(local):
    """(backward offset, yaw) of the whole badge `local` frames after an impact."""
    table = CLIP["kick"]
    if local < len(table):
        return table[local]
    remaining = settle(local - len(table) + 1)
    return tuple(value * remaining for value in table[-1])


def back_rim_knock(local):
    """Pressed on the back rim, knocking off it every other frame, landing pressed on WOUND -> (slide, jolt)."""
    spring, jolt = CLIP["knock"]
    pressed = (WOUND - local) % 2 == 0
    return (-GEOMETRY["contact"] if pressed else -GEOMETRY["contact"] + spring), (jolt if pressed else 0.0)


def shake(slide, amount, frame):
    """A sideways shake across the hole, `amount` of the room the slide leaves."""
    return alternate(frame) * amount * SHAKE * GEOMETRY["side_room"] * (GEOMETRY["contact"] - abs(slide))


def shudder(local, first, slide_from, frame, spins):
    """The plug slides back onto the back rim, shaking harder as it goes, and knocks against it once there.

    `spins` is the fraction of the turn done per wind-up frame. -> (slide, sideways, fraction of the turn done, jolt).
    """
    alpha = clamp((local - first) / float(WOUND - first))
    arrive = 0.7
    spin = spins[local - first]
    if alpha >= arrive:
        slide, jolt = back_rim_knock(local)
        return slide, 0.0, spin, jolt
    slide = slide_from + (-GEOMETRY["contact"] - slide_from) * (alpha / arrive) ** 2
    return slide, shake(slide, alpha / arrive, frame), spin, 0.0


def ratchet(local, clicks, slide_from, frame):
    """The plug cranked back a third of the way per click, a tooth of the turn with it; trembling between clicks,
    knocking once home. -> (slide, sideways, fraction of the turn done, jolt)."""
    done = sum(1 for click in clicks if click <= local)
    if done == len(clicks) and local > clicks[-1]:
        slide, jolt = back_rim_knock(local)
        return slide, 0.0, 1.0, jolt
    slide = slide_from + (-GEOMETRY["contact"] - slide_from) * done / float(len(clicks))
    jolt = CLIP["click_jolt"] * (1.0 if local in clicks else 0.4 if local - 1 in clicks else 0.0)
    sideways = shake(slide, 0.5, frame) if 0 < done < len(clicks) and local not in clicks else 0.0
    return slide, sideways, done / float(len(clicks)), jolt


def wound_state(section, local, frame):
    """Fire and FireRatchet -> (backward kick, badge yaw, plug pose, needle pose)."""
    contact = GEOMETRY["contact"]
    needle = CLIP["needle"]
    rolls, spins = CLIP["roll"], needle["spin"]
    roll_before = {"Fire1": 0.0, "End": rolls["Fire1"], "Start": 360.0}[section]
    yaw_before = {"Fire1": 0.0, "End": spins["Fire1"], "Start": 360.0}[section]
    kicked = section != "Start"
    held = len(CLIP["plug"])
    back, yaw = kick(local) if kicked else (0.0, 0.0)
    small = needle.get("small") or needle["steps"][-1]
    impact = ((contact, 0.0, 0.0, 0.0, roll_before + rolls.get(section, 0.0), 1.0),
              (contact + needle["reach"], 0.0, 0.0, yaw_before + spins.get(section, 0.0), 0.0, 1.0))

    if kicked and local < held:
        slide = contact * CLIP["plug"][local]
        return (back, yaw, (slide, 0.0, 0.0, 0.0, roll_before, 1.0),
                (slide + needle["reach"] * needle["out"][local], 0.0, 0.0, yaw_before, 0.0, 1.0))
    slide_from = contact * CLIP["plug"][-1] if kicked else 0.0
    out_from = needle["reach"] * needle["out"][-1] if kicked else 0.0
    if section == "End":
        home = max(0.0, settle(local - held + 1))
        turned = smoothstep((local - held + 1) / float(BEAT - held + 1))
        return (back, yaw, (slide_from * home, 0.0, 0.0, 0.0, roll_before + (360.0 - roll_before) * turned, 1.0),
                ((slide_from + out_from) * home, 0.0, 0.0, yaw_before + (360.0 - yaw_before) * turned, 0.0, 1.0))

    if local == BEAT:
        return (back, yaw) + impact   # Start's last frame
    still_plug = (-contact, 0.0, 0.0, 0.0, roll_before + rolls[section], 1.0)
    still_needle = (-contact, 0.0, 0.0, yaw_before + spins[section], 0.0, small)
    if local > WOUND + STILL:
        return (back, yaw, lerp(still_plug, impact[0], MID_FLIGHT), lerp(still_needle, impact[1], MID_FLIGHT))
    if local > WOUND:
        return back, yaw, still_plug, still_needle

    first = held - 1 if kicked else 0
    returning = out_from * max(0.0, settle(local - held + 1)) if kicked else 0.0
    if CLIP["windup"] == "shudder":
        slide, sideways, spun, jolt = shudder(local, first, slide_from, frame, SPIN[section])
        scale = 1.0 + (small - 1.0) * smoothstep((local - first) / float(WOUND - first))
    else:
        slide, sideways, spun, jolt = ratchet(local, CLIP["clicks"][section], slide_from, frame)
        scale = needle["steps"][int(round(spun * (len(needle["steps"]) - 1)))]
    return (back + jolt, yaw, (slide, sideways, 0.0, 0.0, roll_before + rolls[section] * spun, 1.0),
            (slide + returning, 0.0, 0.0, yaw_before + spins[section] * spun, 0.0, scale))


def formed(side, angle, rest_height, push=0.0, turned=0.0, roll=0.0, tremble=0.0):
    """A diamond laid out by the arrowhead's tip to `side` (+1 the needle's, -1 the plug's), turned back `angle` degrees
    off the aim about its inner tip, thrown `push` of FORM_THROW and `tremble` further out, spun `turned` more on itself
    -> its pose from its rest."""
    radians = math.radians(angle)
    length = GEOMETRY["half_length"]
    tip_forward = GEOMETRY["apex"] + FORM_TIP[0] - push * FORM_THROW[0]
    tip_out = FORM_TIP[1] + push * FORM_THROW[1] + tremble
    return (tip_forward - length * math.cos(radians) - GEOMETRY["centre"], side * (tip_out + length * math.sin(radians)),
            FORM_HEIGHT - rest_height, -side * (angle + turned), side * roll, 1.0)


def arc(side, target, alpha):
    """`alpha` of the way from the hole to `target` (forward, sideways) round a circle leaving the hole straight out to
    `side`, so the flight bows out and comes in on the arrowhead's tip from beside it -> (forward, sideways)."""
    forward, sideways = target[0], target[1]
    radius = (forward * forward + sideways * sideways) / (2.0 * forward)
    end = math.atan2(sideways, forward - radius) + (0.0 if side > 0 else 2.0 * math.pi)
    angle = math.pi + (end - math.pi) * alpha
    return radius + radius * math.cos(angle), radius * math.sin(angle)


def over_body(alpha):
    """0 while a flight still has the diamond over the body, easing to 1 as it arrives."""
    return smoothstep((alpha - FORM_CLEAR) / (1.0 - FORM_CLEAR))


def fly_out(side, rest_height, up_from, roll, alpha):
    """Out of the hole to the wound formation: it drops toward FORM_HEIGHT and spins once clear of the body."""
    target = formed(side, FORM_OPEN, rest_height, turned=180.0, roll=roll)
    forward, sideways = arc(side, target, alpha)
    # The flight's own easing already slows it into the formation, so the spin need not ease as well.
    spin = clamp((alpha - FORM_CLEAR) / (1.0 - FORM_CLEAR))
    return forward, sideways, up_from + (target[2] - up_from) * over_body(alpha), target[3] * spin, side * roll, 1.0


def fly_home(side, rest_height, up_to, alpha):
    """From blown back by the arrowhead's tip to over the hole along the same circle: it climbs and straightens while
    still clear of the body."""
    start = formed(side, FORM_BLOWN, rest_height, turned=360.0)
    forward, sideways = arc(side, start, 1.0 - alpha)
    rising = smoothstep(alpha / (1.0 - FORM_CLEAR))
    return (forward, sideways, start[2] + (up_to - start[2]) * rising,
            start[3] + (-side * 360.0 - start[3]) * rising, 0.0, 1.0)


def formation_state(section, local, frame):
    """FireFormation -> (backward kick, badge yaw, plug pose, needle pose)."""
    lift = GEOMETRY["lift"]
    kicked = section != "Start"
    back, yaw = kick(local) if kicked else (0.0, 0.0)
    held = len(FORM_PUSH)
    # Half turns spun before this section's own, and what it has spun once wound. A roll comes only with the flight out.
    before = {"Fire1": 0.0, "End": 180.0, "Start": 180.0}[section]
    spun = before + 180.0 if kicked else before
    roll = 0.0 if kicked else 180.0

    def both(angle, push=0.0, turned=spun, tremble=0.0):
        return (formed(-1.0, angle, 0.0, push, turned, roll, tremble),
                formed(1.0, angle, lift, push, turned, roll, tremble))

    if kicked and local < held:
        return (back, yaw) + both(FORM_BLOWN, FORM_PUSH[local], before + 180.0 * FORM_SPIN[local])
    impact = both(FORM_BLOWN, FORM_PUSH[0])
    if local == BEAT:
        return (back, yaw) + impact   # Start's last frame
    if section == "End":
        # The plug home first, dropping back into its hole before the needle comes in over it.
        if local <= 10:
            plug = fly_home(-1.0, 0.0, FORM_JUMP, smoothstep((local - held + 1) / 6.0))
        else:
            plug = (0.0, 0.0, FORM_JUMP * (1.0 - smoothstep((local - 10) / 3.0)), 360.0, 0.0, 1.0)
        if local <= 7:
            needle = formed(1.0, FORM_BLOWN, lift, turned=360.0)
        else:
            needle = fly_home(1.0, lift, 0.0, smoothstep((local - 7) / 8.0))
        return back, yaw, plug, needle
    if local > WOUND + STILL:
        return (back, yaw) + tuple(lerp(wound, hit, MID_FLIGHT) for wound, hit in zip(both(FORM_OPEN), impact))
    if local > WOUND:
        return (back, yaw) + both(FORM_OPEN)

    tremble = FORM_TREMBLE * clamp((local - 9) / 4.0) * (1.0 if frame % 2 else 0.0)
    if kicked:
        angle = track([(held - 1, (FORM_BLOWN,)), (9, (FORM_OPEN,))], local)[0]
        return (back, yaw) + both(angle, tremble=tremble)
    # The needle leaves first, so the plug can rise straight up out of its hole, rolling, before it follows.
    wound_plug, wound_needle = both(FORM_OPEN, tremble=tremble)
    if local >= 12:
        plug = wound_plug
    elif local >= 4:
        plug = fly_out(-1.0, 0.0, FORM_JUMP, 180.0 * clamp((local - 2) / 10.0), smoothstep((local - 4) / 8.0))
    else:
        plug = (0.0, 0.0, FORM_JUMP * smoothstep((local - 2) / 2.0), 0.0, -180.0 * clamp((local - 2) / 10.0), 1.0)
    needle = wound_needle if local >= 9 else fly_out(1.0, lift, 0.0, 180.0 * local / 9.0, smoothstep(local / 9.0))
    return back, yaw, plug, needle


def state(frame):
    """(backward kick, badge yaw, plug pose, needle pose) at a frame of the whole sequence."""
    index = min(frame // BEAT, len(SECTIONS) - 1)
    local = frame - index * BEAT
    section = SECTIONS[index][0]
    if CLIP["windup"] == "formation":
        return formation_state(section, local, frame)
    return wound_state(section, local, frame)


def key(frame, bone, rest_local):
    translation, rotation, scale = rest_local.translation, rest_local.rotation, rest_local.scale3d
    back, turn, plug, needle = state(frame)
    if bone in (TOP, BODY):
        # Both layers stand on the badge's centre, so one yaw turns them about the same point.
        return (unreal.Vector(translation.x + back, translation.y, translation.z),
                unreal.Rotator(yaw=turn).quaternion(), scale)
    if bone in (PLUG, NEEDLE):
        forward, sideways, up, yaw, roll, size = plug if bone == PLUG else needle
        return (unreal.Vector(translation.x + forward, translation.y + sideways, translation.z + up),
                unreal.Rotator(roll=roll, yaw=yaw).quaternion(), scale * size)
    return translation, rotation, scale


def build(toolkit, name):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    sequence = toolkit["get_or_create_asset"](ANIM_PACKAGE, "SK_TriangleBadge_Sequence_" + name, unreal.AnimSequence,
                                              factory)
    toolkit["write_bone_tracks"](sequence, SKELETON_PATH, FPS, FRAMES, key, "Build triangle badge " + name)

    montage_name = "SK_TriangleBadge_Montage_" + name
    montage = toolkit["build_montage"](sequence, ANIM_PACKAGE, montage_name, [section for section, _ in SECTIONS],
                                       [index * BEAT / float(FPS) for index in range(len(SECTIONS))],
                                       [following for _, following in SECTIONS], SLOT)
    # A new shot replays the montage every beat, so any blend-in would smear one beat into the next.
    for blend_name, seconds in (("blend_in", 0.0), ("blend_out", 0.1)):
        blend = montage.get_editor_property(blend_name)
        blend.set_editor_property("blend_time", seconds)
        montage.set_editor_property(blend_name, blend)
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(ANIM_PACKAGE, montage_name))
    return sequence, montage


def report(toolkit, sequence, montage, groups, reference):
    """Every frame: the plug's gap to its hole from above, how deep either diamond sinks into the arrowhead, and how
    close the two come — below 1 they overlap."""
    samples = toolkit["diamond_samples"](GEOMETRY["half_length"], GEOMETRY["half_width"], GEOMETRY["half_width"])
    size = (GEOMETRY["half_length"], GEOMETRY["half_width"], GEOMETRY["half_width"])
    options = unreal.AnimPoseEvaluationOptions()
    rows = {}
    for frame in range(FRAMES + 1):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        table = toolkit["local_pose_table"](pose)
        posed = toolkit["_component_transforms"](pose)
        points = {bone: [unreal.MathLibrary.transform_location(posed[bone], sample) for sample in samples]
                  for bone in (PLUG, NEEDLE)}
        hull = toolkit["convex_hull"](toolkit["place_groups"](groups, posed)[PLUG])
        hole = toolkit["outline_separation"](hull, toolkit["move_outline"](GEOMETRY["outline"], reference[BODY],
                                                                           posed[BODY]))
        sunk = [toolkit["sunk_depth"](points[bone], GEOMETRY["outline"], 0.0, 0.0, reference[BODY], posed[BODY],
                                      GEOMETRY["half_height"]) for bone in (PLUG, NEEDLE)]
        between = min(min(toolkit["diamond_reach"](point, posed[NEEDLE], *size) for point in points[PLUG]),
                      min(toolkit["diamond_reach"](point, posed[PLUG], *size) for point in points[NEEDLE]))
        rows[frame] = (table, hole, sunk, between)

    LOG.append("{} - {} frames at {} fps, {} sampled keys (expect {}), sections {}".format(
        sequence.get_name(), FRAMES, FPS, toolkit["playable_key_count"](sequence), FRAMES + 1,
        toolkit["montage_sections"](montage)))
    LOG.append("frame section  kick  yaw | plug: fwd   side    up  roll | needle: fwd   side    up   yaw  scale |"
               "  hole  sunk p/n  apart")
    for frame in range(FRAMES + 1):
        back, turn, plug, needle = state(frame)
        _, hole, sunk, between = rows[frame]
        LOG.append("%5d %-7s %+5.1f %+4.1f | %+6.2f %+5.2f %+5.1f %5.0f | %+6.2f %+6.2f %+5.1f %5.0f %5.2f |"
                   " %+5.2f %4.2f %4.2f  %5.2f" % (
                       frame, SECTIONS[min(frame // BEAT, len(SECTIONS) - 1)][0], back, turn,
                       plug[0], plug[1], plug[2], plug[4], needle[0], needle[1], needle[2], needle[3], needle[5],
                       hole, sunk[0], sunk[1], between))
    LOG.append("deepest into the arrowhead: plug %.2f, needle %.2f; closest the two come %.2f (below 1 overlaps)" % (
        max(row[2][0] for row in rows.values()), max(row[2][1] for row in rows.values()),
        min(row[3] for row in rows.values())))
    for name, index in (("roll", 4), ("yaw", 3)):
        steps = [abs(state(frame)[part][index] - state(frame - 1)[part][index])
                 for part in (2, 3) for frame in range(1, FRAMES + 1) if frame % BEAT]
        LOG.append("fastest %s %.1f deg/frame (reads backwards past %.0f)" % (name, max(steps), STROBE[name]))
    starts = {name: index * BEAT for index, (name, _) in enumerate(SECTIONS)}
    for section, followers in FOLLOWERS.items():
        end = rows[starts[section] + BEAT][0]
        for follower in followers:
            start = rows[starts[follower]][0]
            worst = max(abs(a - b) for bone in end for part in (0, 2)
                        for a, b in zip(end[bone][part], start[bone][part]))
            LOG.append("%s end -> %s start: largest jump %.4f" % (section, follower, worst))
    LOG.append("")


def measure(reference):
    """The hole, the diamonds and the arrowhead, from the generator and the rig."""
    path = unreal.Paths.project_dir() + GENERATOR
    generator = {"__name__": "badge_generator"}
    exec(compile(open(path).read(), path, "exec"), generator)
    scale = generator["frame"]("SM_TriangleBadge")[2]
    hole_tip = generator["needle_kite"](generator["NEEDLE_GAP"])[0]
    tip, right, _, _ = generator["needle_kite"](0.0)
    apex, base_right = generator["TRI_OUTLINE"][0], generator["TRI_OUTLINE"][1]
    outline = generator["to_world"]("SM_TriangleBadge")[0].outline
    back, front = min(p[0] for p in outline), max(p[0] for p in outline)
    body_half = 0.5 * generator["BODY_HEIGHT"]["SM_TriangleBadge"]
    return {
        "contact": (tip[1] - hole_tip[1]) * scale - TOUCH,
        "side_room": (base_right[0] - apex[0]) / (base_right[1] - apex[1]),
        "half_length": (right[1] - tip[1]) * scale,
        "half_width": (right[0] - tip[0]) * scale,
        "lift": generator["NEEDLE_LIFT"],
        "outline": outline,
        "apex": front,
        "centre": reference[PLUG].translation.x,
        "half_height": lambda x: body_half * generator["wedge"](x, back, front),
    }


def spins(toolkit):
    """Per section, the fraction of its turn done on each frame of its wind-up, accelerating into the stillness."""
    firsts = {"Start": 0, "Fire1": len(CLIP.get("plug", CLIP["kick"])) - 1}
    return {section: toolkit["normalised_spin"](lambda frame: frame * frame, WOUND - first)
            for section, first in firsts.items()}


try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    rig_groups = toolkit["rigid_vertex_groups"](MESH_PATH, SKELETON_PATH)
    rest_pose = toolkit["_component_transforms"](APE.get_reference_pose(unreal.load_asset(SKELETON_PATH)))
    GEOMETRY.update(measure(rest_pose))
    LOG.append("contact %.2f units, %.3f across per unit of slide; diamonds %.2f x %.2f half, needle %.0f up" % (
        GEOMETRY["contact"], GEOMETRY["side_room"], GEOMETRY["half_length"], GEOMETRY["half_width"], GEOMETRY["lift"]))
    for CLIP_NAME, CLIP in CLIPS.items():
        SPIN = spins(toolkit)
        built_sequence, built_montage = build(toolkit, CLIP_NAME)
        report(toolkit, built_sequence, built_montage, rig_groups, rest_pose)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
