"""Star boss death: the star spins itself apart, swallows its own points and collapses to nothing.

The clip opens exactly where the idle leaves it and the star starts to turn, winding up to a speed it never
reaches in a fight and then holding it there for the rest of the clip. Once it is at that speed the points
start coming out — one at a time, each stabbing past where it will sit and whipping sideways as it lands, and
then staying out. They come in no order at all: not round the ring and not on a stride, but scattered, so the
star reads as firing them off wrongly rather than as doing anything on purpose. They arrive faster and faster
until every one of them is out and the thing is a bristling, howling mess turning at the fastest a shape with
eight points can be shown to turn. It holds that for half a second. All of it happens at the size the star has
always been: nothing changes size while it is only turning and spiking.

Then it goes off. The whole star swells in three frames — the rattle dying into it, nothing else moving, just
the shape getting bigger — and on the far side of that swell every point is yanked in at once, three frames
from fully out to flush with the valleys between them, far faster than any of them came out. Four frames later
there is nothing there at all.

The swell has to come first and the retraction after it. Taken the other way round the points read as being
politely put away and the star as deflating; landed on the back of a swell they read as pulled in by whatever
the swell was, and the whole ending reads as one blast rather than as a slow collapse.

Constraints:
- The clip starts on the reference pose, which is where the idle hands over, so the montage blends in.
- It ends on nothing, at zero scale, which is what the actor being removed reads as.
- The star never turns more than half its own symmetry between two frames, past which it reads as turning back.
- A tip bone only ever pushes its needle out: pulled the other way it turns the point inside out, so what draws
  the points in is the scale of the ring they hang off, and that ring is what the implosion is written on.
- The points may come flush with the valleys but never behind them, which would evert the star; the margin is
  measured on the skinned silhouette, since a tip vertex is split between its own bone and the ring.
- Nothing changes size before the implosion, so the one size change in the clip is the one that means death.
  The points come out on their bones' reach, never on a scale, which is what keeps that true while they do.
- Every point has to be out and holding for BRISTLE frames before the implosion, or the star is never once
  seen with all of them out and the eruptions read as a wave the collapse interrupted.
- Nothing moves or scales on Z, which an orthographic camera down that axis would spend for nothing.

Run via mcp-unreal execute_script. Re-runnable: rewrites the sequence and the montage in place.
Report written to REPORT.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/Star/SK_Star"
MESH_PATH = "/Game/Characters/Meshes/Star/SKM_Star"
ANIM_PACKAGE = "/Game/Characters/Anim/Star"
SEQ_NAME = "SK_Star_Sequence_Death"
MONTAGE_NAME = "SK_Star_Montage_Death"
REPORT = ("C:/Users/Felou/AppData/Local/Temp/claude/C--GeoTrinity/"
          "d68d07b7-9cd8-4924-91b7-154cbe48353c/scratchpad/star_death.txt")

SPIKE_PREFIX = "apexe_outside_"
RING_BONE = "apexes_outside"
WAIST_BONE = "apexes_inside"
BODY_BONE = "Root"
VIEW_WIDTH = 3000.0  # the camera volumes' default OrthoWidth, to read the peak silhouette against

FPS = 30
SAMPLE = 4           # frames between report rows

# --- Winding up, then the points coming out: everything before anything changes size -----------------
FALTER = 5           # frames holding the idle's pose before the star starts to turn
CRAZY_FROM = 40      # frame the points start coming out; up to here the star only winds its turn up
SUCK = 146           # first frame of the implosion; the turn runs into it with nothing holding still
SPIN_EASE = 1.0      # 1 is a flat angular acceleration up to the top speed, which is then simply held
SHAKE_RAMP = 2.2     # above 1 the rattle keeps out of the way until the points are mostly out
SHAKE = 30.0         # units the star rattles at the worst of it
BRISTLE = 20         # frames every point must be out and holding before the implosion takes them

ERUPT_GAP, ERUPT_RATIO, ERUPT_MIN = 18.0, 0.82, 5  # frames between eruptions, shrinking, floored
STAB = [0.45, 1.0, 0.62, 0.28]  # of the throw past its held reach, from the frame a point fires
# A tip bone carries half of its own vertex, so a point only shows half of what the bone is given and a
# legible one is stretched well past the blade the mesh has. These put the star's silhouette at three times
# its resting one, which is what makes the points read at all against a body that is not changing size.
ERUPT_REACH = 260.0  # units a tip bone throws on its own eruption; its vertex travels half of that
HELD_REACH = 170.0   # units it keeps out from then on, since a point that came out has to stay out
WHIP_REACH = 55.0    # units it lashes sideways as it lands, the other way from the point before it

# --- Going off: the swell, the points yanked in behind it, and nothing left --------------------------
BOOM = 3             # frames the whole star swells over; the retraction lands on the back of this
SWALLOW = 3          # frames the points take to snap flush, against the 80 they took to come out
VANISH = 4           # frames what is left takes to go from that swell to nothing
TAIL = 6             # frames of nothing, so the montage covers the actor being taken away

BOOM_SWELL = 1.75    # what the star swells to on the beat the points are taken on
RING_CRUSH = 0.70    # the spike ring's scale once the points are in; below this the star turns inside out
WAIST_BLOAT = 1.06   # the valley ring swelling on what the points fed it
VISIBLE = 0.25       # of itself, the size below which the silhouette is too small to read anything off

SPIN_PEAK = 12.0     # turning rate the wind-up reaches and then holds, in units the table normalises away
COLLAPSE_SPIN = 13.0 # and through the implosion, where everything rushing inward speeds the turn up; the
                     # peak is what the whole turn is scaled against, so a tall one spends the rest of it
TOP_SPEED = 21.0     # degrees the star crosses in a frame at its fastest, under the half-symmetry it reads
                     # backwards past; the total turn is scaled to this rather than written as an angle

LOG = []


def smoothstep(alpha):
    alpha = min(1.0, max(0.0, alpha))
    return alpha * alpha * (3.0 - 2.0 * alpha)


def spike_axes(component, toolkit):
    """Tip bone -> (outward unit x, outward unit y, its place in the firing order).

    Fired in no order at all rather than round the ring or on a stride: even gaps read as a shape working,
    and only uneven ones read as one coming apart.
    """
    names = sorted((name for name in component if name.startswith(SPIKE_PREFIX)),
                   key=lambda name: int(name[len(SPIKE_PREFIX):]))
    axes = {}
    for place, index in enumerate(toolkit["scrambled_order"](len(names))):
        position = component[names[index]].translation
        length = math.hypot(position.x, position.y)
        axes[names[index]] = (position.x / length, position.y / length, place)
    return axes


def wound(frame):
    """0 to 1 across the wind-up, then held — the turn reaches its top speed and simply stays there."""
    return min(1.0, max(0.0, frame - FALTER) / float(CRAZY_FROM - FALTER)) ** SPIN_EASE


def crazed(frame):
    """0 to 1 across the stretch the points are coming out over, which the rattle grows with."""
    return min(1.0, max(0.0, frame - CRAZY_FROM) / float(SUCK - 1 - CRAZY_FROM))


def boomed(frame):
    """0 to 1 as the whole star swells, then held — the beat the points are taken on the back of."""
    return smoothstep((frame - SUCK) / float(BOOM))


def swallowed(frame):
    """0 to 1 as the points snap flush with the valleys, which starts only once the swell is done."""
    return smoothstep((frame - SUCK - BOOM) / float(SWALLOW))


def vanished(frame):
    """0 to 1 as what is left of the star is blown out of existence."""
    return smoothstep((frame - SUCK - BOOM - SWALLOW) / float(VANISH))


def gone(frame):
    """True once there is nothing left to draw."""
    return frame >= SUCK + BOOM + SWALLOW + VANISH


def rattle(frame):
    """The star's shudder this frame -> (x, y), alternating every frame since slower reads as a wobble."""
    reach = SHAKE * crazed(frame) ** SHAKE_RAMP * (1.0 - boomed(frame))
    return (reach if frame % 2 else -reach), (reach * 0.6 if frame % 4 < 2 else -reach * 0.6)


