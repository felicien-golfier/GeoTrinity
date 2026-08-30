"""Hex boss intro: three dead pieces that find each other, lock together and wake up.

The clip opens on the boss in bits — the three hexagons lying still and far apart, each tilted out of alignment
and each with its blades folded away. They shiver where they lie, drift in toward the centre, then slam home one
at a time, outermost first, each landing on a frame of its own with the whole body kicking. A piece that has
landed does not wait quietly: it turns, judders and keeps a little of its blade out, so the boss gains one live
part at a time rather than appearing whole.

Once all three are turning — the middle ring against the other two — they accelerate together, draw in and stop
dead. Then the boss blows open: every ring out, every blade out, the body far past anything a fight ever asks of
it, since nothing here is aimed at a player and no capsule has to cover it. It settles back over two slow
seconds, folds its blades and lands exactly on the reference pose, which is where the idle picks it up.

There are two clocks. Up to the point all three are home each piece runs on its own — its own shiver, its own
creep, its own slam — because one at a time is the whole point of that half. From there one curve drives
everything, each ring reading it a fixed number of frames late, exactly as the ability clips do.

Constraints:
- The clip does not start on the reference pose. It starts scattered, so the montage blends in over nothing.
- It ends on the reference pose with the blades folded, so the idle takes over without a pop.
- Every ring's net turn is a whole number of sixths, which map a hexagon onto itself: no spin is ever unwound.
- No ring crosses more than STROBE degrees between two frames, past which it reads as turning the other way.
- Only the fly-in may interpenetrate — a piece passing through the ring already home is the assembly. From its
  own landing on, every ring stays inside the shell around it.
- The blast's overshoot is carried by the body alone: a uniform scale on the root cannot make two parts collide,
  while the ring scales and blade reaches that can are held at their configured values.
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
SEQ_NAME = "SK_HexBoss_Sequence_Intro"
MONTAGE_NAME = "SK_HexBoss_Montage_Intro"
REPORT = ("C:/Users/Felou/AppData/Local/Temp/claude/C--GeoTrinity/"
          "d01a11ac-5e89-4af5-9d0a-7a9d27ea8837/scratchpad/hex_boss_intro.txt")

ROOT = "Root"
OUTER, MID, CORE = "HexOuter", "HexMid", "HexCore"
SPIKE_MARK = "_Spike_"
STROBE = 30.0        # half the 60 degrees that map a hexagon onto itself
VIEW_WIDTH = 3000.0  # the camera volumes' default OrthoWidth, to read the peak silhouette against

FPS = 30
SAMPLE = 4           # frames between report rows
CROSSING = 2         # frames a piece still on its way in may overlap one already home — the slam, and no more

# --- The assembly: when each piece wakes, drifts and lands ---------------------------------------
WAKE = 6             # frames of dead stillness before anything shivers
CREEP_START = 16     # every piece starts drifting inward here, each reaching its own staging point
ANTIC = 5            # frames it pulls back out again before the slam
STILL = 3            # frames stopped dead at the pulled-back extreme
CROSS = 0.45         # of the staging distance the piece is at on the one mid-flight frame
PULL = 70.0          # units it draws back out over ANTIC
LAND_SPRING = [1.0, -0.30, 0.10]        # of its overshoot, from the landing frame: past the centre, then settling
LAND_SQUASH = [0.94, 1.06, 0.99, 1.02]  # the ring's own scale over those same frames

# --- The finale: everything turning, gathering, blowing open and settling -------------------------
ALL_ALIVE = 104      # the three turn together up to here, then accelerate into the gather
WIND_STILL = 145     # first frozen frame — the gather is tightest and the spin has stopped dead
WIND = 148           # last frozen frame
BLAST = WIND + 1
BURST = [0.45, 1.30, 1.0]  # the mid-flight frame, the overshoot that lands the blast, then full extension
HOLD = 14            # frames the blown-open boss is held past the burst
SETTLE = 50          # frames of the slow spring back into the idle

CHARGE_EASE = 2.4    # above 1 the gather accelerates instead of closing at a fixed speed
SHAKE_RAMP = 2.0     # above 1 the gather's rattle keeps out of the way until it is nearly tight
DAMP = 2.4           # how fast the settle dies
SPRING = 3.8         # how far past rest it swings on the way
YAW_EASE = 0.45      # below 1 a piece holds its tilt while it drifts and loses it on the slam

TREMBLE = 9.0        # units a scattered piece shivers at its loudest
TREMBLE_RAMP = 2.2
JUDDER = 3.5         # units a landed ring keeps working at while it waits for the others
BREATH = 0.02        # fraction of itself a waiting ring swells and shrinks by
BREATH_PERIOD = 22

IMPACT_SHAKE = 26.0  # units the whole boss kicks on a landing
IMPACT_POP = 0.09    # fraction it swells by on the same frame
IMPACT_DECAY = 0.42  # per frame
WIND_SHAKE = 20.0    # units it rattles at the tightest of the gather

FLARE = [1.0, 0.86, 0.62, 0.42, 0.28, 0.19]  # of its flare reach, from the landing frame
TEETH = 0.09         # the blade a landed ring keeps out afterwards, as a fraction of that flare

ALIVE_SPIN = 1.0     # turning rate while a landed ring waits; the table normalises the units away
SPIN_PEAK = 12.0     # rate at the top of the acceleration
BLAST_SPIN = 0.9     # of that peak, the rate through the blast and the hold

BODY_CHARGE, BODY_HOLD, BODY_PUNCH = 0.70, 1.14, 1.55  # the whole boss gathered, held open, and on the blast

# (bone, direction it lies in, units out where it lies, units out it drifts to, degrees out of alignment it lies
#  at, units it overshoots the centre by, the frame it lands, frames its shiver is offset, how hard its landing
#  kicks the body, its blade reach on that landing, sixths of a turn it makes over the whole clip, frames it
#  reads the finale curve late, its scale gathered, its scale blown open, its blade reach there)
Piece = collections.namedtuple(
    "Piece", "bone angle far near yaw over land beat boom flare sixths lag charge_scale burst_scale burst_spike")

# Outermost first, each landing sooner after the last than the one before it. The shell blows out furthest of the
# three, which is what buys the rings inside it room for blades of their own. The middle ring turns against the
# other two, as it does in the idle.
#
# A piece drifts only as close as it can while still clearing whatever is already home — a ring waiting its turn
# at less than the shell's radius plus its own sits inside it for the whole wait, which reads as a mistake rather
# than as a piece that has not arrived. The slam is what crosses that shell, on the one mid-flight frame.
PIECES = [
    Piece(OUTER, 165.0, 880.0, 540.0, 27.0, 34.0, 40, 0, 0.70, 1.35, 12, 3, 0.84, 1.55, 2.30),
    Piece(MID, 25.0, 950.0, 760.0, -29.0, 22.0, 66, 1, 0.85, 0.55, -16, 2, 0.88, 1.15, 1.65),
    Piece(CORE, 300.0, 800.0, 620.0, 24.0, 16.0, 88, 3, 1.00, 1.30, 22, 0, 1.18, 1.08, 1.60),
]

RINGS = {piece.bone: piece for piece in PIECES}
MAX_LAG = max(piece.lag for piece in PIECES)
PLATEAU = BLAST + len(BURST) - 1 + MAX_LAG + HOLD  # last frame of full extension; outlasts the largest delay
FRAMES = PLATEAU + SETTLE + MAX_LAG                # the settle gets the longest lag on top, so every ring rests

LOG = []


def smoothstep(alpha):
    alpha = min(1.0, max(0.0, alpha))
    return alpha * alpha * (3.0 - 2.0 * alpha)


def charged(frame):
    """0 to 1 across the gather, accelerating; 1 from the frozen frames on."""
    return min(1.0, max(0.0, frame - ALL_ALIVE) / float(WIND_STILL - 1 - ALL_ALIVE)) ** CHARGE_EASE


def drive(frame):
    """-1 gathered tightest, +1 blown open, 0 at rest — the one curve the finale's parts all follow."""
    if frame < WIND_STILL:
        return -charged(frame)
    if frame <= WIND:
        return -1.0
    if frame < BLAST + len(BURST):
        return BURST[frame - BLAST]
    if frame <= PLATEAU:
        return BURST[-1]
    alpha = (frame - PLATEAU) / float(SETTLE)
    return 0.0 if alpha >= 1.0 else math.exp(-DAMP * alpha) * math.cos(SPRING * alpha) * (1.0 - alpha)


