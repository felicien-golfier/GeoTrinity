"""Hex boss ability animations: the sweep beam, the tile-carving ray and the cone spray.

One clip per ability, each cut into the four montage sections the pattern machinery drives: Start winds up and is
stretched to the ability's FireDelay, Fire is the burst on the frame the pattern goes live, FireLoop holds the
deployed boss for as long as the pattern runs, and End recoils back. FireLoop is a separate section because the
live phase has no fixed length; UPattern accepts any section whose name contains "Fire", which is what lets it
loop on itself until EndPattern jumps to End.

Every clip starts and ends on the reference pose with every spike folded away, so it blends both ways against the
idle. The rings are the exception and deliberately so: each turns a whole number of sixths of a turn, which maps
a hexagon onto itself, so a ring is visually at rest on the last frame however far it actually travelled and
nothing ever has to unwind a spin backwards. The same identity closes FireLoop on its own first frame.

Everything reads one curve a fixed number of frames late — the core leads, the outer ring drags — so the boss
never moves as a single block. Negative while it winds up, positive while it is deployed, zero at rest.

The wind-up rattles: a whole-boss vibration that alternates every frame and grows as the charge tightens, then
cuts out for the last frames before the hit. That silence is what the hit lands against, so the rattle stops
short of it rather than running into it.

The three read apart by how they use the spikes. A focus of zero extends every spike alike; a sharp focus
extends only the ones pointing where the boss is aimed, and it reads a spike's angle from where it points now
rather than where it rests, so a turning ring pushes blades out as they sweep through the front and folds them
as they leave. That is what makes the carving ray grow a barrel along its beam while the sweep bristles all
round.

Nothing moves along Z and nothing scales on it: under an orthographic camera that motion is spent for nothing.

Run via mcp-unreal execute_script. Re-runnable: rewrites every sequence and montage in place.
Report written to REPORT.
"""
import collections
import math

import unreal

APE = unreal.AnimPoseExtensions
MATH = unreal.MathLibrary

SKELETON_PATH = "/Game/Characters/Meshes/HexBoss/SK_HexBoss"
MESH_PATH = "/Game/Characters/Meshes/HexBoss/SKM_HexBoss"
ANIM_PACKAGE = "/Game/Characters/Anim/HexBoss"
REPORT = ("C:/Users/Felou/AppData/Local/Temp/claude/C--GeoTrinity/"
          "0b3b8214-c633-4920-a4b1-c68d4e9de203/scratchpad/hex_boss_abilities.txt")

ROOT = "Root"
OUTER, MID, CORE = "HexOuter", "HexMid", "HexCore"
SPIKE_MARK = "_Spike_"

FPS = 30
SAMPLE = 6          # frames between report rows

CHARGE_EASE = 2.4   # above 1 the wind-up accelerates instead of turning at a fixed speed
STILL = 5           # frames stopped dead at the tightest point of the wind-up
BURST = [0.45, 1.15, 1.0]  # the mid-flight frame, the overshoot that lands the hit, then full extension
SETTLE = 2          # frames after the overshoot before the loop takes over
DAMP = 2.0          # how fast the recoil dies
SPRING = 4.8        # how far past rest it swings on the way
SHAKE_RAMP = 2.0    # above 1 the rattle keeps out of the way until the wind-up is nearly tight

# (frames this ring lags the curve, its scale wound tightest, its scale deployed, sixths of a turn it makes
#  winding up, sixths it makes per loop, how much it pulses over the loop)
Ring = collections.namedtuple("Ring", "lag charge_scale burst_scale charge_sixths loop_sixths loop_pulse")

# (asset suffix, wind-up frames, loop frames, recoil frames, how sharply the spikes favour the front,
#  spike reach wound tightest, spike reach deployed, units the wind-up rattles at its tightest, the rings)
Clip = collections.namedtuple("Clip", "name start loop end focus charge_spike burst_spike shake rings")

