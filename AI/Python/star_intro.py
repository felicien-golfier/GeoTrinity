"""Star boss intro: a seed with no points that grows itself one at a time and then goes nova.

The clip opens on almost nothing — the whole star shrunk to a nub a twentieth of the width it idles at, its arms
short and its valleys shallow. It shivers there, swelling, and then a point erupts: the star draws in, stops, and
one tip fires out past where it will sit, whipping sideways as it lands, with the body kicking and the spike ring
growing by its share. The next point comes sooner than the last, and the next sooner still, until they are
machine-gunning out and the star is turning faster with every one it has grown.

With all of them out it turns as one for a beat, then accelerates while the body draws in and the arms thin to
spindles — the points held in and twitching against it — and stops dead. Then it blows: every point thrown far
past what any ability throws them, the body far past what any capsule has to cover, since nothing here is aimed
at a player. It settles back over nearly two seconds and lands exactly on the reference pose, which is where the
idle picks it up.

This is the hex boss's intro told with what the star's rig has. The hex assembles from three pieces that exist
from the first frame; the star has no pieces, so what arrives one at a time is its points, and what stands in for
a piece coming home is a tip firing out of a body that grows to meet it.

Constraints:
- The clip does not start on the reference pose. It starts as a seed, so the montage blends in over nothing.
- It ends on the reference pose with every point flush, so the idle takes over without a pop.
- The net spin is a whole number of the star's own symmetry, so nothing is ever unwound.
- The star never turns more than half that symmetry between two frames, past which it reads as turning back.
- A tip bone only ever pushes its needle out: pulled the other way it would turn its spike inside out, and
  drawing the points in is the ring bone's scale.
- Points erupt on a stride round the ring rather than to the neighbour, so the star wakes all over rather than
  being swept round once.
- Nothing moves or scales on Z, which an orthographic camera down that axis would spend for nothing.

Run via mcp-unreal execute_script. Re-runnable: rewrites the sequence and the montage in place.
Report written to REPORT.
"""
import collections
import math

import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/Star/SK_Star"
MESH_PATH = "/Game/Characters/Meshes/Star/SKM_Star"
ANIM_PACKAGE = "/Game/Characters/Anim/Star"
SEQ_NAME = "SK_Star_Sequence_Intro"
MONTAGE_NAME = "SK_Star_Montage_Intro"
REPORT = ("C:/Users/Felou/AppData/Local/Temp/claude/C--GeoTrinity/"
          "d01a11ac-5e89-4af5-9d0a-7a9d27ea8837/scratchpad/star_intro.txt")

SPIKE_PREFIX = "apexe_outside_"
RING_BONE = "apexes_outside"
WAIST_BONE = "apexes_inside"
BODY_BONE = "Root"
VIEW_WIDTH = 3000.0  # the camera volumes' default OrthoWidth, to read the peak silhouette against

FPS = 30
SAMPLE = 4           # frames between report rows

# --- The seed and the stirring: what the star is worth before any point has come out ----------------
WAKE = 8             # frames of dead stillness before the seed stirs
SEED_BODY, WOKEN_BODY = 0.14, 0.62    # scale of the whole star: a nub, then what the stirring alone gets it to
SEED_RING, WOKEN_RING = 0.30, 0.72    # scale of the spike ring — half of this reaches the silhouette, no more
SEED_WAIST, WOKEN_WAIST = 0.50, 0.80  # scale of the valley ring, which about three quarters of does
STIR = 6.0           # units the seed shivers at its loudest
STIR_RAMP = 2.2

# --- The eruptions: one point at a time, each sooner after the last -----------------------------------
FIRST = 34           # frame the first point erupts
GAP, GAP_RATIO, MIN_GAP = 16.0, 0.78, 3  # frames between eruptions, shrinking, floored
GROW = 5             # frames a point's share of the star's size ramps in over
ANTIC = 4            # frames the star draws in ahead of each point
GATHER_DIP = 0.10    # fraction of itself the body pulls in at the tightest of that
# From the frame before the eruption: one mid-flight frame, the overshoot that lands it, then the settle.
STAB = [0.45, 1.10, 0.92, 0.62, 0.38, 0.22, 0.12, 0.05, 0.0]
WHIP = [0.0, 1.0, -0.40, 0.15, -0.05, 0.0]  # of WHIP_REACH, sideways, over those same frames
STAB_REACH = 150.0   # units the tip bone throws on its eruption; its vertex travels half of that
HELD_REACH = 34.0    # units it keeps out afterwards, while the rest of the star is still coming
WHIP_REACH = 26.0    # units it lashes sideways as it lands, the other way from the point before it
KICK_POP = 0.11      # fraction the body swells on an eruption
RING_POP = 0.08      # and the spike ring with it
KICK_SHAKE = 18.0    # units the whole star kicks there
KICK_DECAY = 0.45    # per frame