def lagged(frame, piece):
    """The curve this ring reads, delayed by its own weight, the delay reversing while the boss is open.

    Opening, the shell has to lead — an inner ring swelling into one that has not opened goes straight through it.
    Closing wants the other order, and the plateau between outlasts the largest delay so nothing jumps on the flip.
    """
    delay = MAX_LAG - piece.lag if BLAST <= frame <= PLATEAU else piece.lag
    return drive(frame - delay)


def impact(frame):
    """Strength 0 to 1 of the most recent landing, decaying."""
    live = [piece.boom * math.exp(-IMPACT_DECAY * (frame - piece.land)) for piece in PIECES if frame >= piece.land]
    return max(live) if live else 0.0


def offset(frame, piece):
    """Units from the boss's centre along the piece's own direction: `far` where it lies, 0 once it is home.

    The tail of the approach is a hit's three beats — a drift inward that eases, an anticipation that pulls back
    out and stops dead, then a two-frame crossing with one frame mid-flight and an overshoot past the centre that
    it springs off.
    """
    step = frame - piece.land
    if step >= len(LAND_SPRING):
        return 0.0
    if step >= 0:
        return -piece.over * LAND_SPRING[step]
    staging = piece.near + PULL
    if step == -1:
        return staging * CROSS
    still_start = piece.land - 1 - STILL
    if frame >= still_start:
        return staging
    if frame >= still_start - ANTIC:
        return piece.near + PULL * smoothstep((frame - still_start + ANTIC) / float(ANTIC))
    if frame <= CREEP_START:
        return piece.far
    return piece.far + (piece.near - piece.far) * smoothstep(
        (frame - CREEP_START) / float(still_start - ANTIC - CREEP_START))