def spin_rate(frame):
    """How fast the star is turning at `frame`, in units the spin table normalises away.

    It reaches its top speed before the points start coming out and holds it from there, so the whole of the
    worst of the clip runs at that speed rather than only touching it at the end. The swell speeds it up again
    — a shape pulling its own mass in turns harder for it — and it stops only where nothing is left.
    """
    if frame <= FALTER or gone(frame):
        return 0.0
    if frame < SUCK:
        return 1.0 + (SPIN_PEAK - 1.0) * wound(frame)
    return SPIN_PEAK + (COLLAPSE_SPIN - SPIN_PEAK) * boomed(frame)


def stab(frame, place, schedule):
    """Units this point is thrown by, which only ever pushes it out.

    A point overshoots where it will sit and then keeps that reach: one that came out has to stay out, or the
    ring reads as a wave travelling round rather than as points that have gone. The implosion is what takes
    them back, through the same curve that draws the ring in.
    """
    step = frame - schedule[place]
    if step < 0:
        return 0.0
    throw = STAB[step] if step < len(STAB) else 0.0
    return (HELD_REACH + (ERUPT_REACH - HELD_REACH) * throw) * (1.0 - swallowed(frame))


def whip(frame, place, schedule):
    """Units this point lashes sideways as it lands, the other way from the point before it."""
    step = frame - schedule[place]
    if not 0 <= step < len(STAB):
        return 0.0
    return WHIP_REACH * STAB[step] * (1.0 if place % 2 else -1.0)