# --- The finale: turning as one, winding in, blowing open and settling ---------------------------------
ALIVE = 12           # frames every point is out and the star turns as one before the wind-up
WIND = 44            # frames of accelerating wind-up
FREEZE = 4           # frames stopped dead at the tightest of it
BURST = [0.45, 1.30, 1.0]  # the mid-flight frame, the overshoot that lands the nova, then full extension
HOLD = 14            # frames the blown-open star is held past the burst
SETTLE = 52          # frames of the slow spring back into the idle

CHARGE_EASE = 2.4    # above 1 the wind-up accelerates instead of closing at a fixed speed
SHAKE_RAMP = 2.0     # above 1 the wind-up's rattle keeps out of the way until it is nearly tight
WIND_SHAKE = 16.0    # units the star rattles at the tightest of it
TWITCH = 7.0         # units a held-in point stabs on the spot there, its own frames, not its neighbour's
DAMP = 2.4           # how fast the settle dies
SPRING = 3.8         # how far past rest it swings on the way

BODY_CHARGE, BODY_HOLD, BODY_PUNCH = 0.52, 1.32, 1.85  # the whole star wound in, held open, and on the nova
RING_CHARGE, RING_BURST = 1.02, 1.80    # the arms stay long while the body shrinks, which is what thins them
WAIST_CHARGE, WAIST_BURST = 0.42, 1.18  # and the valleys close, which is what makes them spindles
BURST_REACH = 280.0  # units every tip bone throws on the nova, well past what any ability asks of them

NEEDLE_LAG, RING_LAG, BODY_LAG, WAIST_LAG = 0, 1, 1, 2  # the needles lead, the waist drags, as the abilities do
MAX_LAG = max(NEEDLE_LAG, RING_LAG, BODY_LAG, WAIST_LAG)

SPIN_STEPS = 28      # whole turns of the star's own symmetry over the clip; nothing is ever unwound
SEED_SPIN = 1.0      # turning rate with one point out; the spin table normalises the units away
ASSEMBLED_SPIN = 5.0 # rate once every point is out
SPIN_PEAK = 26.0     # rate at the top of the acceleration
BLAST_SPIN = 0.9     # of that peak, the rate through the nova and the hold

# Every derived frame of the clip, plus the spin it integrates: the eruption schedule is as long as the star has
# points, so nothing after it can be a constant.
Clock = collections.namedtuple("Clock", "tips schedule symmetry alive wind_still wind blast plateau frames spun")

LOG = []


def smoothstep(alpha):
    alpha = min(1.0, max(0.0, alpha))
    return alpha * alpha * (3.0 - 2.0 * alpha)


def spike_axes(component, toolkit):
    """Tip bone -> (outward unit x, outward unit y, its place in the eruption order)."""
    names = sorted((name for name in component if name.startswith(SPIKE_PREFIX)),
                   key=lambda name: int(name[len(SPIKE_PREFIX):]))
    axes = {}
    for place, index in enumerate(toolkit["stride_order"](len(names))):
        position = component[names[index]].translation
        length = math.hypot(position.x, position.y)
        axes[names[index]] = (position.x / length, position.y / length, place)
    return axes


def build_clock(axes, toolkit):
    """Every frame the clip turns on, worked out from how many points the star has."""
    schedule = toolkit["accelerating_schedule"](FIRST, len(axes), GAP, GAP_RATIO, MIN_GAP)
    alive = schedule[-1] + GROW + ALIVE
    wind_still = alive + WIND
    wind = wind_still + FREEZE - 1
    blast = wind + 1
    plateau = blast + len(BURST) - 1 + MAX_LAG + HOLD  # full extension outlasts the largest delay
    clock = Clock(len(axes), schedule, 360.0 / len(axes), alive, wind_still, wind, blast, plateau,
                  plateau + SETTLE + MAX_LAG, None)  # the settle gets that delay on top, so every part rests
    # Integrated from the rate rather than written as an angle: the star is then free to accelerate however it
    # likes and still land on a whole multiple of its own symmetry.
    return clock._replace(spun=toolkit["normalised_spin"](
        lambda frame: spin_rate(frame, clock), clock.frames))


