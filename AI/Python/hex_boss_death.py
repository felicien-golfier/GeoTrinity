"""Hex boss death: the three hexagons each wander off the axis, then blow apart and sit there.

The clip opens exactly where the idle leaves it and the boss starts to spin up — square on its axis, still one
machine, just turning, and turning faster every frame until it is going as fast as a hexagon can be shown to
turn. Only then does it lose its alignment: each of the three hexagons walks off the shared axis on an orbit of
its own, at its own speed and its own direction, so the boss stops being one turning thing and becomes three
that no longer agree, blades ratcheting out of every face a notch at a time. All of that happens at the size
the boss has always been. Only in the last half-second does anything change size: the three are dragged back
toward the axis, the blades suck in and the whole shape gathers down — straight into the detonation, with
nothing holding still on the way. Then it goes: the shell thrown first, the middle ring two frames later, the
core last, the three flying out a third of a turn apart with every blade full out.

They slow as they fly and the tumble bleeds off with them, so the clip ends on three separate pieces sitting
still, apart, at rest — the wreckage left where it landed rather than anything cleared away.

This is the intro's shape run the other way. The intro's three pieces arrive one at a time and lock into a
whole; here a whole comes apart into three. Where the intro freezes before its blast, this one never stops:
nothing here is winding up to strike, it is only breaking, and a break has no beat of stillness in it.

Constraints:
- The clip starts on the pose the idle hands over: the reference pose with every blade folded into its face.
- It ends on three pieces held still and whole, so nothing here returns to a pose and nothing is scaled away.
- Nothing changes size while the boss is only turning or coming off its axis. Every scale in the clip belongs
  to the gather and the blast, so the one size change the eye gets is the one that means the explosion.
- No ring crosses more than STROBE degrees between two frames, past which it reads as turning the other way.
  The turns are scaled to that bound rather than written as angles, so the spin runs as fast as the shape can
  be shown turning and no faster, whatever the clip's timings are changed to.
- Up to the detonation the boss is still one object: however far the rings stray, every pair of them has to
  keep touching, or the shape comes apart before the blast that is supposed to do it.
- After it they are three, and each pair has to be clear of the other CLEAR_BEFORE frames before the flight
  ends, since a pair still merged when the motion stops never reads as two pieces at all.
- The tableau they come to rest in has to fit the view, piece centres and stretched blades together.
- Nothing moves or scales on Z, which an orthographic camera down that axis would spend for nothing.

Run via mcp-unreal execute_script. Re-runnable: rewrites the sequence and the montage in place.
Report written to REPORT.
"""
import collections
import math

import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/HexBoss/SK_HexBoss"
MESH_PATH = "/Game/Characters/Meshes/HexBoss/SKM_HexBoss"
ANIM_PACKAGE = "/Game/Characters/Anim/HexBoss"
SEQ_NAME = "SK_HexBoss_Sequence_Death"
MONTAGE_NAME = "SK_HexBoss_Montage_Death"
REPORT = ("C:/Users/Felou/AppData/Local/Temp/claude/C--GeoTrinity/"
          "d68d07b7-9cd8-4924-91b7-154cbe48353c/scratchpad/hex_boss_death.txt")

ROOT = "Root"
OUTER, MID, CORE = "HexOuter", "HexMid", "HexCore"
SPIKE_MARK = "_Spike_"
STROBE = 30.0        # half the 60 degrees that map a hexagon onto itself
VIEW_WIDTH = 3000.0  # the camera volumes' default OrthoWidth, which the resting tableau has to fit

FPS = 30
SAMPLE = 4           # frames between report rows
CLEAR_BEFORE = 12    # frames a pair must read as two pieces before the flight stops and nothing moves again

# --- Spinning up, coming apart, and the gather that runs straight into the blast --------------------
FALTER = 5           # frames holding the idle's pose before the boss starts to turn
STRAY_FROM = 42      # frame the rings start leaving the axis; up to here the boss only spins up, square
BLAST = 104          # the detonation; the run-up goes into it with no frame of stillness anywhere
GATHER = 14          # frames before it over which the one size change in the clip happens
BURST = [0.50, 1.35, 1.0]  # the mid-flight frame, the overshoot that lands the detonation, then full extension