def ring_scale(frame):
    """The spike ring's XY scale: its own size until the implosion, then crashed in until the points are flush."""
    return 1.0 + (RING_CRUSH - 1.0) * swallowed(frame)


def waist_scale(frame):
    """The valley ring's XY scale: its own size, then swelling on the points it has just taken in."""
    return 1.0 + (WAIST_BLOAT - 1.0) * swallowed(frame)


def body_scale(frame):
    """The whole star's XY scale: its own size, the swell, then blown out of existence from that size.

    The swell is held through the retraction rather than eased off it, so the points are taken while the star
    is at its biggest and nothing between the two beats reads as the shape settling.
    """
    if gone(frame):
        return 0.0
    return (1.0 + (BOOM_SWELL - 1.0) * boomed(frame)) * (1.0 - vanished(frame))


def key(frame, bone, rest_local, axes, spun, turn, schedule):
    """The bone's local transform at `frame`, as the track writer wants it."""
    translation = unreal.Vector(rest_local.translation.x, rest_local.translation.y, rest_local.translation.z)
    scale = unreal.Vector(rest_local.scale3d.x, rest_local.scale3d.y, rest_local.scale3d.z)
    rotation = rest_local.rotation
    if bone == BODY_BONE:
        shake_x, shake_y = rattle(frame)
        translation.x += shake_x
        translation.y += shake_y
        factor = body_scale(frame)
        scale.x *= factor
        scale.y *= factor
        rotator = rotation.rotator()
        rotator.yaw += turn * spun[frame]
        rotation = rotator.quaternion()
    elif bone in (RING_BONE, WAIST_BONE):
        factor = ring_scale(frame) if bone == RING_BONE else waist_scale(frame)
        scale.x *= factor
        scale.y *= factor
    elif bone in axes:
        x, y, place = axes[bone]
        reach, sideways = stab(frame, place, schedule), whip(frame, place, schedule)
        translation.x += x * reach - y * sideways
        translation.y += y * reach + x * sideways
    return translation, rotation, scale


def top_step(spun):
    """The largest fraction of its whole turn the star crosses between two frames."""
    return max(spun[frame] - spun[frame - 1] for frame in range(1, len(spun)))


def build_sequence(toolkit, axes, spun, turn, schedule, frames):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    sequence = toolkit["get_or_create_asset"](ANIM_PACKAGE, SEQ_NAME, unreal.AnimSequence, factory)
    toolkit["write_bone_tracks"](
        sequence, SKELETON_PATH, FPS, frames,
        lambda frame, bone, rest_local: key(frame, bone, rest_local, axes, spun, turn, schedule),
        "Build star death")
    return sequence


def build_montage(sequence, toolkit):
    """One section that plays through and stops there, blending in off the idle and out of nothing."""
    montage = toolkit["build_montage"](sequence, ANIM_PACKAGE, MONTAGE_NAME, ["Death"], [0.0], ["None"])
    for name, seconds in (("blend_in", 0.08), ("blend_out", 0.0)):
        blend = montage.get_editor_property(name)
        blend.set_editor_property("blend_time", seconds)
        montage.set_editor_property(name, blend)
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(ANIM_PACKAGE, MONTAGE_NAME))
    return montage


def vertex_groups(toolkit):
    """Vertex index -> which of the star's rings carries it.

    A tip vertex is split between its own bone and the spike ring, so it travels a fraction of what either
    does and no bone track reads as the silhouette on its own.
    """
    groups = {}
    for index, _, _, _, weights in toolkit["vertex_weight_report"](MESH_PATH):
        groups[index] = "needle" if any(bone.startswith(SPIKE_PREFIX) for bone in weights) else \
            "spike" if RING_BONE in weights else "waist"
    return groups