def shiver(frame, piece):
    """The piece's own offset this frame -> (x, y): a shiver while it lies out there, a judder once it is home.

    Alternates every frame, since slower reads as a wobble, and dies for the frames before the slam — that
    stillness is what the slam lands against. Offset per piece so the three never shake in step, and fading out
    across the gather, where the parts stop being three things.
    """
    if frame < WAKE:
        return 0.0, 0.0
    still_start = piece.land - 1 - STILL
    if frame < still_start:
        reach = TREMBLE * (min(1.0, (frame - WAKE) / float(still_start - WAKE)) ** TREMBLE_RAMP)
    elif frame < piece.land:
        return 0.0, 0.0
    else:
        reach = JUDDER * (1.0 - charged(frame))
    beat = frame + piece.beat
    return (reach if beat % 2 else -reach), (reach * 0.6 if beat % 4 < 2 else -reach * 0.6)


def rattle(frame):
    """The whole boss's offset this frame -> (x, y): a landing's kick, then the gather's load.

    Dead through the frozen frames, which is what the blast lands against.
    """
    reach = IMPACT_SHAKE * impact(frame)
    if ALL_ALIVE <= frame < WIND_STILL:
        reach += WIND_SHAKE * charged(frame) ** SHAKE_RAMP
    return (reach if frame % 2 else -reach), (reach * 0.6 if frame % 4 < 2 else -reach * 0.6)


def spin_rate(frame, piece):
    """How fast the piece is turning at `frame`, in units the spin table normalises away."""
    if frame <= piece.land:
        return 0.0
    if frame < ALL_ALIVE:
        return ALIVE_SPIN
    if frame < WIND_STILL:
        return ALIVE_SPIN + (SPIN_PEAK - ALIVE_SPIN) * charged(frame)
    if frame <= WIND:
        return 0.0
    if frame <= PLATEAU:
        return SPIN_PEAK * BLAST_SPIN
    return SPIN_PEAK * BLAST_SPIN * (1.0 - smoothstep((frame - PLATEAU) / float(SETTLE)))


def spin_table(toolkit):
    """Each piece's turn so far as a fraction of its total, per frame -> {bone: [fraction]}.

    Integrated from the rate rather than written as an angle: a ring is then free to accelerate however it likes
    and still land on the whole sixths that map a hexagon onto itself, so the clip ends on an orientation the eye
    cannot tell from rest without anything ever being unwound.
    """
    return {piece.bone: toolkit["normalised_spin"](lambda frame, piece=piece: spin_rate(frame, piece), FRAMES)
            for piece in PIECES}


def ring_yaw(frame, piece, spun):
    """Degrees the ring has turned: the tilt it lies at, closing as it comes home, plus its own spin.

    The tilt is read off the distance it still has to travel, so one curve gives the drift, the anticipation's
    turn back and the overshoot past alignment for nothing.
    """
    distance = offset(frame, piece) / piece.far
    return piece.yaw * math.copysign(abs(distance) ** YAW_EASE, distance) \
        + 60.0 * piece.sixths * spun[piece.bone][frame]