CLIPS = [
    # Sweeping laser: the boss winds all three rings up, snaps open and turns into a spinning emitter, teeth out
    # all the way round because the beam covers the whole arc.
    Clip("SweepBeam", 54, 36, 20, 0.0, 0.30, 0.95, 5.0, {
        OUTER: Ring(3, 0.86, 1.14, 2, 1, 0.000),
        MID: Ring(2, 0.82, 1.06, -3, -2, 0.000),
        CORE: Ring(0, 1.22, 1.05, 6, 3, 0.000)}),
    # Tile-carving ray: direction locks at activation, so the boss stops dead instead of turning — the tell is the
    # idle's constant motion giving way to a rattle, the hardest of the three since nothing else is moving —
    # clamps down hard and grows a barrel of spikes along the beam, then throbs while it grinds.
    Clip("CarvingRay", 48, 10, 24, 8.0, 0.45, 1.00, 7.0, {
        OUTER: Ring(3, 0.80, 1.16, 0, 0, 0.020),
        MID: Ring(2, 0.84, 1.12, 0, 0, 0.028),
        CORE: Ring(0, 1.24, 1.10, 0, 0, 0.040)}),
    # Cone spray: salvos pumped out of the front. The rings pulse once per loop so a loop reads as one salve,
    # and the core keeps turning so its blades ripple through the cone rather than sitting in it.
    Clip("ConeSpray", 36, 15, 18, 3.0, 0.25, 0.85, 4.0, {
        OUTER: Ring(3, 0.92, 1.08, 0, 0, 0.045),
        MID: Ring(2, 0.88, 1.12, -1, 0, 0.060),
        CORE: Ring(0, 1.22, 1.18, 2, 1, 0.080)}),
]

MAX_LAG = max(ring.lag for clip in CLIPS for ring in clip.rings.values())
BURST_FRAMES = len(BURST) + MAX_LAG + SETTLE  # long enough that every ring has settled before the loop starts

LOG = []


def burst_end(clip):
    return clip.start + BURST_FRAMES


def loop_end(clip):
    return burst_end(clip) + clip.loop


def total_frames(clip):
    """The recoil is given the longest lag on top, so even the ring that drags most reaches rest by the last key."""
    return loop_end(clip) + clip.end + MAX_LAG


def snapshot(pose):
    """Every bone's local transform as plain numbers -> {bone: ((x, y, z), yaw, (sx, sy, sz))}.

    Copied out rather than held as transforms: every evaluated pose reuses one buffer. Yaw alone stands for the
    rotation because every bone of this rig is placed with one.
    """
    table = {}
    for bone in APE.get_bone_names(pose):
        local = APE.get_bone_pose(pose, str(bone), unreal.AnimPoseSpaces.LOCAL)
        table[str(bone)] = ((local.translation.x, local.translation.y, local.translation.z),
                            local.rotation.rotator().yaw,
                            (local.scale3d.x, local.scale3d.y, local.scale3d.z))
    return table


def spike_layout(placed):
    """Spike bone -> (the ring it hangs off, the direction it rests pointing in degrees).

    Read from where each bone sits rather than from a list, so it follows the rig instead of restating it.
    """
    layout = {}
    for bone, transform in placed.items():
        if SPIKE_MARK in bone:
            position = transform.translation
            layout[bone] = (bone.split(SPIKE_MARK)[0], math.degrees(math.atan2(position.y, position.x)))
    return layout


def charged(frame, clip):
    """0 to 1 across the wind-up, accelerating; 1 once the boss has stopped dead."""
    return min(1.0, max(0.0, frame) / float(clip.start - STILL - 1)) ** CHARGE_EASE


def drive(frame, clip):
    """-1 wound tightest, +1 fully deployed, 0 at rest — the one curve every part follows."""
    wind = clip.start - STILL
    if frame < wind:
        return -charged(frame, clip)
    if frame < clip.start:
        return -1.0
    if frame < clip.start + len(BURST):
        return BURST[frame - clip.start]
    # Through loop_end and not up to it: that frame is the loop's last, and it has to match its first.
    if frame <= loop_end(clip):
        return BURST[-1]
    alpha = (frame - loop_end(clip)) / float(clip.end)
    return 0.0 if alpha >= 1.0 else math.exp(-DAMP * alpha) * math.cos(SPRING * alpha) * (1.0 - alpha)


def rattle(frame, clip):
    """The whole boss's offset this frame -> (x, y), growing as the wind-up tightens and stopping before the hit.

    Alternates every frame, since anything slower reads as a wobble rather than a vibration, and its two axes
    alternate a beat apart so it never settles onto one line. It dies the moment the boss stops dead: that
    stillness is what the hit lands against, and a rattle running through it would spend it.
    """
    if frame >= clip.start - STILL:
        return 0.0, 0.0
    reach = clip.shake * charged(frame, clip) ** SHAKE_RAMP
    return (reach if frame % 2 else -reach), (reach * 0.6 if frame % 4 < 2 else -reach * 0.6)