SPIN_EASE = 1.0      # 1 is a flat angular acceleration: the boss is already turning hard before it strays
STRAY_EASE = 1.6     # above 1 the rings leave the axis gently and then let go
CHARGE_EASE = 2.0    # above 1 the gather bites late, so it reads as one movement rather than a slow squeeze
GATHER_PULL = 0.45   # of its stray a ring is dragged back toward the axis by the gather
SHAKE_RAMP = 2.0     # above 1 the rattle keeps out of the way until the run-up is nearly wound
WIND_SHAKE = 24.0    # units the whole boss rattles at the worst of it
SHOCK_DECAY = 0.5    # per frame, the detonation's own shock dying off

RATCHET = [1.0, 0.30, 0.72, 0.12]  # of a ring's flare, one step every RATCHET_HOLD frames
RATCHET_HOLD = 2

BODY_CHARGE, BODY_PUNCH = 0.72, 1.62  # the whole boss gathered, and on the detonation frame alone

# --- The flight: how a thrown piece travels and where it stops --------------------------------------
FLIGHT = 42          # frames a piece takes to cover its reach and come to a halt
HOLD = 18            # frames the three sit there, still, once everything has stopped

ALIVE_SPIN = 1.0     # turning rate as the spin-up starts; the spin table normalises the units away
SPIN_PEAK = 12.0     # rate at the top of it, which the thrown pieces carry off and then bleed away
TOP_SPEED = 27.0     # degrees the fastest ring crosses in a frame, kept under the STROBE it would read past

# (bone, degrees it is thrown along, its share of TOP_SPEED and the direction it turns, frames it reads the
#  drive late, units it strays off the axis at the worst of the wander, degrees per frame it walks round that
#  stray, frames its ratchet is offset, its blade reach there, the frame it is thrown, units it flies, its
#  scale gathered, its scale on the blast, its blade reach there)
Piece = collections.namedtuple(
    "Piece", "bone angle share lag stray orbit beat flare launch reach charge_scale burst_scale burst_spike")

# The three are thrown a third of a turn apart, which is the widest three directions can be and so the soonest
# they read as separate. The shell goes first and the core last, and the lighter a piece is the further it is
# thrown. Each ring's stray walks round at its own rate and lands pointing where it will be thrown, so the
# wander runs straight into the throw instead of being reset by it. The shares are not whole sixths of a turn:
# the pieces are meant to come to rest askew, like wreckage, rather than square with where they started.
PIECES = [
    Piece(OUTER, 210.0, 0.84, 0, 140.0, 7.0, 0, 0.60, BLAST, 660.0, 0.80, 1.25, 1.75),
    Piece(MID, 90.0, -1.00, 1, 165.0, -13.0, 1, 0.45, BLAST + 2, 780.0, 0.86, 1.12, 1.70),
    Piece(CORE, 330.0, 0.93, 2, 185.0, 19.0, 3, 0.70, BLAST + 4, 880.0, 1.10, 1.06, 1.60),
]

RINGS = {piece.bone: piece for piece in PIECES}
FRAMES = BLAST + FLIGHT + HOLD

LOG = []


def smoothstep(alpha):
    alpha = min(1.0, max(0.0, alpha))
    return alpha * alpha * (3.0 - 2.0 * alpha)


def wound(frame):
    """0 to 1 across the whole run-up — how far the boss has spun up, which the turn and the rattle read."""
    return min(1.0, max(0.0, frame - FALTER) / float(BLAST - 1 - FALTER)) ** SPIN_EASE


def strayed(frame):
    """0 to 1 from the frame the rings start leaving the axis; 0 while the boss is still only turning."""
    return min(1.0, max(0.0, frame - STRAY_FROM) / float(BLAST - 1 - STRAY_FROM)) ** STRAY_EASE


def charged(frame):
    """0 to 1 across the gather alone, which is the only stretch of the clip anything changes size in."""
    return min(1.0, max(0.0, frame - (BLAST - GATHER)) / float(GATHER - 1)) ** CHARGE_EASE


def drive(frame):
    """-1 gathered tightest, +1 blown apart — the one curve every size in the clip reads.

    It never comes back to rest: past the detonation there is no shape left to return to one.
    """
    if frame < BLAST:
        return -charged(frame)
    step = frame - BLAST
    return BURST[step] if step < len(BURST) else BURST[-1]


