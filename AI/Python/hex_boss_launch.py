"""Hex boss launch animations: the tile bomb and the tile turrets.

Two very short clips, one beat each. Neither ability is a pattern — both are server-only, spawn a replicated
deployable and end — so these hang off the base ability's own montage property rather than off a pattern. That
gives two sections and no loop: Start gathers and is stretched to the ability's FireDelay, Fire1 throws and
springs back, and nothing ever jumps to an End section on a fixed-delay ability, so the montage just plays out.

Neither ability has a direction. The bomb lands on whichever player is drawn and the turrets on whichever tiles
are still standing, so nothing here may point: every blade extends alike, no ring leans, and the only translation
is the wind-up rattle, which alternates rather than travels. What reads as a launch is the shape gathering inward
and then blowing outward all at once, not a throw aimed anywhere.

The rest is the muscle the ability clips are cut from: one curve every part reads a fixed number of frames late,
a wind-up that accelerates, a few frames stopped dead, a two-frame crossing into an overshoot, and a spring back
through rest. Each ring also twists against that curve — back while the boss gathers, through and past as it
throws — since anticipation running opposite to the action is what makes a release read as a throw.

Both start and end on the reference pose with the spikes folded, so they blend against the idle. Nothing moves
along Z: under an orthographic camera that motion is spent for nothing.

Run via mcp-unreal execute_script. Re-runnable: rewrites both sequences and montages in place.
Report written to REPORT.
"""
import collections
import math

import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/HexBoss/SK_HexBoss"
MESH_PATH = "/Game/Characters/Meshes/HexBoss/SKM_HexBoss"
ANIM_PACKAGE = "/Game/Characters/Anim/HexBoss"
REPORT = ("C:/Users/Felou/AppData/Local/Temp/claude/C--GeoTrinity/"
          "0b3b8214-c633-4920-a4b1-c68d4e9de203/scratchpad/hex_boss_launch.txt")

ROOT = "Root"
OUTER, MID, CORE = "HexOuter", "HexMid", "HexCore"
SPIKE_MARK = "_Spike_"
STROBE = 30.0       # half the 60 degrees that map a hexagon onto itself: past this a ring reads as turning back

FPS = 30
SAMPLE = 3          # frames between report rows — these clips are too short for anything coarser

CHARGE_EASE = 2.4   # above 1 the gather accelerates instead of closing at a fixed speed
SHAKE_RAMP = 2.0    # above 1 the rattle keeps out of the way until the gather is nearly tight
STILL = 3           # frames stopped dead at the tightest point of the gather
BURST = [0.45, 1.15, 1.0]  # the mid-flight frame, the overshoot that lands the throw, then full extension
DAMP = 2.0          # how fast the spring back dies
SPRING = 4.8        # how far past rest it swings on the way

# (frames this ring lags the curve, its scale gathered tightest, its scale at full throw, degrees it winds the
#  wrong way while gathering — the anticipation the throw unwinds)
Ring = collections.namedtuple("Ring", "lag gather_scale throw_scale twist")

# (asset suffix, gather frames, spring-back frames, units the gather rattles at its tightest, spike reach
#  gathered, spike reach thrown, the rings)
Clip = collections.namedtuple("Clip", "name gather recoil shake gather_spike throw_spike rings")

CLIPS = [
    # Tile bomb: one heavy object lobbed at one player. The deepest gather of the two and the slowest, with the
    # core swelling against the shrinking rings so the shape looks loaded, then the whole boss blowing open on one
    # frame. The core carries almost all of the twist: it is the part doing the throwing.
    Clip("Bomb", 21, 12, 6.0, 0.35, 1.00, {
        OUTER: Ring(3, 0.80, 1.18, 8.0),
        MID: Ring(2, 0.84, 1.10, -12.0),
        CORE: Ring(0, 1.24, 1.06, 18.0)}),
    # Tile turrets: several lighter things shed onto tiles at once. Quicker and shallower, and the twist sits on
    # the outer ring as much as the core so the release reads as the rim flicking them away rather than as one
    # object leaving the middle.
    Clip("Turret", 15, 10, 4.0, 0.25, 0.90, {
        OUTER: Ring(3, 0.88, 1.20, -14.0),
        MID: Ring(2, 0.90, 1.08, 10.0),
        CORE: Ring(0, 1.18, 1.10, -18.0)}),
]