def charged(frame, clock):
    """0 to 1 across the wind-up, accelerating; 1 from the frozen frames on."""
    return min(1.0, max(0.0, frame - clock.alive) / float(clock.wind_still - 1 - clock.alive)) ** CHARGE_EASE


def drive(frame, clock):
    """-1 wound tightest, +1 blown open, 0 at rest — the one curve the finale's parts all follow."""
    if frame < clock.wind_still:
        return -charged(frame, clock)
    if frame <= clock.wind:
        return -1.0
    if frame < clock.blast + len(BURST):
        return BURST[frame - clock.blast]
    if frame <= clock.plateau:
        return BURST[-1]
    alpha = (frame - clock.plateau) / float(SETTLE)
    return 0.0 if alpha >= 1.0 else math.exp(-DAMP * alpha) * math.cos(SPRING * alpha) * (1.0 - alpha)


def points_out(frame, clock):
    """How much of the star has come out by `frame`, 0 to 1, each point ramping its share in over GROW frames."""
    return sum(min(1.0, max(0.0, frame - erupt) / float(GROW)) for erupt in clock.schedule) / float(clock.tips)


def stir(frame, clock):
    """0 to 1 across the stirring, before any point has erupted."""
    return smoothstep((frame - WAKE) / float(clock.schedule[0] - WAKE))


def grown(frame, clock, seed, woken):
    """A part's factor as the star wakes: seed to woken across the stirring, then to 1 as the points come out."""
    return seed + (woken - seed) * stir(frame, clock) + (1.0 - woken) * points_out(frame, clock)


def gather(frame, clock):
    """0 to 1: how tightly the star is drawing itself in for the next point to come out."""
    return max([smoothstep((frame - erupt + ANTIC) / float(ANTIC))
                for erupt in clock.schedule if erupt - ANTIC <= frame < erupt] or [0.0])


def kick(frame, clock):
    """0 to 1: the decaying shock of the most recent point erupting."""
    return max([math.exp(-KICK_DECAY * (frame - erupt)) for erupt in clock.schedule if frame >= erupt] or [0.0])


def rattle(frame, clock):
    """The whole star's offset this frame -> (x, y): the seed's shiver, an eruption's kick, the wind-up's load.

    Alternates every frame, since slower reads as a wobble, and is dead through the frozen frames, which is what
    the nova lands against.
    """
    reach = KICK_SHAKE * kick(frame, clock)
    if WAKE <= frame < clock.schedule[0]:
        reach += STIR * stir(frame, clock) ** STIR_RAMP
    if clock.alive <= frame < clock.wind_still:
        reach += WIND_SHAKE * charged(frame, clock) ** SHAKE_RAMP
    return (reach if frame % 2 else -reach), (reach * 0.6 if frame % 4 < 2 else -reach * 0.6)


def spin_rate(frame, clock):
    """How fast the star is turning at `frame`, in units the spin table normalises away.

    While it is assembling the rate is what it has grown: a star with one point creeps and a whole one is already
    moving, so the turn escalates on its own with every point that comes out.
    """
    if frame <= clock.schedule[0]:
        return 0.0
    if frame < clock.alive:
        return SEED_SPIN + (ASSEMBLED_SPIN - SEED_SPIN) * points_out(frame, clock)
    if frame < clock.wind_still:
        return ASSEMBLED_SPIN + (SPIN_PEAK - ASSEMBLED_SPIN) * charged(frame, clock)
    if frame <= clock.wind:
        return 0.0
    if frame <= clock.plateau:
        return SPIN_PEAK * BLAST_SPIN
    return SPIN_PEAK * BLAST_SPIN * (1.0 - smoothstep((frame - clock.plateau) / float(SETTLE)))


def body_yaw(frame, clock):
    """Degrees the star has turned by `frame`, kept rather than unwound."""
    return clock.symmetry * SPIN_STEPS * clock.spun[frame]


def part_scale(frame, clock, seed, woken, charge, burst, lag):
    """One ring's XY scale: what it has grown to as the star wakes, then the finale's curve on top of that."""
    value = drive(frame - lag, clock)
    return grown(frame, clock, seed, woken) * (
        1.0 + (min(1.0, value) * (burst - 1.0) if value > 0.0 else value * (1.0 - charge)))