def stray(frame, piece):
    """Where this ring has wandered to -> (x, y), on an orbit of its own that opens as the boss loses it.

    Wound back from the direction the piece is thrown in, so each ring arrives at the blast already pointing
    the way it is about to go, and dragged back toward the axis by the gather that ends the run-up.
    """
    held = min(frame, BLAST)
    radius = piece.stray * strayed(held) * (1.0 - GATHER_PULL * charged(held))
    angle = math.radians(piece.angle - piece.orbit * (BLAST - held))
    return radius * math.cos(angle), radius * math.sin(angle)


def rattle(frame):
    """The whole boss's shudder this frame -> (x, y): the run-up's load, then the detonation's shock.

    Alternates every frame, since slower reads as a wobble.
    """
    reach = WIND_SHAKE * (wound(frame) ** SHAKE_RAMP if frame < BLAST
                          else math.exp(-SHOCK_DECAY * (frame - BLAST)))
    return (reach if frame % 2 else -reach), (reach * 0.6 if frame % 4 < 2 else -reach * 0.6)


def spin_rate(frame):
    """How fast the rings are turning at `frame`, in units the spin table normalises away.

    It runs into the blast at its fastest and carries that off, then bleeds away over exactly the frames the
    flight takes, so a piece stops turning where it stops moving.
    """
    if frame <= FALTER:
        return 0.0
    if frame < BLAST:
        return ALIVE_SPIN + (SPIN_PEAK - ALIVE_SPIN) * wound(frame)
    return SPIN_PEAK * (1.0 - smoothstep((frame - BLAST) / float(FLIGHT)))


def spin_turns(spun):
    """Degrees each ring turns over the whole clip -> {bone: degrees}.

    Scaled so the fastest of them crosses exactly TOP_SPEED in a frame: the bound the shape's own symmetry
    puts on how fast it can be shown turning is what sets the turn, rather than an angle picked by hand that
    a change of timings would quietly push past it.
    """
    step = max(spun[frame] - spun[frame - 1] for frame in range(1, FRAMES + 1))
    return {piece.bone: piece.share * TOP_SPEED / step for piece in PIECES}


def offset(frame, piece):
    """Units the piece has flown from the boss's centre along its own direction.

    Hard off the mark and easing to a dead stop at FLIGHT, which both separates the three soonest and leaves
    them sitting where they land.
    """
    step = frame - piece.launch
    if step <= 0:
        return 0.0
    alpha = min(1.0, step / float(FLIGHT))
    return piece.reach * (1.0 - (1.0 - alpha) ** 3)