MAX_LAG = max(ring.lag for clip in CLIPS for ring in clip.rings.values())

LOG = []


def throw_end(clip):
    """The last frame of full extension.

    Held for at least the largest delay past the throw, so every ring reads the same value at both ends of that
    hold — which is what lets the delay reverse across it without any ring jumping.
    """
    return clip.gather + len(BURST) - 1 + MAX_LAG


def total_frames(clip):
    """The spring back is given the longest lag on top, so even the ring that drags most reaches rest by the last
    key."""
    return throw_end(clip) + clip.recoil + MAX_LAG


def charged(frame, clip):
    """0 to 1 across the gather, accelerating; 1 once the boss has stopped dead."""
    return min(1.0, max(0.0, frame) / float(clip.gather - STILL - 1)) ** CHARGE_EASE


def drive(frame, clip):
    """-1 gathered tightest, +1 at full throw, 0 at rest — the one curve every part follows."""
    if frame < clip.gather - STILL:
        return -charged(frame, clip)
    if frame < clip.gather:
        return -1.0
    if frame < clip.gather + len(BURST):
        return BURST[frame - clip.gather]
    if frame <= throw_end(clip):
        return BURST[-1]
    alpha = (frame - throw_end(clip)) / float(clip.recoil)
    return 0.0 if alpha >= 1.0 else math.exp(-DAMP * alpha) * math.cos(SPRING * alpha) * (1.0 - alpha)


def lagged(frame, clip, ring):
    """The curve this ring reads, delayed by its own weight — but the delay reverses while the boss opens.

    Light parts leading and heavy ones dragging is what keeps the boss from moving as one block, and closing is
    the direction that wants. Opening, it is the wrong way round: an inner ring swelling into a shell that has not
    opened yet goes straight through it. So across the throw the order flips, and the outer ring is first open and
    last shut.
    """
    delay = MAX_LAG - ring.lag if clip.gather <= frame <= throw_end(clip) else ring.lag
    return drive(frame - delay, clip)


def rattle(frame, clip):
    """The whole boss's offset this frame -> (x, y), growing as the gather tightens and stopping before the throw.

    Alternates every frame, since anything slower reads as a wobble rather than a vibration, and its two axes
    alternate a beat apart so it never settles onto one line. It dies the moment the boss stops dead: that
    stillness is what the throw lands against, and a rattle running through it would spend it.
    """
    if frame >= clip.gather - STILL:
        return 0.0, 0.0
    reach = clip.shake * charged(frame, clip) ** SHAKE_RAMP
    return (reach if frame % 2 else -reach), (reach * 0.6 if frame % 4 < 2 else -reach * 0.6)


def ring_yaw(frame, clip, ring):
    """Degrees the ring has twisted: back as the boss gathers, through and past as it throws, none at rest.

    Riding the same curve as everything else is what lands it back on rest at both ends without being unwound.
    """
    return -ring.twist * lagged(frame, clip, ring)


def ring_scale(frame, clip, ring):
    """The ring's XY scale: the curve it lags, spread between its gathered and its thrown size."""
    value = lagged(frame, clip, ring)
    return 1.0 + value * ((1.0 - ring.gather_scale) if value < 0.0 else (ring.throw_scale - 1.0))


def spike_reach(frame, clip, ring):
    """A spike's X scale: 0 folded into its face, 1 the full blade.

    Every blade alike, with none of the weighting by direction the ability clips use: the launch commits to no
    direction, so nothing about the boss may point at where the thing is going.

    Capped at the blade the mesh gives it. The throw's overshoot belongs to the rings, which are meant to swell
    past where they land, but a reach past 1 is a stretched blade rather than a further-out one — and a stretched
    one reaches through whatever the ring it hangs off sits inside.
    """
    value = lagged(frame, clip, ring)
    return -value * clip.gather_spike if value < 0.0 else min(1.0, value * clip.throw_spike)