def measure(sequence, toolkit, groups, frames):
    """Per frame -> (needle radius, spike radius, waist radius), the silhouette the renderer actually draws."""
    times = [frame / float(FPS) for frame in range(frames + 1)]
    measured = []
    for positions in toolkit["skinned_vertex_positions"](MESH_PATH, sequence, times):
        radii = {"needle": 0.0, "spike": 0.0, "waist": 0.0}
        for index, position in enumerate(positions):
            group = groups[index]
            radii[group] = max(radii[group], math.hypot(position.x, position.y))
        measured.append((radii["needle"], radii["spike"], radii["waist"]))
    return measured


def report(sequence, montage, toolkit, measured, axes, spun, turn, schedule, tips, frames):
    beats = [(0, "the idle's pose, dead still"), (FALTER, "the star starts winding its turn up"),
             (CRAZY_FROM, "top speed reached and held — the points start coming out")]
    for place, erupt in enumerate(schedule):
        beats.append((erupt, "point %d of %d fires out and stays out" % (place + 1, tips)))
    beats += [(schedule[-1] + len(STAB), "every point out, bristling and holding"),
              (SUCK, "BOOM — the whole star swells, points still fully out"),
              (SUCK + BOOM, "and the points are yanked in behind it"),
              (SUCK + BOOM + SWALLOW, "flush: no star left, only a disc at full swell"),
              (SUCK + BOOM + SWALLOW + VANISH, "gone"), (frames, "nothing")]

    LOG.append("{} — {} frames at {} fps = {:.2f}s, {} sampled keys (expect {})".format(
        SEQ_NAME, frames, FPS, frames / float(FPS), toolkit["playable_key_count"](sequence), frames + 1))
    order = [int(name[len(SPIKE_PREFIX):])
             for name, _ in sorted(axes.items(), key=lambda item: item[1][2])]
    LOG.append("{} points, firing in no order: {} — gaps round the ring of {}, which is what says wrong"
               " rather than deliberate".format(
                   tips, " -> ".join(str(index) for index in order),
                   [(order[place + 1] - order[place]) % tips for place in range(tips - 1)]))
    LOG.append("they land on f{}, the last {} frames before the implosion holding every point out".format(
        schedule, SUCK - (schedule[-1] + len(STAB))))
    LOG.append("")
    for frame, what in sorted(beats):
        LOG.append("  f%-4d %5.2fs  %s" % (frame, frame / float(FPS), what))
    LOG.append("")
    LOG.append("net turn %+.0f deg = %.1f turns, fastest %.1f deg/frame (reads backwards past %.1f)" % (
        turn, turn / 360.0, turn * top_step(spun), (360.0 / tips) / 2.0))
    LOG.append("the implosion takes the spike ring to %.2f and the valleys to %.2f, which is what swallows the"
               " points; the star swells to %.2f first and is gone %d frames after they are in"
               % (RING_CRUSH, WAIST_BLOAT, BOOM_SWELL, VANISH))
    LOG.append("")
    LOG.append("frame  wound  crazy   boom  swall   gone   body   ring  waist    yaw    shake   needle  waist"
               "  spiky")

    marks = sorted(set(list(range(0, frames + 1, SAMPLE)) + [frame for frame, _ in beats]))
    for frame in marks:
        needle_radius, spike_radius, waist_radius = measured[frame]
        LOG.append("%5d %6.2f %6.2f %6.2f %6.2f %6.2f %6.3f %6.3f %6.3f %7.1f %6.1f %8.1f %6.1f %6.1f  %s" % (
            frame, wound(frame), crazed(frame), boomed(frame), swallowed(frame), vanished(frame),
            body_scale(frame),
            ring_scale(frame), waist_scale(frame), turn * spun[frame], math.hypot(*rattle(frame)),
            needle_radius, waist_radius, needle_radius - waist_radius,
            "#" * int(round(needle_radius / 6.0))))

    peak, peak_frame = max((entry[0], frame) for frame, entry in enumerate(measured))
    LOG.append("")
    LOG.append("silhouette: %.0f units across at f0, peak %.0f on f%d — %.0f%% of a %.0f-unit view; the last"
               " frame leaves %.3f" % (2.0 * measured[0][0], 2.0 * peak, peak_frame,
                                       200.0 * peak / VIEW_WIDTH, VIEW_WIDTH, 2.0 * measured[frames][0]))

    # Nothing may change size before the implosion, or the spin reads as the star inflating rather than turning.
    early = [frame for frame in range(SUCK + 1)
             if abs(body_scale(frame) - 1.0) > 0.001 or abs(ring_scale(frame) - 1.0) > 0.001
             or abs(waist_scale(frame) - 1.0) > 0.001]
    LOG.append("size held flat until the implosion on f%d: %s" % (
        SUCK, "yes" if not early else "CHANGED on f%d" % min(early)))
    if early:
        raise RuntimeError("something scales on f{}, before the implosion that is supposed to own it".format(
            min(early)))

    # The points may come flush with the valleys but never behind them, which turns the star inside out. Read
    # only where the star is still there to look at: once the body is crushed both radii run to zero together
    # and the difference stops meaning anything.
    spikiness = [(measured[frame][0] - measured[frame][2], frame) for frame in range(frames + 1)
                 if body_scale(frame) > VISIBLE]
    tightest, tightest_frame = min(spikiness)
    LOG.append("points against the valleys: %.1f units proud at f0, %.1f once they are swallowed on f%d,"
               " tightest %.1f on f%d while the star is still bigger than %.2f of itself" % (
                   measured[0][0] - measured[0][2],
                   measured[SUCK + BOOM + SWALLOW][0] - measured[SUCK + BOOM + SWALLOW][2],
                   SUCK + BOOM + SWALLOW,
                   tightest, tightest_frame, VISIBLE))
    if tightest < 0.0:
        raise RuntimeError("the points sit {:.1f} units behind the valleys on f{}, which everts the star".format(
            -tightest, tightest_frame))

    # A tip bone pulled inward everts its own needle, so the reach written on one is only ever outward. The
    # silhouette cannot answer this once the tips are back at rest, where a needle and its ring read alike.
    pulled = [(frame, place) for frame in range(frames + 1) for place in range(tips)
              if stab(frame, place, schedule) < 0.0]
    LOG.append("tip bones only ever pushed out: %s" % (
        "clear on all %d frames" % (frames + 1) if not pulled else "PULLED IN on %s" % pulled[:6]))
    if pulled:
        raise RuntimeError("a tip bone is pulled inward on f{}, which everts its point".format(pulled[0][0]))

    rest = {name: local for name, (local, _) in toolkit["reference_pose_table"](SKELETON_PATH).items()}
    options = unreal.AnimPoseEvaluationOptions()
    first = toolkit["local_pose_table"](APE.get_anim_pose_at_time(sequence, 0.0, options))
    gaps = [(math.dist(first[bone][0], (transform.translation.x, transform.translation.y,
                                        transform.translation.z)),
             abs(first[bone][1] - transform.rotation.rotator().yaw),
             max(abs(a - b) for a, b in zip(first[bone][2], (transform.scale3d.x, transform.scale3d.y,
                                                             transform.scale3d.z))))
            for bone, transform in rest.items()]
    LOG.append("first key against the reference pose over all %d bones: translation %.4f units, yaw %.4f deg,"
               " scale %.5f" % (len(rest), max(g[0] for g in gaps), max(g[1] for g in gaps),
                                max(g[2] for g in gaps)))
    LOG.append("montage sections read back: {}, blending in over {:.2f}s".format(
        toolkit["montage_sections"](montage),
        montage.get_editor_property("blend_in").get_editor_property("blend_time")))


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

    tips = len(axes)
    frames = SUCK + BOOM + SWALLOW + VANISH + TAIL
    # Gaps that shrink by a fixed ratio escalate on their own, so the rhythm follows however many points the
    # rig turns out to carry rather than being keyed beat by beat.
    schedule = toolkit["accelerating_schedule"](CRAZY_FROM + 2, tips, ERUPT_GAP, ERUPT_RATIO, ERUPT_MIN)
    if SUCK - (schedule[-1] + len(STAB)) < BRISTLE:
        raise RuntimeError("the last point lands on f{} and the implosion starts on f{}, so the star is never"
                           " seen with all of them out".format(schedule[-1] + len(STAB), SUCK))
    # Integrated from the rate rather than written as an angle, so the star is free to accelerate through its
    # own collapse and the total is still exactly the turn it was given.
    spun = toolkit["normalised_spin"](spin_rate, frames)
    # Scaled to the bound the star's own symmetry puts on how fast it can be shown turning, rather than to an
    # angle picked by hand that a change of timings would quietly push past it.
    turn = TOP_SPEED / top_step(spun)
    if TOP_SPEED >= (360.0 / tips) / 2.0:
        raise RuntimeError("the star is driven at {:.1f} deg/frame against a {:.1f} deg symmetry, which reads"
                           " as going backwards".format(TOP_SPEED, 360.0 / tips))

    sequence = build_sequence(toolkit, axes, spun, turn, schedule, frames)
    montage = build_montage(sequence, toolkit)
    report(sequence, montage, toolkit, measure(sequence, toolkit, vertex_groups(toolkit), frames), axes, spun,
           turn, schedule, tips, frames)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