def lagged(frame, clip, ring):
    """The curve this ring reads, delayed by its own weight — but the delay reverses while the boss opens.

    Light parts leading and heavy ones dragging is what keeps the boss from moving as one block, and closing is
    the direction that wants. Opening, it is the wrong way round: an inner ring swelling into a shell that has not
    opened yet goes straight through it. So from the burst until the pattern stops the order flips, and the outer
    ring is first open and last shut. The plateau either side of the loop outlasts the largest delay, so the flip
    lands on the same value at both ends of it and no ring jumps.
    """
    delay = MAX_LAG - ring.lag if clip.start <= frame <= loop_end(clip) else ring.lag
    return drive(frame - delay, clip)


def ring_yaw(frame, clip, ring):
    """Degrees the ring has turned by `frame`, kept rather than unwound.

    Both the wind-up spin and the loop's turn are whole sixths, so every one of them leaves the hexagon looking
    exactly as it did — which is what lets the clip end mid-spin and the loop close on its own first frame.
    """
    yaw = 60.0 * ring.charge_sixths * charged(frame, clip)
    if frame > burst_end(clip):
        yaw += 60.0 * ring.loop_sixths * (min(frame, loop_end(clip)) - burst_end(clip)) / float(clip.loop)
    return yaw


def ring_scale(frame, clip, ring):
    """The ring's XY scale: the curve it lags, spread between its wound and its deployed size, plus the loop pulse."""
    value = lagged(frame, clip, ring)
    scale = 1.0 + value * ((1.0 - ring.charge_scale) if value < 0.0 else (ring.burst_scale - 1.0))
    if burst_end(clip) <= frame <= loop_end(clip):
        scale *= 1.0 + ring.loop_pulse * math.sin(2.0 * math.pi * (frame - burst_end(clip)) / float(clip.loop))
    return scale


def spike_reach(frame, clip, ring, angle):
    """A spike's X scale: 0 folded into its face, 1 the full blade.

    Weighted by where the spike points at this frame rather than where it rests, so a turning ring pushes its
    blades out as they sweep through the boss's front and folds them again as they leave it. A focus of zero
    flattens that to every spike alike.

    Capped at the blade the mesh gives it. The burst's overshoot belongs to the rings, which are meant to swell
    past where they land, but a reach past 1 is a stretched blade rather than a further-out one.
    """
    value = lagged(frame, clip, ring)
    reach = -value * clip.charge_spike if value < 0.0 else min(1.0, value * clip.burst_spike)
    return reach * max(0.0, math.cos(math.radians(angle))) ** clip.focus


def key(frame, bone, clip, rest, layout):
    """The bone's local transform at `frame` -> (translation, yaw, scale)."""
    (x, y, z), yaw, (sx, sy, sz) = rest[bone]
    if bone == ROOT:
        shake_x, shake_y = rattle(frame, clip)
        x, y = x + shake_x, y + shake_y
    elif bone in clip.rings:
        ring = clip.rings[bone]
        yaw += ring_yaw(frame, clip, ring)
        factor = ring_scale(frame, clip, ring)
        sx, sy = sx * factor, sy * factor
    elif bone in layout:
        parent, angle = layout[bone]
        ring = clip.rings[parent]
        sx = spike_reach(frame, clip, ring, angle + ring_yaw(frame, clip, ring))
    return (x, y, z), yaw, (sx, sy, sz)


def get_or_create(asset_name, asset_class, factory):
    """Load the asset if it exists, else create it.

    Loaded rather than looked up in the asset registry: a registry still scanning reports an asset that is on disk
    as missing, and creating over it then fails and hands back nothing. Never deletes: deleting a loaded asset
    leaves the package unloadable for the rest of the editor session.
    """
    asset = unreal.load_asset("{}/{}".format(ANIM_PACKAGE, asset_name))
    return asset or unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, ANIM_PACKAGE, asset_class, factory)


