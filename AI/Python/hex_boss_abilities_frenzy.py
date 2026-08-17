"""Hex boss ability animations, frenzy cut: the same three abilities wound up to the edge of what the eye can
still read as turning.

Everything here is smooth and nothing is ever still. One curve ramps the spin in, holds it flat while the pattern
runs and bleeds it off again, and the shake and the wobble both read that same curve — so the boss rattles hardest
exactly when it spins fastest, and all three die together.

How fast it can go is bounded by the mesh, not by taste: a sixth of a turn maps a hexagon onto itself, so a ring
crossing half a sixth between two frames reads as likely to be turning the other way, and past a whole sixth it
reads as standing still. The rings run just under that, which is as fast as this shape can be spun and still be
seen to spin.

The shake is a whole-boss rattle that alternates every frame, because anything slower reads as a wobble rather
than a vibration. On top of it each ring's centre orbits at its own rate, so the three drift against each other
and the boss looks like it is coming apart.

Spin is never unwound. Each ring's travel is stretched over the spin-down to land on whole sixths, so the clip
finishes on an orientation the eye cannot tell from rest however many turns it made. The ratchet's travel is
whole sixths on its own, which is what closes the loop.

Every clip starts and ends on the reference pose with the spikes folded, so all three cuts blend against the idle
and against each other. Nothing moves along Z: under an orthographic camera that motion is spent for nothing.

Run via mcp-unreal execute_script. Re-runnable: rewrites every sequence and montage in place.
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
          "0b3b8214-c633-4920-a4b1-c68d4e9de203/scratchpad/hex_boss_frenzy.txt")

ROOT = "Root"
OUTER, MID, CORE = "HexOuter", "HexMid", "HexCore"
SPIKE_MARK = "_Spike_"
SYMMETRY = 60.0     # degrees that map a hexagon onto itself
STROBE = SYMMETRY / 2.0  # degrees between frames past which a ring reads as turning the other way

FPS = 30
SAMPLE = 6          # frames between report rows

SPIN_EASE = 1.6     # above 1 the spin keeps gaining rather than settling into its speed early
THROW = 6           # frames the wind-up takes to cross into full extension
SETTLE = 3          # frames of full extension before the loop, at least the largest delay
PUMP_CYCLES = 4     # times the spikes pulse over one loop

# (frames this ring lags the curve, its scale wound tightest, its scale deployed, sixths of a turn per loop,
#  units its centre orbits, turns of that orbit per loop)
Ring = collections.namedtuple("Ring", "lag charge_scale burst_scale loop_sixths wobble wobble_turns")

# (asset suffix, wind-up frames, loop frames, spin-down frames, how sharply the spikes favour the front, spike
#  reach wound tightest, spike reach deployed, units the whole boss rattles, how hard the spikes pulse, the rings)
Clip = collections.namedtuple(
    "Clip", "name start loop end focus charge_spike burst_spike shake pump rings")

CLIPS = [
    # Sweeping laser: all three rings screaming at once in opposite directions, every blade out, the whole boss
    # rattling — the beam covers the entire arc so nothing needs aiming.
    Clip("SweepBeam", 48, 24, 30, 0.0, 0.35, 0.95, 15.0, 0.15, {
        OUTER: Ring(3, 0.86, 1.18, 5, 7.0, 1),
        MID: Ring(2, 0.86, 1.02, -7, 10.0, 2),
        CORE: Ring(0, 1.14, 1.05, 9, 13.0, 3)}),
    # Tile-carving ray: direction locks at activation, so the outer rings have to hold their aim while the core
    # tears itself apart inside them. The hardest rattle of the three — it reads as the boss straining to keep
    # the lance pointed while everything else wants to spin.
    Clip("CarvingRay", 44, 24, 28, 8.0, 0.45, 1.00, 21.0, 0.10, {
        OUTER: Ring(3, 0.84, 1.20, 0, 5.0, 1),
        MID: Ring(2, 0.88, 1.06, 0, 7.0, 2),
        CORE: Ring(0, 1.14, 1.10, 9, 14.0, 4)}),
    # Cone spray: the shortest and busiest, its spikes pumping hardest so each pulse reads as a salve leaving.
    Clip("ConeSpray", 34, 20, 24, 3.0, 0.25, 0.85, 13.0, 0.30, {
        OUTER: Ring(3, 0.92, 1.14, 2, 7.0, 1),
        MID: Ring(2, 0.90, 1.06, -4, 10.0, 2),
        CORE: Ring(0, 1.14, 1.14, 8, 12.0, 3)}),
]

MAX_LAG = max(ring.lag for clip in CLIPS for ring in clip.rings.values())

LOG = []


def burst_end(clip):
    return clip.start + THROW + SETTLE


def loop_end(clip):
    return burst_end(clip) + clip.loop


def total_frames(clip):
    """The spin-down is given the longest delay on top, so even the ring that drags most reaches rest by the
    last key."""
    return loop_end(clip) + clip.end + MAX_LAG


def smoothstep(alpha):
    alpha = min(1.0, max(0.0, alpha))
    return alpha * alpha * (3.0 - 2.0 * alpha)


def spin_profile(frame, clip):
    """0 stopped, 1 at full speed — the one curve the spin, the rattle and the wobble all read.

    Ramps in across the wind-up and the throw, holds flat for as long as the pattern runs, and bleeds off over
    the spin-down, so no frame of the clip is ever still and everything slows together.
    """
    if frame < burst_end(clip):
        return smoothstep(max(0.0, frame) / float(burst_end(clip))) ** SPIN_EASE
    if frame <= loop_end(clip):
        return 1.0
    return 1.0 - smoothstep((frame - loop_end(clip)) / float(clip.end))


def yaw_table(clip, ring):
    """Cumulative degrees turned, per frame.

    The loop's own travel is whole sixths by construction, which closes it. The spin-down's travel is then
    stretched to land the clip's total on whole sixths too, so the ring stops on an orientation the eye cannot
    tell from rest and no rotation is ever run backwards to get there.
    """
    frames = total_frames(clip)
    peak = SYMMETRY * ring.loop_sixths / float(clip.loop)
    rates = [spin_profile(frame, clip) * peak for frame in range(frames + 1)]

    running, table = 0.0, [0.0]
    for frame in range(1, frames + 1):
        running += rates[frame]
        table.append(running)

    coasting = table[loop_end(clip)]
    spun_down = table[frames] - coasting
    if spun_down:
        step = SYMMETRY if ring.loop_sixths > 0 else -SYMMETRY
        stretch = (math.ceil(table[frames] / step) * step - coasting) / spun_down
        for frame in range(loop_end(clip) + 1, frames + 1):
            table[frame] = coasting + (table[frame] - coasting) * stretch
    return table


def drive(frame, clip):
    """-1 wound tightest, +1 fully deployed, 0 at rest — smooth end to end, with nothing held still.

    The wind-up runs straight into the throw rather than stopping dead before it, and the throw bleeds away
    instead of springing past rest, so the clip reads as one continuous surge rather than as a hit.
    """
    if frame < clip.start:
        return -smoothstep(max(0.0, frame) / float(clip.start)) ** 1.4
    if frame < clip.start + THROW:
        return -1.0 + 2.0 * smoothstep((frame - clip.start) / float(THROW))
    if frame <= loop_end(clip):
        return 1.0
    return 1.0 - smoothstep((frame - loop_end(clip)) / float(clip.end))


def rattle(frame, clip):
    """The whole boss's offset this frame -> (x, y).

    Alternates every frame, since anything slower reads as a wobble rather than a vibration, and its two axes
    alternate a beat apart so the rattle never settles onto one line. Four frames long, so it closes any loop of
    an even number of beats.
    """
    reach = clip.shake * spin_profile(frame, clip)
    return (reach if frame % 2 else -reach), (reach * 0.6 if frame % 4 < 2 else -reach * 0.6)


def orbit(frame, clip, ring):
    """The ring centre's drift this frame -> (x, y), a whole number of turns per loop so it closes."""
    reach = ring.wobble * spin_profile(frame, clip)
    angle = 2.0 * math.pi * ring.wobble_turns * frame / float(clip.loop)
    return reach * math.cos(angle), reach * math.sin(angle)