def body_scale(frame, clock):
    """The whole star's XY scale: drawn in before each point, kicked as it lands, then wound and blown open.

    The overshoot alone drives the punch and everything below it the hold, so the frame nothing could be hit at
    is the nova's and no other.
    """
    value = drive(frame - BODY_LAG, clock)
    if value < 0.0:
        factor = 1.0 + value * (1.0 - BODY_CHARGE)
    else:
        overshoot = max(0.0, value - 1.0) / (max(BURST) - 1.0)
        factor = 1.0 + min(1.0, value) * (BODY_HOLD - 1.0) + overshoot * (BODY_PUNCH - BODY_HOLD)
    return grown(frame, clock, SEED_BODY, WOKEN_BODY) * factor \
        * (1.0 - GATHER_DIP * gather(frame, clock)) * (1.0 + KICK_POP * kick(frame, clock))


def ring_scale(frame, clock):
    return part_scale(frame, clock, SEED_RING, WOKEN_RING, RING_CHARGE, RING_BURST, RING_LAG) \
        * (1.0 + RING_POP * kick(frame, clock))


def waist_scale(frame, clock):
    return part_scale(frame, clock, SEED_WAIST, WOKEN_WAIST, WAIST_CHARGE, WAIST_BURST, WAIST_LAG)


def stab(frame, place, clock):
    """Units this point is showing from its own eruption: the throw, then what it keeps out until the wind-up."""
    step = frame - clock.schedule[place] + 1
    if step < 0 or frame >= clock.wind_still:
        return 0.0
    return HELD_REACH + (STAB_REACH - HELD_REACH) * (STAB[step] if step < len(STAB) else 0.0)


def whip(frame, place, clock):
    """Units this point lashes sideways as it lands, the other way from the point that came out before it."""
    step = frame - clock.schedule[place] + 1
    if step < 0 or step >= len(WHIP):
        return 0.0
    return WHIP_REACH * WHIP[step] * (1.0 if place % 2 else -1.0)


def twitch(frame, place, clock):
    """Units a held-in point stabs on the spot while the star winds up.

    On its own frames rather than its neighbour's, so the star has something moving every frame while each point
    alternates. It only ever pushes out: a tip bone pulled inward turns its spike inside out.
    """
    if not clock.alive <= frame < clock.wind_still:
        return 0.0
    return TWITCH * charged(frame, clock) * ((frame + place) % 2)


def needle(frame, place, clock):
    """Units a tip bone is thrown by: its own eruption, or the nova that throws them all.

    Whatever a point is showing is drawn back in by the wind-up before the nova throws it out again.
    """
    value = drive(frame - NEEDLE_LAG, clock)
    out = stab(frame, place, clock)
    if value < 0.0:
        return out * (1.0 + value) + twitch(frame, place, clock)
    return max(out, min(1.0, value) * BURST_REACH)


def key(frame, bone, rest_local, axes, clock):
    """The bone's local transform at `frame`, as the track writer wants it."""
    translation = unreal.Vector(rest_local.translation.x, rest_local.translation.y, rest_local.translation.z)
    scale = unreal.Vector(rest_local.scale3d.x, rest_local.scale3d.y, rest_local.scale3d.z)
    rotation = rest_local.rotation
    if bone == BODY_BONE:
        shake_x, shake_y = rattle(frame, clock)
        translation.x += shake_x
        translation.y += shake_y
        factor = body_scale(frame, clock)
        scale.x *= factor
        scale.y *= factor
        rotator = rotation.rotator()
        rotator.yaw += body_yaw(frame, clock)
        rotation = rotator.quaternion()
    elif bone in (RING_BONE, WAIST_BONE):
        factor = ring_scale(frame, clock) if bone == RING_BONE else waist_scale(frame, clock)
        scale.x *= factor
        scale.y *= factor
    elif bone in axes:
        x, y, place = axes[bone]
        reach, sideways = needle(frame, place, clock), whip(frame, place, clock)
        translation.x += x * reach - y * sideways
        translation.y += y * reach + x * sideways
    return translation, rotation, scale


def top_spin(clock):
    """The most the star turns between two frames, in degrees."""
    return clock.symmetry * SPIN_STEPS * max(clock.spun[frame] - clock.spun[frame - 1]
                                             for frame in range(1, clock.frames + 1))


def build_sequence(toolkit, axes, clock):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    sequence = toolkit["get_or_create_asset"](ANIM_PACKAGE, SEQ_NAME, unreal.AnimSequence, factory)
    toolkit["write_bone_tracks"](sequence, SKELETON_PATH, FPS, clock.frames,
                                 lambda frame, bone, rest_local: key(frame, bone, rest_local, axes, clock),
                                 "Build star intro")
    return sequence