def ring_scale(frame, piece):
    """The ring's XY scale: its landing bounce and its breath while it waits, then the finale's curve.

    Clamped at full extension — the blast's overshoot is the body's, which is uniform and so cannot put one part
    through another.
    """
    value = lagged(frame, piece)
    factor = 1.0 + (min(1.0, value) * (piece.burst_scale - 1.0) if value > 0.0
                    else value * (1.0 - piece.charge_scale))
    step = frame - piece.land
    if 0 <= step < len(LAND_SQUASH):
        factor *= LAND_SQUASH[step]
    if step >= 0:
        factor *= 1.0 + BREATH * (1.0 - charged(frame)) * math.sin(2.0 * math.pi * step / float(BREATH_PERIOD))
    return factor


def body_scale(frame):
    """The whole boss's XY scale: a kick on each landing, then gathered, punched and held open.

    The overshoot alone drives the punch and everything below it the hold, so the frame nothing could be hit at
    is the blast's and no other.
    """
    value = drive(frame)
    if value < 0.0:
        factor = 1.0 + value * (1.0 - BODY_CHARGE)
    else:
        overshoot = max(0.0, value - 1.0) / (max(BURST) - 1.0)
        factor = 1.0 + min(1.0, value) * (BODY_HOLD - 1.0) + overshoot * (BODY_PUNCH - BODY_HOLD)
    return factor * (1.0 + IMPACT_POP * impact(frame))


def teeth(frame, piece):
    """The blade a landed ring shows: a stab on the landing frame, decaying to what it keeps out afterwards."""
    if not piece.land <= frame < WIND_STILL:
        return 0.0
    step = frame - piece.land
    return piece.flare * (FLARE[step] if step < len(FLARE) else TEETH)


def spike_reach(frame, piece):
    """A spike's X scale: 0 folded into its face, 1 the blade the mesh gives it, past that a stretched one.

    Every blade of a ring alike — the boss is aimed at nothing here — and whatever it is showing is drawn back in
    by the gather before the blast throws it out again.
    """
    value = lagged(frame, piece)
    if value < 0.0:
        return teeth(frame, piece) * (1.0 + value)
    return max(teeth(frame, piece), min(1.0, value) * piece.burst_spike)


def key(frame, bone, rest_local, layout, spun):
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
        distance = offset(frame, piece)
        shiver_x, shiver_y = shiver(frame, piece)
        x += distance * math.cos(math.radians(piece.angle)) + shiver_x
        y += distance * math.sin(math.radians(piece.angle)) + shiver_y
        yaw += ring_yaw(frame, piece, spun)
        factor = ring_scale(frame, piece)
        sx, sy = sx * factor, sy * factor
    elif bone in layout:
        sx = spike_reach(frame, RINGS[layout[bone][0]])
    return unreal.Vector(x, y, z), unreal.Rotator(yaw=yaw).quaternion(), unreal.Vector(sx, sy, sz)


def top_spin(piece, spun):
    """The most the ring turns between two frames, in degrees."""
    return 60.0 * abs(piece.sixths) * max(spun[piece.bone][frame] - spun[piece.bone][frame - 1]
                                          for frame in range(1, FRAMES + 1))


def build_sequence(toolkit, layout, spun):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    sequence = toolkit["get_or_create_asset"](ANIM_PACKAGE, SEQ_NAME, unreal.AnimSequence, factory)
    toolkit["write_bone_tracks"](sequence, SKELETON_PATH, FPS, FRAMES,
                                 lambda frame, bone, rest_local: key(frame, bone, rest_local, layout, spun),
                                 "Build hex boss intro")
    return sequence


def build_montage(sequence, toolkit):
    """One section that plays through: nothing jumps around inside an intro.

    Blends in over nothing — a blend would drag the scattered pieces out of whatever pose the boss was already in.
    """
    montage = toolkit["build_montage"](sequence, ANIM_PACKAGE, MONTAGE_NAME, ["Start"], [0.0], ["None"])
    for name, seconds in (("blend_in", 0.0), ("blend_out", 0.15)):
        blend = montage.get_editor_property(name)
        blend.set_editor_property("blend_time", seconds)
        montage.set_editor_property(name, blend)
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(ANIM_PACKAGE, MONTAGE_NAME))
    return montage