def build_sequence(clip, rest, layout):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    name = "SK_HexBoss_Sequence_{}".format(clip.name)
    sequence = get_or_create(name, unreal.AnimSequence, factory)
    frames = total_frames(clip)

    controller = sequence.get_editor_property("controller")
    controller.open_bracket("Build {}".format(clip.name))
    controller.set_frame_rate(unreal.FrameRate(FPS, 1))
    controller.set_number_of_frames(unreal.FrameNumber(frames))
    for bone in rest:
        positions, rotations, scales = [], [], []
        for frame in range(frames + 1):  # a sequence holds one more key than its frame count
            translation, yaw, scale = key(frame, bone, clip, rest, layout)
            positions.append(unreal.Vector(*translation))
            rotations.append(unreal.Rotator(yaw=yaw).quaternion())
            scales.append(unreal.Vector(*scale))
        # Unconditional and result ignored: the adder reports failure for an existing track, and the key setter
        # reports success whether or not a track is there.
        controller.add_bone_curve(bone)
        controller.set_bone_track_keys(bone, positions, rotations, scales)
    controller.close_bracket()
    # The keys above land in the raw model; this builds the data that actually plays back from them.
    unreal.AnimationLibrary.finalize_bone_animation(sequence)
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(ANIM_PACKAGE, name))
    return sequence


def build_montage(clip, sequence):
    """Cut the clip into the sections the pattern drives, FireLoop holding until EndPattern jumps to End.

    The factory builds the slot track and its segment itself when handed a source animation. Sections have no
    scripting path, so they go through the C++ editor shim.
    """
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("target_skeleton", sequence.get_skeleton())
    factory.set_editor_property("source_animation", sequence)
    name = "SK_HexBoss_Montage_{}".format(clip.name)
    montage = get_or_create(name, unreal.AnimMontage, factory)

    names = ["Start", "Fire", "FireLoop", "End"]
    starts = [0.0, clip.start / float(FPS), burst_end(clip) / float(FPS), loop_end(clip) / float(FPS)]
    following = ["Fire", "FireLoop", "FireLoop", "None"]

    util = unreal.get_default_object(unreal.GeoAnimBuilderUtil)
    util.set_montage_slot_segment(montage, sequence, "DefaultSlot")
    util.set_montage_sections(montage, [unreal.Name(n) for n in names], starts,
                              [unreal.Name(n) for n in following])
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(ANIM_PACKAGE, name))
    return montage, names, starts


def protrusion(sequence, toolkit, groups, layout, frames):
    """What the boss actually looks like, per frame -> {frame: ({spike: units past its face}, (gap, gap))}.

    The gaps are how much room the core has inside the middle ring and the middle ring inside the outer one.
    Both measurements come from the shared toolkit, which is also what the clockwork cut reads them with.
    """
    options = unreal.AnimPoseEvaluationOptions()
    inner = {CORE: [bone for bone in groups if bone.startswith(CORE)],
             MID: [bone for bone in groups if bone.startswith(MID)]}
    reach = {}
    for frame in frames:
        posed = toolkit["_component_transforms"](
            APE.get_anim_pose_at_time(sequence, frame / float(FPS), options))
        placed = toolkit["place_groups"](groups, posed)
        past = {spike: toolkit["feature_protrusion"](placed, posed, spike, parent)
                for spike, (parent, _) in layout.items()}
        reach[frame] = (past, (toolkit["nested_clearance"](placed, inner[CORE], MID),
                               toolkit["nested_clearance"](placed, inner[MID], OUTER)))
    return reach