def top_twist(clip):
    """The most any ring turns between two frames -> (degrees, ring)."""
    return max((max(abs(ring_yaw(frame, clip, ring) - ring_yaw(frame - 1, clip, ring))
                    for frame in range(1, total_frames(clip) + 1)), bone)
               for bone, ring in clip.rings.items())


def key(frame, bone, rest_local, clip, layout):
    """The bone's local transform at `frame`, as the track writer wants it.

    Yaw alone stands for the rotation because every bone of this rig is placed with one.
    """
    x, y, z = rest_local.translation.x, rest_local.translation.y, rest_local.translation.z
    yaw = rest_local.rotation.rotator().yaw
    sx, sy, sz = rest_local.scale3d.x, rest_local.scale3d.y, rest_local.scale3d.z
    if bone == ROOT:
        shake_x, shake_y = rattle(frame, clip)
        x, y = x + shake_x, y + shake_y
    elif bone in clip.rings:
        ring = clip.rings[bone]
        yaw += ring_yaw(frame, clip, ring)
        factor = ring_scale(frame, clip, ring)
        sx, sy = sx * factor, sy * factor
    elif bone in layout:
        sx = spike_reach(frame, clip, clip.rings[layout[bone][0]])
    return unreal.Vector(x, y, z), unreal.Rotator(yaw=yaw).quaternion(), unreal.Vector(sx, sy, sz)


def build_sequence(clip, toolkit, layout):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    sequence = toolkit["get_or_create_asset"](ANIM_PACKAGE, "SK_HexBoss_Sequence_{}".format(clip.name),
                                              unreal.AnimSequence, factory)
    toolkit["write_bone_tracks"](sequence, SKELETON_PATH, FPS, total_frames(clip),
                                 lambda frame, bone, rest_local: key(frame, bone, rest_local, clip, layout),
                                 "Build {} launch".format(clip.name))
    return sequence


def build_montage(clip, sequence, toolkit):
    """Two sections and no loop, since neither ability has a phase of its own to hold through.

    Fire1 rather than Fire: the base ability jumps to Start on a fresh activation and to Fire<n> on a repeat, so a
    section named Fire would satisfy the has-a-fire-section test and then never be jumped to.
    """
    names, starts = ["Start", "Fire1"], [0.0, clip.gather / float(FPS)]
    montage = toolkit["build_montage"](sequence, ANIM_PACKAGE, "SK_HexBoss_Montage_{}".format(clip.name),
                                       names, starts, ["Fire1", "None"])
    return montage, names, starts