def build_montage(sequence, toolkit):
    """One section that plays through, blending in over nothing — a blend would grow the seed out of whatever
    pose the star was already in."""
    montage = toolkit["build_montage"](sequence, ANIM_PACKAGE, MONTAGE_NAME, ["Start"], [0.0], ["None"])
    for name, seconds in (("blend_in", 0.0), ("blend_out", 0.15)):
        blend = montage.get_editor_property(name)
        blend.set_editor_property("blend_time", seconds)
        montage.set_editor_property(name, blend)
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(ANIM_PACKAGE, MONTAGE_NAME))
    return montage


def vertex_groups(toolkit):
    """Vertex index -> which of the star's three rings carries it.

    A vertex split across two bones travels a fraction of what either does, so what the player sees is never one
    bone track; the needle group is whatever a tip bone touches at all.
    """
    groups = {}
    for index, _, _, _, weights in toolkit["vertex_weight_report"](MESH_PATH):
        groups[index] = "needle" if any(bone.startswith(SPIKE_PREFIX) for bone in weights) else \
            "spike" if RING_BONE in weights else "waist"
    return groups


def measure(sequence, toolkit, groups, clock):
    """Per frame -> (needle radius, spike radius, waist radius), the silhouette the renderer actually draws."""
    times = [frame / float(FPS) for frame in range(clock.frames + 1)]
    measured = []
    for positions in toolkit["skinned_vertex_positions"](MESH_PATH, sequence, times):
        radii = {"needle": 0.0, "spike": 0.0, "waist": 0.0}
        for index, position in enumerate(positions):
            group = groups[index]
            radii[group] = max(radii[group], math.hypot(position.x, position.y))
        measured.append((radii["needle"], radii["spike"], radii["waist"]))
    return measured