def lagged(frame, clip, ring):
    """The curve this ring reads, delayed by its own weight — but the delay reverses while the boss opens.

    Light parts leading and heavy ones dragging is what keeps the boss from moving as one block, and everywhere
    it shrinks that is the order it wants. Growing, it is the wrong way round: nested rings would swell into a
    shell that has not opened yet. So through the throw and the run the order flips, and the outer ring is first
    open and last shut.
    """
    delay = MAX_LAG - ring.lag if clip.start <= frame <= loop_end(clip) else ring.lag
    return drive(frame - delay, clip)


def ring_scale(frame, clip, ring):
    value = lagged(frame, clip, ring)
    return 1.0 + value * ((1.0 - ring.charge_scale) if value < 0.0 else (ring.burst_scale - 1.0))


def spike_reach(frame, clip, ring, angle):
    """A spike's X scale: 0 folded into its face, 1 the full blade.

    Weighted by where the spike points at this frame rather than where it rests, so a spinning ring throws its
    blades out as they sweep through the front. While the pattern runs the blades stab back in and out on top of
    that; the pulse only ever shortens them, so none is driven past the length the mesh gives it.
    """
    value = lagged(frame, clip, ring)
    reach = -value * clip.charge_spike if value < 0.0 else value * clip.burst_spike
    if burst_end(clip) <= frame <= loop_end(clip):
        reach *= 1.0 - clip.pump * (0.5 - 0.5 * math.cos(
            2.0 * math.pi * PUMP_CYCLES * (frame - burst_end(clip)) / float(clip.loop)))
    return reach * max(0.0, math.cos(math.radians(angle))) ** clip.focus