def report(clip, sequence, montage_names, montage_starts, toolkit, groups, layout):
    frames = total_frames(clip)
    peak, hush = clip.start - STILL - 1, clip.start - STILL
    marks = sorted(set(list(range(0, frames + 1, SAMPLE))
                       + [peak, hush, clip.start, burst_end(clip), loop_end(clip), frames]))
    # Every frame rather than only the printed rows: a coarse sample steps straight over the two or three frames
    # where the rings cross each other, which is exactly where they would collide.
    reach = protrusion(sequence, toolkit, groups, layout, range(frames + 1))
    options = unreal.AnimPoseEvaluationOptions()
    spin = 60.0 * max(abs(ring.charge_sixths) for ring in clip.rings.values()) \
        * (charged(clip.start - STILL - 1, clip) - charged(clip.start - STILL - 2, clip))

    LOG.append("")
    LOG.append("=" * 108)
    LOG.append("SK_HexBoss_Sequence_{} — {} frames at {} fps = {:.2f}s, {} sampled keys (expect {})".format(
        clip.name, frames, FPS, frames / float(FPS),
        sequence.get_editor_property("number_of_sampled_keys"), frames + 1))
    LOG.append("sections " + ", ".join("%s f%d (%.2fs)" % (name, round(start * FPS), start)
                                       for name, start in zip(montage_names, montage_starts)))
    LOG.append("focus {:.0f}, spikes reach {:.2f} wound and {:.2f} deployed, wind-up rattles {:.0f} units;"
               " fastest wind-up spin {:.1f} deg/frame (strobes past 30)".format(
                   clip.focus, clip.charge_spike, clip.burst_spike, clip.shake, spin))
    LOG.append("")
    LOG.append("frame  drive  shake   outer          mid            core           blade past face   room inside")
    LOG.append("               x   y  yaw   scale    yaw   scale    yaw   scale    max    out (18)  core   mid")

    poses = {}
    for frame in marks:
        local = poses[frame] = snapshot(APE.get_anim_pose_at_time(sequence, frame / float(FPS), options))
        past, gaps = reach[frame]
        LOG.append("%5d %+6.2f %4.0f %3.0f " % ((frame, drive(frame, clip)) + local[ROOT][0][:2])
                   + " ".join("%6.1f %6.3f " % (local[ring][1], local[ring][2][0]) for ring in (OUTER, MID, CORE))
                   + "  %6.1f %5d  %6.1f %6.1f  %s" % (
                       max(past.values()), sum(1 for value in past.values() if value > 0.0), gaps[0], gaps[1],
                       "#" * int(round(max(0.0, max(past.values())) / 6.0))))

    folded = [max(reach[frame][0].values()) for frame in (0, frames)]
    closure = [sorted(reach[burst_end(clip)][0].values()), sorted(reach[loop_end(clip)][0].values())]
    rings = [poses[burst_end(clip)], poses[loop_end(clip)]]
    room = min(min(gaps) for _, gaps in reach.values())
    LOG.append("")
    LOG.append("folded at both ends: frame 0 max blade %.1f, frame %d max blade %.1f (negative = buried)" % (
        folded[0], frames, folded[1]))
    LOG.append("rattle grows to %.1f units by frame %d, then dead still: %.1f units over the %d frames before the"
               " hit" % (math.hypot(*poses[peak][ROOT][0][:2]), peak,
                         math.hypot(*poses[hush][ROOT][0][:2]), STILL))
    LOG.append("FireLoop closes: silhouette gap %.4f units, ring scale gap %.5f, ring yaw gap %.4f deg (mod 60)" % (
        max(abs(a - b) for a, b in zip(*closure)),
        max(abs(rings[0][ring][2][0] - rings[1][ring][2][0]) for ring in clip.rings),
        max(abs((rings[0][ring][1] - rings[1][ring][1] + 30.0) % 60.0 - 30.0) for ring in clip.rings)))
    LOG.append("closest the rings ever come: %.1f units%s" % (room, "" if room > 0.0 else "  <-- THEY OVERLAP"))


try:
    # Loading the shared toolkit the same way a script is run, so the silhouette maths lives in one place.
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    rest = snapshot(APE.get_reference_pose(unreal.load_asset(SKELETON_PATH)))
    layout = spike_layout(toolkit["_component_transforms"](
        APE.get_reference_pose(unreal.load_asset(SKELETON_PATH))))
    groups = toolkit["rigid_vertex_groups"](MESH_PATH, SKELETON_PATH)
    missing = [bone for clip in CLIPS for bone in clip.rings if bone not in rest]
    if missing or not layout:
        raise RuntimeError("skeleton is missing rings {} or has no {} bones".format(missing, SPIKE_MARK))

    LOG.append("hex boss ability clips — every one starts and ends on the reference pose with the spikes folded")
    LOG.append("montage sections: Start stretches to the ability's FireDelay, Fire is the burst, FireLoop holds")
    LOG.append("until EndPattern jumps to End")
    for clip in CLIPS:
        sequence = build_sequence(clip, rest, layout)
        montage, names, starts = build_montage(clip, sequence)
        report(clip, sequence, names, starts, toolkit, groups, layout)
        LOG.append("montage sections read back: {}".format(
            [str(montage.get_section_name(i)) for i in range(montage.get_num_sections())]))
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