def report(sequence, montage, toolkit, measured, axes, clock):
    beats = [(0, "a seed, no points at all, dead still"), (WAKE, "it starts to shiver and swell")]
    for place, erupt in enumerate(clock.schedule):
        beats.append((erupt, "point %d of %d erupts" % (place + 1, clock.tips)))
    beats += [(clock.alive, "every point out — the star turns as one, then accelerates"),
              (clock.wind_still, "frozen: wound tightest, spin stopped dead"),
              (clock.blast + 1, "NOVA — the curve's overshoot; each part lands as it reads it"),
              (clock.plateau, "held open until here, then the slow settle"),
              (clock.frames, "back on the reference pose, every point flush")]

    LOG.append("{} — {} frames at {} fps = {:.2f}s, {} sampled keys (expect {})".format(
        SEQ_NAME, clock.frames, FPS, clock.frames / float(FPS),
        toolkit["playable_key_count"](sequence), clock.frames + 1))
    LOG.append("{} points, erupting on a stride round the ring: {}".format(
        clock.tips, " -> ".join(name[len(SPIKE_PREFIX):]
                                for name, _ in sorted(axes.items(), key=lambda item: item[1][2]))))
    LOG.append("")
    for frame, what in sorted(beats):
        LOG.append("  f%-4d %5.2fs  %s" % (frame, frame / float(FPS), what))
    LOG.append("")
    LOG.append("net turn %+.0f deg = %d x the star's own %.1f deg symmetry, fastest %.1f deg/frame (reads"
               " backwards past %.1f)" % (clock.symmetry * SPIN_STEPS, SPIN_STEPS, clock.symmetry,
                                          top_spin(clock), clock.symmetry / 2.0))
    LOG.append("seed: body %.2f, spike ring %.2f, valley ring %.2f — a ring's scale only carries the share of"
               " each vertex it is weighted, so the two nearly cancel and the seed reads as the whole star"
               " shrunk" % (SEED_BODY, SEED_RING, SEED_WAIST))
    LOG.append("nova: body %.2f on the frame and %.2f held, spike ring %.2f, valleys %.2f, every tip thrown %.0f"
               " units" % (BODY_PUNCH, BODY_HOLD, RING_BURST, WAIST_BURST, BURST_REACH))
    LOG.append("")
    LOG.append("frame  drive  points   body   ring  waist   yaw     shake    needle   spike   waist   silhouette")

    marks = sorted(set(list(range(0, clock.frames + 1, SAMPLE)) + [frame for frame, _ in beats]))
    for frame in marks:
        needle_radius, spike_radius, waist_radius = measured[frame]
        shake = math.hypot(*rattle(frame, clock))
        LOG.append("%5d %+6.2f %6.2f %6.3f %6.3f %6.3f %7.1f %6.1f  %8.1f %7.1f %7.1f   %s" % (
            frame, drive(frame, clock), points_out(frame, clock), body_scale(frame, clock),
            ring_scale(frame, clock), waist_scale(frame, clock), body_yaw(frame, clock), shake,
            needle_radius, spike_radius, waist_radius,
            "#" * int(round(needle_radius / 40.0))))

    peak, peak_frame = max((entry[0], frame) for frame, entry in enumerate(measured))
    LOG.append("")
    LOG.append("silhouette: %.0f units across at rest, %.0f as the seed, peak %.0f on f%d — %.0f%% of a %.0f-unit"
               " view" % (2.0 * measured[clock.frames][0], 2.0 * measured[0][0], 2.0 * peak, peak_frame,
                          200.0 * peak / VIEW_WIDTH, VIEW_WIDTH))
    LOG.append("held open %.0f across over f%d..%d" % (
        2.0 * max(measured[frame][0] for frame in range(clock.blast + 3, clock.plateau + 1)),
        clock.blast + 3, clock.plateau))

    # A needle inside its own spike means a tip bone was pulled the wrong way, which everts the point.
    inverted = [frame for frame in range(clock.frames + 1) if measured[frame][0] < measured[frame][1] - 0.01]
    LOG.append("points never pulled inside their own spikes: %s" % (
        "clear on all %d frames" % (clock.frames + 1) if not inverted else "INVERTED on %s" % inverted))
    swallowed = [frame for frame in range(clock.frames + 1) if measured[frame][1] < measured[frame][2]]
    LOG.append("valleys standing outside the points, which would evert the star: %s" % (
        "f%d..f%d" % (min(swallowed), max(swallowed)) if swallowed else "never"))

    rest = {name: local for name, (local, _) in toolkit["reference_pose_table"](SKELETON_PATH).items()}
    options = unreal.AnimPoseEvaluationOptions()
    last = toolkit["local_pose_table"](APE.get_anim_pose_at_time(sequence, clock.frames / float(FPS), options))
    gaps = [(math.dist(last[bone][0], (transform.translation.x, transform.translation.y, transform.translation.z)),
             abs((last[bone][1] - transform.rotation.rotator().yaw + clock.symmetry / 2.0) % clock.symmetry
                 - clock.symmetry / 2.0),
             max(abs(a - b) for a, b in zip(last[bone][2], (transform.scale3d.x, transform.scale3d.y,
                                                            transform.scale3d.z))))
            for bone, transform in rest.items()]
    LOG.append("last key against the reference pose over all %d bones: translation %.4f units, yaw %.4f deg"
               " (mod %.1f), scale %.5f" % (len(rest), max(g[0] for g in gaps), max(g[1] for g in gaps),
                                            clock.symmetry, max(g[2] for g in gaps)))
    LOG.append("montage sections read back: {}, blending in over {:.2f}s".format(
        toolkit["montage_sections"](montage),
        montage.get_editor_property("blend_in").get_editor_property("blend_time")))

    if inverted:
        raise RuntimeError("points are turned inside out on {} frames, first on f{}".format(
            len(inverted), inverted[0]))


try:
    # Loaded the same way a script is run, so the rig and silhouette maths lives in one place.
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    component = {name: world for name, (_, world) in toolkit["reference_pose_table"](SKELETON_PATH).items()}
    axes = spike_axes(component, toolkit)
    missing = [bone for bone in (BODY_BONE, RING_BONE, WAIST_BONE) if bone not in component]
    if missing or len(axes) < 3:
        raise RuntimeError("skeleton is missing {} or has fewer than three {}N bones".format(
            missing, SPIKE_PREFIX))

    clock = build_clock(axes, toolkit)
    if top_spin(clock) >= clock.symmetry / 2.0:
        raise RuntimeError("the star turns {:.1f} deg/frame against a {:.1f} deg symmetry, which reads as going"
                           " backwards".format(top_spin(clock), clock.symmetry))

    sequence = build_sequence(toolkit, axes, clock)
    montage = build_montage(sequence, toolkit)
    report(sequence, montage, toolkit, measure(sequence, toolkit, vertex_groups(toolkit), clock), axes, clock)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