def key(frame, bone, clip, rest, layout, yaws):
    """The bone's local transform at `frame` -> (translation, yaw, scale)."""
    (x, y, z), yaw, (sx, sy, sz) = rest[bone]
    if bone == ROOT:
        shake_x, shake_y = rattle(frame, clip)
        x, y = x + shake_x, y + shake_y
    elif bone in clip.rings:
        ring = clip.rings[bone]
        yaw += yaws[bone][frame]
        drift_x, drift_y = orbit(frame, clip, ring)
        x, y = x + drift_x, y + drift_y
        factor = ring_scale(frame, clip, ring)
        sx, sy = sx * factor, sy * factor
    elif bone in layout:
        parent, angle = layout[bone]
        sx = spike_reach(frame, clip, clip.rings[parent], angle + yaws[parent][frame])
    return (x, y, z), yaw, (sx, sy, sz)


def top_speed(clip, yaws):
    """The most any ring turns between two frames, in degrees -> (degrees, ring)."""
    return max((max(abs(table[frame] - table[frame - 1]) for frame in range(1, len(table))), bone)
               for bone, table in yaws.items())


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


def get_or_create(asset_name, asset_class, factory):
    """Load the asset if it exists, else create it.

    Loaded rather than looked up in the asset registry: a registry still scanning reports an asset that is on disk
    as missing, and creating over it then fails and hands back nothing. Never deletes: deleting a loaded asset
    leaves the package unloadable for the rest of the editor session.
    """
    asset = unreal.load_asset("{}/{}".format(ANIM_PACKAGE, asset_name))
    return asset or unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, ANIM_PACKAGE, asset_class, factory)


def build_sequence(clip, rest, layout, yaws):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    name = "SK_HexBoss_Sequence_{}_Frenzy".format(clip.name)
    sequence = get_or_create(name, unreal.AnimSequence, factory)
    frames = total_frames(clip)

    controller = sequence.get_editor_property("controller")
    controller.open_bracket("Build {} frenzy".format(clip.name))
    controller.set_frame_rate(unreal.FrameRate(FPS, 1))
    controller.set_number_of_frames(unreal.FrameNumber(frames))
    for bone in rest:
        positions, rotations, scales = [], [], []
        for frame in range(frames + 1):  # a sequence holds one more key than its frame count
            translation, yaw, scale = key(frame, bone, clip, rest, layout, yaws)
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
    name = "SK_HexBoss_Montage_{}_Frenzy".format(clip.name)
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