def report(clip, sequence, montage, names, starts, toolkit, groups, layout):
    """Evaluate the finished sequence — the track list reads a legacy path and reports empty for every one."""
    frames = total_frames(clip)
    options = unreal.AnimPoseEvaluationOptions()
    peak, hush = clip.gather - STILL - 1, clip.gather - STILL
    marks = sorted(set(list(range(0, frames + 1, SAMPLE)) + [peak, hush, clip.gather, throw_end(clip), frames]))
    inner = {CORE: [bone for bone in groups if bone.startswith(CORE)],
             MID: [bone for bone in groups if bone.startswith(MID)]}
    speed, fastest = top_twist(clip)

    # Every frame rather than only the printed rows: a coarse sample steps straight over the two or three frames
    # where the rings cross each other, which is exactly where they would collide.
    poses, measured = {}, {}
    for frame in range(frames + 1):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        poses[frame] = toolkit["local_pose_table"](pose)
        posed = toolkit["_component_transforms"](pose)
        placed = toolkit["place_groups"](groups, posed)
        past = {spike: toolkit["feature_protrusion"](placed, posed, spike, parent)
                for spike, (parent, _) in layout.items()}
        measured[frame] = (past, (toolkit["nested_clearance"](placed, inner[CORE], MID),
                                  toolkit["nested_clearance"](placed, inner[MID], OUTER)))

    LOG.append("")
    LOG.append("=" * 108)
    LOG.append("SK_HexBoss_Sequence_{} — {} frames at {} fps = {:.2f}s, {} sampled keys (expect {})".format(
        clip.name, frames, FPS, frames / float(FPS),
        toolkit["playable_key_count"](sequence), frames + 1))
    LOG.append("sections " + ", ".join("%s f%d (%.2fs)" % (name, round(start * FPS), start)
                                       for name, start in zip(names, starts)))
    LOG.append("Start runs {:.2f}s — set the ability's FireDelay to that and the clip plays at its authored rate"
               .format(clip.gather / float(FPS)))
    LOG.append("spikes reach {:.2f} gathered and {:.2f} thrown, all round; gather rattles {:.0f} units; fastest"
               " twist {:.1f} deg/frame on {} (reads backwards past {:.0f})".format(
                   clip.gather_spike, clip.throw_spike, clip.shake, speed, fastest, STROBE))
    LOG.append("")
    LOG.append("frame  drive  shake   outer          mid            core           blade past face   room inside")
    LOG.append("               x   y  yaw   scale    yaw   scale    yaw   scale    max    out (18)  core   mid")

    for frame in marks:
        local = poses[frame]
        past, gaps = measured[frame]
        LOG.append("%5d %+6.2f %4.0f %3.0f " % ((frame, drive(frame, clip)) + local[ROOT][0][:2])
                   + " ".join("%6.1f %6.3f " % (local[ring][1], local[ring][2][0]) for ring in (OUTER, MID, CORE))
                   + "  %6.1f %5d  %6.1f %6.1f  %s" % (
                       max(past.values()), sum(1 for value in past.values() if value > 0.0), gaps[0], gaps[1],
                       "#" * int(round(max(0.0, max(past.values())) / 6.0))))

    folded = [max(measured[frame][0].values()) for frame in (0, frames)]
    room = min(min(gaps) for _, gaps in measured.values())
    LOG.append("")
    LOG.append("folded at both ends: frame 0 max blade %.1f, frame %d max blade %.1f (negative = buried)" % (
        folded[0], frames, folded[1]))
    LOG.append("rings back at rest: %s" % ", ".join("%s yaw %.4f deg, scale %.5f" % (
        ring, poses[frames][ring][1], poses[frames][ring][2][0]) for ring in clip.rings))
    LOG.append("rattle grows to %.1f units by frame %d, then dead still: %.1f units over the %d frames before the"
               " throw" % (math.hypot(*poses[peak][ROOT][0][:2]), peak,
                           math.hypot(*poses[hush][ROOT][0][:2]), STILL))
    LOG.append("closest the rings ever come: %.1f units%s" % (room, "" if room > 0.0 else "  <-- THEY OVERLAP"))
    LOG.append("montage sections read back: {}".format(toolkit["montage_sections"](montage)))


try:
    # Loading the shared toolkit the same way a script is run, so the silhouette maths lives in one place.
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    layout = toolkit["feature_directions"](toolkit["_component_transforms"](
        APE.get_reference_pose(unreal.load_asset(SKELETON_PATH))), SPIKE_MARK)
    groups = toolkit["rigid_vertex_groups"](MESH_PATH, SKELETON_PATH)

    LOG.append("hex boss launch clips — one beat each, no loop, and no direction anywhere in them")
    LOG.append("both start and end on the reference pose with the spikes folded, so they blend against the idle")
    LOG.append("neither ability is a pattern: these hang off the base ability's own AnimMontage property")
    for clip in CLIPS:
        speed, fastest = top_twist(clip)
        if speed >= STROBE:
            raise RuntimeError("{} twists {} at {:.1f} deg/frame, which reads as going backwards".format(
                clip.name, fastest, speed))
        sequence = build_sequence(clip, toolkit, layout)
        montage, names, starts = build_montage(clip, sequence, toolkit)
        report(clip, sequence, montage, names, starts, toolkit, groups, layout)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