def measure(sequence, toolkit, groups, layout):
    """Per frame -> (local pose, {spike: units past its face}, (mid in outer, core in mid), radius, separations).

    Every frame rather than the printed rows: the pieces cross each other over two or three frames a coarse
    sample steps straight past.
    """
    options = unreal.AnimPoseEvaluationOptions()
    members = {piece.bone: [bone for bone in groups if bone.startswith(piece.bone)] for piece in PIECES}
    measured = {}
    for frame in range(FRAMES + 1):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        local = toolkit["local_pose_table"](pose)
        posed = toolkit["_component_transforms"](pose)
        placed = toolkit["place_groups"](groups, posed)
        past = {spike: toolkit["feature_protrusion"](placed, posed, spike, parent)
                for spike, (parent, _) in layout.items()}
        measured[frame] = (local, past,
                           (toolkit["nested_clearance"](placed, members[MID], OUTER),
                            toolkit["nested_clearance"](placed, members[CORE], MID)),
                           max(math.hypot(v.x, v.y) for group in placed.values() for v in group),
                           toolkit["part_separations"](placed, posed, members))
    return measured


def report(sequence, montage, toolkit, measured, spun):
    beats = [(0, "scattered, dead still"), (WAKE, "the pieces start to shiver"),
             (CREEP_START, "they start drifting toward the centre")]
    for piece in PIECES:
        beats.append((piece.land - 1 - STILL, "{} stops dead, pulled back".format(piece.bone)))
        beats.append((piece.land, "{} SLAMS home".format(piece.bone)))
    beats += [(ALL_ALIVE, "all three turning together — the acceleration starts"),
              (WIND_STILL, "frozen: gathered tightest, spin stopped dead"),
              (BLAST + 1, "BLAST — the overshoot frame"),
              (PLATEAU, "held open until here, then the slow settle"),
              (FRAMES, "back on the reference pose, blades folded")]

    LOG.append("{} — {} frames at {} fps = {:.2f}s, {} sampled keys (expect {})".format(
        SEQ_NAME, FRAMES, FPS, FRAMES / float(FPS), toolkit["playable_key_count"](sequence), FRAMES + 1))
    LOG.append("")
    for frame, what in sorted(beats):
        LOG.append("  f%-4d %5.2fs  %s" % (frame, frame / float(FPS), what))
    LOG.append("")
    LOG.append("pieces lie at " + ", ".join("%s %.0f units on %.0f deg, %.0f deg out of true" % (
        piece.bone, piece.far, piece.angle, piece.yaw) for piece in PIECES))
    LOG.append("net turn " + ", ".join("%s %+d sixths, fastest %.1f deg/frame" % (
        piece.bone, piece.sixths, top_spin(piece, spun)) for piece in PIECES)
        + " (reads backwards past %.0f)" % STROBE)
    LOG.append("blown open: body %.2f on the blast and %.2f held, rings %s, blades %s" % (
        BODY_PUNCH, BODY_HOLD, "/".join("%.2f" % piece.burst_scale for piece in PIECES),
        "/".join("%.2f" % piece.burst_spike for piece in PIECES)))
    LOG.append("")
    LOG.append("frame  drive  body   shake    outer                 mid                   core"
               "                blade  room inside   silhouette")
    LOG.append("                       x   y  dist    yaw   scale   dist    yaw   scale   dist"
               "    yaw   scale   past   mid    core  radius")

    marks = sorted(set(list(range(0, FRAMES + 1, SAMPLE)) + [frame for frame, _ in beats]))
    for frame in marks:
        local, past, gaps, radius, _ = measured[frame]
        row = "%5d %+6.2f %6.3f %4.0f %3.0f " % ((frame, drive(frame), local[ROOT][2][0])
                                                 + local[ROOT][0][:2]) + \
              " ".join("%6.0f %6.1f %6.3f " % (
                  math.hypot(local[piece.bone][0][0], local[piece.bone][0][1]),
                  local[piece.bone][1], local[piece.bone][2][0]) for piece in PIECES)
        LOG.append(row + " %6.1f %6.0f %6.0f %7.0f  %s" % (
            max(past.values()), gaps[0], gaps[1], radius, "#" * int(round(radius / 60.0))))

    LOG.append("")
    LOG.append("peak silhouette %.0f units across, against a %.0f-unit default OrthoWidth; at rest it is %.0f" % (
        2.0 * max(entry[3] for entry in measured.values()), VIEW_WIDTH, 2.0 * measured[FRAMES][3]))
    LOG.append("scattered at frame 0 it spans %.0f units across" % (2.0 * measured[0][3]))

    # Scattered, the pieces are separate objects and have to read as separate objects. The only frames a pair may
    # touch before the later one is home are the slam's.
    for index, piece in enumerate(PIECES):
        for other in PIECES[index + 1:]:
            gaps = [(measured[frame][4][(piece.bone, other.bone)], frame) for frame in range(other.land)]
            touching = [frame for gap, frame in gaps if gap < 0.0]
            clear = min(gap for gap, _ in gaps if gap >= 0.0)
            LOG.append("%s and %s: %.0f units apart at the closest of f0..f%d, touching on %s" % (
                piece.bone, other.bone, clear, other.land - 1, touching or "no frame"))
            if len(touching) > CROSSING:
                raise RuntimeError("{} and {} overlap on {} frames before {} lands, not just the slam".format(
                    piece.bone, other.bone, len(touching), other.bone))

    # Once home, a ring stays inside the shell around it; the frames it flies through that shell are the slam's.
    for label, index, landed in (("mid inside outer", 0, RINGS[MID].land), ("core inside mid", 1, RINGS[CORE].land)):
        room, tightest = min((measured[frame][2][index], frame) for frame in range(landed, FRAMES + 1))
        LOG.append("%s: %.1f units clear at the closest, on f%d%s" % (
            label, room, tightest, "" if room > 0.0 else "  <-- THEY OVERLAP"))
        if room <= 0.0:
            raise RuntimeError("{} overlap by {:.1f} units on f{}".format(label, -room, tightest))

    # The blades end folded, which is their idle pose and not their reference scale, so they answer to the
    # protrusion line below instead of to this one.
    rest = {name: local for name, (local, _) in toolkit["reference_pose_table"](SKELETON_PATH).items()
            if SPIKE_MARK not in name}
    last = measured[FRAMES][0]
    gaps = [(math.dist(last[bone][0], (transform.translation.x, transform.translation.y, transform.translation.z)),
             abs((last[bone][1] - transform.rotation.rotator().yaw + 30.0) % 60.0 - 30.0),
             max(abs(a - b) for a, b in zip(last[bone][2], (transform.scale3d.x, transform.scale3d.y,
                                                            transform.scale3d.z))))
            for bone, transform in rest.items()]
    LOG.append("last key against the reference pose over the %d bones that return to it: translation %.4f units,"
               " yaw %.4f deg (mod 60), scale %.5f" % (len(rest), max(g[0] for g in gaps),
                                                       max(g[1] for g in gaps), max(g[2] for g in gaps)))
    LOG.append("blades: %.1f units past their faces at frame 0, %.1f at frame %d (negative = folded away),"
               " %.1f at the blast" % (max(measured[0][1].values()), max(measured[FRAMES][1].values()), FRAMES,
                                       max(max(measured[frame][1].values())
                                           for frame in range(BLAST, PLATEAU + 1))))
    LOG.append("montage sections read back: {}, blending in over {:.2f}s".format(
        toolkit["montage_sections"](montage),
        montage.get_editor_property("blend_in").get_editor_property("blend_time")))


try:
    # Loaded the same way a script is run, so the silhouette maths lives in one place.
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    reference = toolkit["_component_transforms"](APE.get_reference_pose(unreal.load_asset(SKELETON_PATH)))
    layout = toolkit["feature_directions"](reference, SPIKE_MARK)
    groups = toolkit["rigid_vertex_groups"](MESH_PATH, SKELETON_PATH)
    missing = [piece.bone for piece in PIECES if piece.bone not in reference]
    if missing or not layout:
        raise RuntimeError("skeleton is missing rings {} or has no {} bones".format(missing, SPIKE_MARK))

    spun = spin_table(toolkit)
    fast = [(piece.bone, top_spin(piece, spun)) for piece in PIECES if top_spin(piece, spun) >= STROBE]
    if fast:
        raise RuntimeError("{} turn at {} deg/frame, which reads as going backwards".format(
            [bone for bone, _ in fast], ["%.1f" % speed for _, speed in fast]))

    sequence = build_sequence(toolkit, layout, spun)
    montage = build_montage(sequence, toolkit)
    report(sequence, montage, toolkit, measure(sequence, toolkit, groups, layout), spun)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