def report(clip, sequence, toolkit, groups, layout, yaws, montage_names, montage_starts):
    """Evaluate the finished sequence — the track list reads a legacy path and reports empty for every one."""
    frames = total_frames(clip)
    options = unreal.AnimPoseEvaluationOptions()
    marks = sorted(set(list(range(0, frames + 1, SAMPLE))
                       + [clip.start, burst_end(clip), loop_end(clip), frames]))
    inner = {CORE: [b for b in groups if b.startswith(CORE)], MID: [b for b in groups if b.startswith(MID)]}
    speed, fastest = top_speed(clip, yaws)

    measured = {}
    for frame in marks:
        posed = toolkit["_component_transforms"](
            APE.get_anim_pose_at_time(sequence, frame / float(FPS), options))
        placed = toolkit["place_groups"](groups, posed)
        past = {spike: toolkit["feature_protrusion"](placed, posed, spike, parent)
                for spike, (parent, _) in layout.items()}
        measured[frame] = (past, (toolkit["nested_clearance"](placed, inner[CORE], MID),
                                  toolkit["nested_clearance"](placed, inner[MID], OUTER)))

    LOG.append("")
    LOG.append("=" * 112)
    LOG.append("SK_HexBoss_Sequence_{}_Frenzy — {} frames at {} fps = {:.2f}s, {} sampled keys (expect {})".format(
        clip.name, frames, FPS, frames / float(FPS),
        sequence.get_editor_property("number_of_sampled_keys"), frames + 1))
    LOG.append("sections " + ", ".join("%s f%d (%.2fs)" % (name, round(start * FPS), start)
                                       for name, start in zip(montage_names, montage_starts)))
    LOG.append("top speed {:.1f} deg/frame on {} = {:.0f} deg/s = {:.2f} turns/s (reads backwards past {:.0f})"
               .format(speed, fastest, speed * FPS, speed * FPS / 360.0, STROBE))
    LOG.append("rattle {:.0f} units, spikes pulse {:.0f}% {} times per loop, focus {:.0f}".format(
        clip.shake, clip.pump * 100.0, PUMP_CYCLES, clip.focus))
    LOG.append("")
    LOG.append("frame  shake   outer          mid            core           blade past face   room inside")
    LOG.append("        x   y  yaw   scale    yaw   scale    yaw   scale    max    out (18)  core   mid")

    for frame in marks:
        local = snapshot(APE.get_anim_pose_at_time(sequence, frame / float(FPS), options))
        past, gaps = measured[frame]
        LOG.append("%5d %4.0f %3.0f " % ((frame,) + local[ROOT][0][:2])
                   + " ".join("%6.1f %6.3f " % (local[ring][1], local[ring][2][0]) for ring in (OUTER, MID, CORE))
                   + "  %6.1f %5d  %6.1f %6.1f  %s" % (
                       max(past.values()), sum(1 for value in past.values() if value > 0.0), gaps[0], gaps[1],
                       "#" * int(round(max(0.0, max(past.values())) / 6.0))))

    folded = [max(measured[frame][0].values()) for frame in (0, frames)]
    closure = [sorted(measured[burst_end(clip)][0].values()), sorted(measured[loop_end(clip)][0].values())]
    poses = [snapshot(APE.get_anim_pose_at_time(sequence, frame / float(FPS), options))
             for frame in (burst_end(clip), loop_end(clip))]
    room = min(min(gaps) for _, gaps in measured.values())
    LOG.append("")
    LOG.append("folded at both ends: frame 0 max blade %.1f, frame %d max blade %.1f (negative = buried)" % (
        folded[0], frames, folded[1]))
    LOG.append("clip ends on whole sixths: " + ", ".join("%s %.4f deg off" % (
        ring, abs((yaws[ring][frames] + SYMMETRY / 2.0) % SYMMETRY - SYMMETRY / 2.0)) for ring in clip.rings))
    LOG.append("FireLoop closes: silhouette gap %.4f units, ring scale gap %.5f, ring yaw gap %.4f deg (mod 60)" % (
        max(abs(a - b) for a, b in zip(*closure)),
        max(abs(poses[0][ring][2][0] - poses[1][ring][2][0]) for ring in clip.rings),
        max(abs((poses[0][ring][1] - poses[1][ring][1] + SYMMETRY / 2.0) % SYMMETRY - SYMMETRY / 2.0)
            for ring in clip.rings)))
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

    LOG.append("hex boss ability clips, frenzy cut — smooth throughout, spun as fast as the shape can be read")
    LOG.append("every clip starts and ends on the reference pose with the spikes folded, so it blends against")
    LOG.append("the idle and against the other cuts")
    for clip in CLIPS:
        if clip.loop % 4:
            raise RuntimeError("{} loop of {} frames does not close the four-frame rattle".format(
                clip.name, clip.loop))
        yaws = {bone: yaw_table(clip, ring) for bone, ring in clip.rings.items()}
        speed, fastest = top_speed(clip, yaws)
        if speed >= STROBE:
            raise RuntimeError("{} turns {} at {:.1f} deg/frame, which reads as going backwards".format(
                clip.name, fastest, speed))
        sequence = build_sequence(clip, rest, layout, yaws)
        montage, names, starts = build_montage(clip, sequence)
        report(clip, sequence, toolkit, groups, layout, yaws, names, starts)
        LOG.append("montage sections read back: {}".format(
            [str(montage.get_section_name(i)) for i in range(montage.get_num_sections())]))
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