def teeth(frame, piece):
    """The blade this ring shows: a ratchet that grows as it leaves the axis and is sucked in by the gather.

    Stepped rather than eased, so the ring reads as working itself apart a notch at a time, and tied to the
    stray rather than the turn so that the boss spinning up square shows no teeth at all.
    """
    if frame >= BLAST:
        return 0.0
    return piece.flare * strayed(frame) * (1.0 - charged(frame)) \
        * RATCHET[((frame + piece.beat) // RATCHET_HOLD) % len(RATCHET)]


def spike_reach(frame, piece):
    """A spike's X scale: 0 folded into its face, 1 the blade the mesh gives it, past that a stretched one."""
    value = drive(frame - piece.lag)
    return max(teeth(frame, piece), min(1.0, max(0.0, value)) * piece.burst_spike)


def ring_scale(frame, piece):
    """The ring's XY scale: its own size until the gather, then down, then blown out and held there."""
    value = drive(frame - piece.lag)
    return 1.0 + (min(1.0, value) * (piece.burst_scale - 1.0) if value > 0.0
                  else value * (1.0 - piece.charge_scale))


def body_scale(frame):
    """The whole boss's XY scale: gathered, punched on the detonation, then back to its own size.

    The overshoot alone drives the punch, so the frame nothing could be hit at is the detonation's and no
    other, and the pieces fly at the size they will be found at.
    """
    value = drive(frame)
    if value < 0.0:
        return 1.0 + value * (1.0 - BODY_CHARGE)
    return 1.0 + max(0.0, value - 1.0) / (max(BURST) - 1.0) * (BODY_PUNCH - 1.0)


def key(frame, bone, rest_local, layout, spun, turns):
    """The bone's local transform at `frame`, as the track writer wants it.

    Yaw alone stands for the rotation because every bone of this rig is placed with one.
    """
    x, y, z = rest_local.translation.x, rest_local.translation.y, rest_local.translation.z
    yaw = rest_local.rotation.rotator().yaw
    sx, sy, sz = rest_local.scale3d.x, rest_local.scale3d.y, rest_local.scale3d.z
    if bone == ROOT:
        shake_x, shake_y = rattle(frame)
        x, y = x + shake_x, y + shake_y
        factor = body_scale(frame)
        sx, sy = sx * factor, sy * factor
    elif bone in RINGS:
        piece = RINGS[bone]
        stray_x, stray_y = stray(frame, piece)
        distance = offset(frame, piece)
        x += stray_x + distance * math.cos(math.radians(piece.angle))
        y += stray_y + distance * math.sin(math.radians(piece.angle))
        yaw += turns[bone] * spun[frame]
        factor = ring_scale(frame, piece)
        sx, sy = sx * factor, sy * factor
    elif bone in layout:
        sx = spike_reach(frame, RINGS[layout[bone][0]])
    return unreal.Vector(x, y, z), unreal.Rotator(yaw=yaw).quaternion(), unreal.Vector(sx, sy, sz)


def top_spin(piece, spun, turns):
    """The most the ring turns between two frames, in degrees."""
    return abs(turns[piece.bone]) * max(spun[frame] - spun[frame - 1] for frame in range(1, FRAMES + 1))


def build_sequence(toolkit, layout, spun, turns):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    sequence = toolkit["get_or_create_asset"](ANIM_PACKAGE, SEQ_NAME, unreal.AnimSequence, factory)
    toolkit["write_bone_tracks"](
        sequence, SKELETON_PATH, FPS, FRAMES,
        lambda frame, bone, rest_local: key(frame, bone, rest_local, layout, spun, turns),
        "Build hex boss death")
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


def measure(sequence, toolkit, groups, layout):
    """Per frame -> (local pose, {spike: units past its face}, radius, {pair: gap between whole pieces}).

    Every frame rather than the printed rows: a pair crosses over two or three frames that a coarse sample
    steps straight past.
    """
    options = unreal.AnimPoseEvaluationOptions()
    members = {piece.bone: [bone for bone in groups if bone.startswith(piece.bone)] for piece in PIECES}
    measured = {}
    for frame in range(FRAMES + 1):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        posed = toolkit["_component_transforms"](pose)
        placed = toolkit["place_groups"](groups, posed)
        measured[frame] = (toolkit["local_pose_table"](pose),
                           {spike: toolkit["feature_protrusion"](placed, posed, spike, parent)
                            for spike, (parent, _) in layout.items()},
                           max(math.hypot(v.x, v.y) for group in placed.values() for v in group),
                           toolkit["part_separations"](placed, posed, members))
    return measured


def separated_from(measured, piece, other):
    """The frame past which these two never touch again."""
    touching = [frame for frame in range(BLAST, FRAMES + 1)
                if measured[frame][3][(piece.bone, other.bone)] < 0.0]
    return (max(touching) + 1) if touching else BLAST


def report(sequence, montage, toolkit, measured, spun, turns):
    beats = [(0, "the idle's pose, blades folded"), (FALTER, "the boss starts to spin up, square on its axis"),
             (STRAY_FROM, "it loses its alignment — the three start to stray and to show teeth"),
             (BLAST - GATHER, "dragged back in and gathering down — the first size change in the clip")]
    for piece in PIECES:
        beats.append((piece.launch, "{} is thrown".format(piece.bone)))
    beats += [(BLAST + 1, "DETONATION — the overshoot frame"),
              (BLAST + FLIGHT, "everything has stopped moving and turning"),
              (FRAMES, "three pieces sitting apart, still")]

    LOG.append("{} — {} frames at {} fps = {:.2f}s, {} sampled keys (expect {})".format(
        SEQ_NAME, FRAMES, FPS, FRAMES / float(FPS), toolkit["playable_key_count"](sequence), FRAMES + 1))
    LOG.append("")
    for frame, what in sorted(beats):
        LOG.append("  f%-4d %5.2fs  %s" % (frame, frame / float(FPS), what))
    LOG.append("")
    LOG.append("strays " + ", ".join("%s %.0f units at %+.0f deg/frame" % (piece.bone, piece.stray, piece.orbit)
                                     for piece in PIECES))
    LOG.append("turns " + ", ".join("%s %+.0f deg over the clip, fastest %.1f deg/frame" % (
        piece.bone, turns[piece.bone], top_spin(piece, spun, turns)) for piece in PIECES)
        + " (reads backwards past %.0f)" % STROBE)
    LOG.append("spinning up square from f%d, off the axis from f%d — %.2fs of turning before anything strays"
               % (FALTER, STRAY_FROM, (STRAY_FROM - FALTER) / float(FPS)))
    LOG.append("thrown " + ", ".join("%s %.0f units on %.0f deg" % (piece.bone, piece.reach, piece.angle)
                                     for piece in PIECES))
    LOG.append("")
    LOG.append("frame  drive  body   shake    outer                 mid                   core"
               "                blade   piece gaps          silhouette")
    LOG.append("                       x   y  dist    yaw   scale   dist    yaw   scale   dist"
               "    yaw   scale   past   o-m    m-c    o-c  radius")

    marks = sorted(set(list(range(0, FRAMES + 1, SAMPLE)) + [frame for frame, _ in beats]))
    for frame in marks:
        local, past, radius, gaps = measured[frame]
        row = "%5d %+6.2f %6.3f %4.0f %3.0f " % ((frame, drive(frame), local[ROOT][2][0])
                                                 + local[ROOT][0][:2]) + \
              " ".join("%6.0f %6.1f %6.3f " % (
                  math.hypot(local[piece.bone][0][0], local[piece.bone][0][1]),
                  local[piece.bone][1], local[piece.bone][2][0]) for piece in PIECES)
        LOG.append(row + " %6.1f %6.0f %6.0f %6.0f %7.0f  %s" % (
            max(past.values()), gaps[(OUTER, MID)], gaps[(MID, CORE)], gaps[(OUTER, CORE)],
            radius, "#" * int(round(radius / 60.0))))

    LOG.append("")
    LOG.append("peak silhouette %.0f units across, against a %.0f-unit default OrthoWidth; at f0 it is %.0f,"
               " and the resting wreckage spans %.0f" % (
                   2.0 * max(entry[2] for entry in measured.values()), VIEW_WIDTH, 2.0 * measured[0][2],
                   2.0 * measured[FRAMES][2]))
    if 2.0 * measured[FRAMES][2] > VIEW_WIDTH:
        raise RuntimeError("the pieces come to rest {:.0f} units across, past a {:.0f}-unit view".format(
            2.0 * measured[FRAMES][2], VIEW_WIDTH))

    # Nothing but the gather and the blast may change a size, or the wander reads as the boss inflating.
    sizes = [(frame, measured[frame][0][ROOT][2][0],
              max(measured[frame][0][piece.bone][2][0] for piece in PIECES))
             for frame in range(BLAST - GATHER + 1)]
    moved = [frame for frame, body, ring in sizes if abs(body - 1.0) > 0.001 or abs(ring - 1.0) > 0.001]
    LOG.append("size held flat until the gather starts on f%d: %s" % (
        BLAST - GATHER, "yes" if not moved else "CHANGED on f%d" % min(moved)))
    if moved:
        raise RuntimeError("something scales on f{}, before the gather that is supposed to own it".format(
            min(moved)))

    # And nothing leaves the axis while the boss is only spinning up, or the two beats read as one.
    strayed_early = [frame for frame in range(STRAY_FROM + 1)
                     if max(math.hypot(*measured[frame][0][piece.bone][0][:2]) for piece in PIECES) > 0.001]
    LOG.append("the three hold the axis until f%d: %s" % (
        STRAY_FROM, "yes" if not strayed_early else "STRAYED on f%d" % min(strayed_early)))
    if strayed_early:
        raise RuntimeError("a ring leaves the axis on f{}, while the boss is still only turning".format(
            min(strayed_early)))

    # Up to the detonation the boss is one object, however badly it is behaving: the pieces still touch.
    for index, piece in enumerate(PIECES):
        for other in PIECES[index + 1:]:
            apart = [frame for frame in range(BLAST + 1) if measured[frame][3][(piece.bone, other.bone)] >= 0.0]
            LOG.append("%s and %s hold together until the blast: %s" % (
                piece.bone, other.bone, "yes" if not apart else "APART on f%d..f%d" % (min(apart), max(apart))))
            if apart:
                raise RuntimeError("{} and {} come apart on f{}, before the detonation does it".format(
                    piece.bone, other.bone, min(apart)))

    # And from it they are three: a pair still merged when everything stops never reads as two pieces at all.
    for index, piece in enumerate(PIECES):
        for other in PIECES[index + 1:]:
            clear = separated_from(measured, piece, other)
            LOG.append("%s and %s are two pieces from f%d, %d frames before the flight stops on f%d" % (
                piece.bone, other.bone, clear, BLAST + FLIGHT - clear, BLAST + FLIGHT))
            if BLAST + FLIGHT - clear < CLEAR_BEFORE:
                raise RuntimeError("{} and {} only come apart {} frames before everything stops".format(
                    piece.bone, other.bone, BLAST + FLIGHT - clear))

    # The idle hands over the reference pose with the blades folded, so those answer to the protrusion line.
    rest = {name: local for name, (local, _) in toolkit["reference_pose_table"](SKELETON_PATH).items()
            if SPIKE_MARK not in name}
    first = measured[0][0]
    gaps = [(math.dist(first[bone][0], (transform.translation.x, transform.translation.y,
                                        transform.translation.z)),
             abs(first[bone][1] - transform.rotation.rotator().yaw),
             max(abs(a - b) for a, b in zip(first[bone][2], (transform.scale3d.x, transform.scale3d.y,
                                                             transform.scale3d.z))))
            for bone, transform in rest.items()]
    LOG.append("first key against the reference pose over the %d bones the idle leaves on it: translation"
               " %.4f units, yaw %.4f deg, scale %.5f" % (len(rest), max(g[0] for g in gaps),
                                                          max(g[1] for g in gaps), max(g[2] for g in gaps)))
    LOG.append("blades: %.1f units past their faces at f0 (negative = folded away), %.1f at the worst of the"
               " ratchet, %.1f where they come to rest" % (
                   max(measured[0][1].values()),
                   max(max(measured[frame][1].values()) for frame in range(FALTER, BLAST)),
                   max(measured[FRAMES][1].values())))
    still = max(math.dist(measured[FRAMES][0][bone][0], measured[FRAMES - 1][0][bone][0])
                for bone in measured[FRAMES][0])
    LOG.append("the last two frames differ by %.4f units, so the wreckage is holding still" % still)
    LOG.append("montage sections read back: {}, blending in over {:.2f}s".format(
        toolkit["montage_sections"](montage),
        montage.get_editor_property("blend_in").get_editor_property("blend_time")))


try:
    # Loaded the same way a script is run, so the rig and silhouette maths lives in one place.
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    reference = toolkit["_component_transforms"](APE.get_reference_pose(unreal.load_asset(SKELETON_PATH)))
    layout = toolkit["feature_directions"](reference, SPIKE_MARK)
    groups = toolkit["rigid_vertex_groups"](MESH_PATH, SKELETON_PATH)
    missing = [piece.bone for piece in PIECES if piece.bone not in reference]
    if missing or not layout:
        raise RuntimeError("skeleton is missing rings {} or has no {} bones".format(missing, SPIKE_MARK))

    # One rate shape drives all three, so one table serves them and only the totals tell them apart.
    spun = toolkit["normalised_spin"](spin_rate, FRAMES)
    turns = spin_turns(spun)
    fast = [(piece.bone, top_spin(piece, spun, turns)) for piece in PIECES
            if top_spin(piece, spun, turns) >= STROBE]
    if fast:
        raise RuntimeError("{} turn at {} deg/frame, which reads as going backwards".format(
            [bone for bone, _ in fast], ["%.1f" % speed for _, speed in fast]))

    sequence = build_sequence(toolkit, layout, spun, turns)
    montage = build_montage(sequence, toolkit)
    report(sequence, montage, toolkit, measure(sequence, toolkit, groups, layout), spun, turns)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
