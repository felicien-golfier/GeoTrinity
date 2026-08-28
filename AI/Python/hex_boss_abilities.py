"""Hex boss ability animations: the sweep beam, the tile-carving ray and the cone spray.

One clip per ability, each cut into the four montage sections the pattern machinery drives: Start winds up and is
stretched to the ability's FireDelay, Fire is the burst on the frame the pattern goes live, FireLoop holds the
deployed boss for as long as the pattern runs, and End recoils back. FireLoop is a separate section because the
live phase has no fixed length; UPattern accepts any section whose name contains "Fire", which is what lets it
loop on itself until EndPattern jumps to End.

Every part reads one curve — negative wound up, positive deployed, zero at rest — each a fixed number of frames
late, the body undelayed.

Constraints:
- A clip starts and ends on the reference pose with the spikes folded.
- Ring turns are whole sixths, which map a hexagon onto itself: nothing unwinds, and FireLoop closes on its own
  first frame along with the loop pump and the judder beat.
- The body is capped at HITBOX_SCALE outside the hit frame — the capsule never moves.
- The lag order reverses across the burst, so an inner ring never opens into a shell that has not.
- A blade past reach 1 is stretched, and goes through whatever its ring sits inside: a turning ring clears that
  shell's near side, a still one its corners.
- Only the middle ring rests with a spike facing front, so a sharp focus leaves the other two barely open.
- Nothing moves or scales on Z under an orthographic camera.

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
          "8d5b0c3d-b469-4d6a-8e0e-557ad9e88982/scratchpad/hex_boss_abilities.txt")

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
GRIND_BEAT = 4      # frames a working ring's judder takes to come round; a loop holds a whole number of them
HITBOX_SCALE = 1.2  # past this the boss looks bigger than its capsule can be hit at, so only the hit goes there
PUMP_RISE = 0.20    # of the loop spent expanding; the rest of it decays back
PUMP_SNAP = 0.6     # below 1 the expansion is fastest where it starts
PUMP_FALL = 1.6     # above 1 the decay lingers instead of dropping straight back
# The most a turning beat falls behind its own clock; the wind-up shake is scaled against it.
SLIP_PEAK = max(step / 200.0 - (step / 200.0) ** CHARGE_EASE for step in range(201))

# (frames this ring lags the curve, its scale wound tightest, its scale deployed, sixths of a turn it makes
#  winding up, sixths it makes per loop, how much it pulses over the loop, how far its blades reach deployed —
#  1 is the blade the mesh gives, past that a stretched one — and the units it judders through the live phase)
Ring = collections.namedtuple(
    "Ring", "lag charge_scale burst_scale charge_sixths loop_sixths loop_pulse burst_spike loop_shake")

# The whole boss. Hit and hold are separate scales because the capsule never moves.
Body = collections.namedtuple("Body", "charge_scale punch_scale hold_scale loop_pulse")

# The wind-up's two turning beats: crank back as a fraction of the net turn, then frames per beat. None instead
# winds up as one accelerating ramp, turning as it draws in.
Swing = collections.namedtuple("Swing", "back frames")

# (asset suffix, wind-up frames, loop frames, recoil frames, how sharply the spikes favour the front,
#  spike reach wound tightest, the whole-boss swell, the wind-up's crank, units it rattles at its loudest,
#  the rings)
Clip = collections.namedtuple("Clip", "name start loop end focus charge_spike body swing shake rings")

CLIPS = [
    # Sweeping laser: one unbroken accelerating wind-up, then a spinning emitter with teeth all round.
    # Tightest of the three for room — every ring turns, so every inner blade is held to a near side.
    Clip("SweepBeam", 54, 36, 20, 0.0, 0.30, Body(0.68, 1.22, 1.00, 0.050), None, 18.0, {
        OUTER: Ring(3, 0.86, 1.14, 2, 2, 0.000, 1.80, 0.0),
        MID: Ring(2, 0.82, 1.00, -4, -4, 0.000, 1.03, 0.0),
        CORE: Ring(0, 1.18, 1.00, 8, 8, 0.000, 1.30, 0.0)}),
    # Tile-carving ray: direction locks at activation, so the outer two never turn and the wind-up is rattle
    # alone. Only the core turns, and only once live, juddering as it goes. That turn is what lets its blades
    # read: at rest none of its six faces front, so a still core barely opens whatever reach it is given.
    # Standing still buys the outer two the longest blades of any clip — corners, not near sides.
    Clip("CarvingRay", 48, 12, 24, 5.0, 0.45, Body(0.62, 1.30, 1.00, 0.000), None, 22.0, {
        OUTER: Ring(3, 0.80, 1.08, 0, 0, 0.100, 3.30, 0.0),
        MID: Ring(2, 0.84, 1.00, 0, 0, 0.100, 1.45, 0.0),
        CORE: Ring(0, 1.18, 1.00, 0, 1, 0.100, 1.40, 8.0)}),
    # Cone spray: salvos out of the front, the core turning so its blades ripple across the cone. Three-beat
    # wind-up — crank right, whip left, then clamp still. Only the body pulses through the loop, so every ring
    # holds its spacing there.
    Clip("ConeSpray", 50, 15, 18, 3.0, 0.25, Body(0.70, 1.25, 1.00, 0.100), Swing(0.40, 15), 16.0, {
        OUTER: Ring(3, 0.86, 1.09, 1, 0, 0.000, 1.90, 0.0),
        MID: Ring(2, 0.86, 1.00, -1, 0, 0.000, 1.35, 0.0),
        CORE: Ring(0, 1.22, 1.00, 2, 1, 0.000, 1.35, 0.0)}),
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

    Copied out because every evaluated pose reuses one buffer.
    """
    table = {}
    for bone in APE.get_bone_names(pose):
        local = APE.get_bone_pose(pose, str(bone), unreal.AnimPoseSpaces.LOCAL)
        table[str(bone)] = ((local.translation.x, local.translation.y, local.translation.z),
                            local.rotation.rotator().yaw,
                            (local.scale3d.x, local.scale3d.y, local.scale3d.z))
    return table


def spike_layout(placed):
    """Spike bone -> (the ring it hangs off, the direction it rests pointing in degrees)."""
    layout = {}
    for bone, transform in placed.items():
        if SPIKE_MARK in bone:
            position = transform.translation
            layout[bone] = (bone.split(SPIKE_MARK)[0], math.degrees(math.atan2(position.y, position.x)))
    return layout


def clamp_start(clip):
    """The frame the wind-up stops turning and starts drawing itself in — its first, for a clip that never cranks."""
    return 2 * clip.swing.frames if clip.swing else 0


def charged(frame, clip):
    """0 to 1 across the wind-up's clamp, accelerating; 1 once the boss has stopped dead."""
    return min(1.0, max(0.0, frame - clamp_start(clip))
               / float(clip.start - STILL - 1 - clamp_start(clip))) ** CHARGE_EASE


def turned(frame, clip):
    """0 to 1 across the wind-up as the rings' rotation reads it: right against the net turn, then whipped left.

    Ends at exactly 1, so a ring lands on the whole sixth charge_sixths asks for. No crank falls back on the
    clamp's own ramp.
    """
    if not clip.swing:
        return charged(frame, clip)
    if frame >= clamp_start(clip):
        return 1.0
    beat, back = clip.swing.frames, clip.swing.back
    eased = ((frame % beat) / float(beat)) ** CHARGE_EASE
    return -back * eased if frame < beat else -back + (1.0 + back) * eased


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
    """The whole boss's offset this frame -> (x, y): the load the mechanism is under, as a vibration.

    Alternates every frame; slower reads as a wobble. Scaled by load, not time — through a turning beat, how far
    the rings have fallen behind it. Dead for the last STILL frames, which is what the hit lands against.
    """
    if frame >= clip.start - STILL:
        return 0.0, 0.0
    if clip.swing and frame < clamp_start(clip):
        slip = (frame % clip.swing.frames) / float(clip.swing.frames)
        load = (slip - slip ** CHARGE_EASE) / SLIP_PEAK
    else:
        load = charged(frame, clip) ** SHAKE_RAMP
    reach = clip.shake * load
    return (reach if frame % 2 else -reach), (reach * 0.6 if frame % 4 < 2 else -reach * 0.6)


def grind(frame, clip, ring):
    """One ring's own offset this frame -> (x, y): the live phase's judder, hung off the ring not the root.

    Counted from the loop's first frame, so its last holds the same offset — which needs the loop to be whole
    GRIND_BEAT beats long.
    """
    if not ring.loop_shake or not burst_end(clip) <= frame <= loop_end(clip):
        return 0.0, 0.0
    step, reach = frame - burst_end(clip), ring.loop_shake
    return (reach if step % 2 else -reach), (reach * 0.6 if step % GRIND_BEAT < 2 else -reach * 0.6)


def lagged(frame, clip, ring):
    """The curve this ring reads, delayed by its own weight.

    The delay reverses from the burst to loop_end so the outer ring opens first — an inner ring must not swell
    into a shell that has not. The plateau either side outlasts the largest delay, so no ring jumps on the flip.
    """
    delay = MAX_LAG - ring.lag if clip.start <= frame <= loop_end(clip) else ring.lag
    return drive(frame - delay, clip)


def ring_yaw(frame, clip, ring):
    """Degrees the ring has turned by `frame`, kept rather than unwound — both spins are whole sixths."""
    yaw = 60.0 * ring.charge_sixths * turned(frame, clip)
    if frame > burst_end(clip):
        yaw += 60.0 * ring.loop_sixths * (min(frame, loop_end(clip)) - burst_end(clip)) / float(clip.loop)
    return yaw


def pulsed(frame, clip, scale, loop_pulse):
    """`scale` pumped once over the loop: out fast, back slow, nothing at either end, which closes the loop."""
    if not burst_end(clip) <= frame <= loop_end(clip):
        return scale
    within = (frame - burst_end(clip)) / float(clip.loop)
    pump = (within / PUMP_RISE) ** PUMP_SNAP if within < PUMP_RISE \
        else ((1.0 - within) / (1.0 - PUMP_RISE)) ** PUMP_FALL
    return scale * (1.0 + loop_pulse * pump)


def ring_scale(frame, clip, ring):
    """The ring's XY scale: the curve it lags, spread between its wound and its deployed size, plus the loop pulse."""
    value = lagged(frame, clip, ring)
    return pulsed(frame, clip,
                  1.0 + value * ((1.0 - ring.charge_scale) if value < 0.0 else (ring.burst_scale - 1.0)),
                  ring.loop_pulse)


def body_scale(frame, clip):
    """The whole boss's XY scale: clamped winding up, punched on the hit, then held where the capsule covers it.

    The overshoot alone drives the punch and everything below it the hold, so the size the capsule cannot follow
    lasts the hit and no longer.
    """
    value, body = drive(frame, clip), clip.body
    if value < 0.0:
        return 1.0 + value * (1.0 - body.charge_scale)
    overshoot = max(0.0, value - 1.0) / (max(BURST) - 1.0)
    return pulsed(frame, clip, 1.0 + min(1.0, value) * (body.hold_scale - 1.0)
                  + overshoot * (body.punch_scale - body.hold_scale), body.loop_pulse)


def overall(frame, clip):
    """How big the boss reads against its capsule; every frame past HITBOX_SCALE is one it cannot be hit at."""
    return body_scale(frame, clip) * ring_scale(frame, clip, clip.rings[OUTER])


def spike_reach(frame, clip, ring, angle):
    """A spike's X scale: 0 folded into its face, 1 the blade the mesh gives it, past that a stretched one.

    `angle` is where the spike points this frame, not where it rests, so a turning ring opens its blades as they
    sweep the front. Capped at the ring's reach — the burst's overshoot belongs to the rings, not the blades.
    """
    value = lagged(frame, clip, ring)
    reach = -value * clip.charge_spike if value < 0.0 else min(1.0, value) * ring.burst_spike
    return reach * max(0.0, math.cos(math.radians(angle))) ** clip.focus


def key(frame, bone, clip, rest, layout):
    """The bone's local transform at `frame` -> (translation, yaw, scale)."""
    (x, y, z), yaw, (sx, sy, sz) = rest[bone]
    if bone == ROOT:
        shake_x, shake_y = rattle(frame, clip)
        x, y = x + shake_x, y + shake_y
        factor = body_scale(frame, clip)
        sx, sy = sx * factor, sy * factor
    elif bone in clip.rings:
        ring = clip.rings[bone]
        judder_x, judder_y = grind(frame, clip, ring)
        x, y = x + judder_x, y + judder_y
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

    Loaded, not looked up: a scanning registry reports an on-disk asset as missing and the create then fails.
    Never deletes — deleting a loaded asset leaves the package unloadable for the session.
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
        controller.add_bone_curve(bone)  # result ignored: it reports failure for a track that already exists
        controller.set_bone_track_keys(bone, positions, rotations, scales)
    controller.close_bracket()
    unreal.AnimationLibrary.finalize_bone_animation(sequence)  # keys land in the raw model; this builds playback
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(ANIM_PACKAGE, name))
    return sequence


def build_montage(clip, sequence):
    """Cut the clip into the sections the pattern drives, FireLoop holding until EndPattern jumps to End.

    The factory builds the slot track from the source animation; sections have no scripting path and go through
    the C++ shim.
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
    """The silhouette per frame -> {frame: ({spike: units past its face}, (core in mid, mid in outer))}."""
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
                       + [peak, hush, clip.start, burst_end(clip), loop_end(clip), frames]
                       + ([clip.swing.frames, clamp_start(clip)] if clip.swing else [])))
    # Every frame, not just the printed rows: rings cross over two or three frames a coarse sample steps past.
    reach = protrusion(sequence, toolkit, groups, layout, range(frames + 1))
    options = unreal.AnimPoseEvaluationOptions()
    # Off the curve, not the wind-up's end: a crank turns fastest where its second beat lands.
    spin = 60.0 * max(abs(ring.charge_sixths) for ring in clip.rings.values()) \
        * max(abs(turned(frame, clip) - turned(frame - 1, clip)) for frame in range(1, clip.start))
    loop_spin = 60.0 * max(abs(ring.loop_sixths) for ring in clip.rings.values()) / float(clip.loop)

    LOG.append("")
    LOG.append("=" * 108)
    LOG.append("SK_HexBoss_Sequence_{} — {} frames at {} fps = {:.2f}s, {} sampled keys (expect {})".format(
        clip.name, frames, FPS, frames / float(FPS),
        sequence.get_editor_property("number_of_sampled_keys"), frames + 1))
    LOG.append("sections " + ", ".join("%s f%d (%.2fs)" % (name, round(start * FPS), start)
                                       for name, start in zip(montage_names, montage_starts)))
    LOG.append("wind-up: {}, then {} frames clamping and {} dead still".format(
        "two beats of {} frames cranking {:.0f}% back before the whip".format(
            clip.swing.frames, 100.0 * clip.swing.back) if clip.swing else "one accelerating ramp",
        clip.start - STILL - clamp_start(clip), STILL))
    LOG.append("body {:.2f} wound, {:.2f} on the hit, {:.2f} held, pumping {:.1f}%; focus {:.0f}, spikes reach"
               " {:.2f} wound and {} deployed (outer/mid/core), wind-up rattles {:.0f} units; fastest wind-up"
               " spin {:.1f} deg/frame (strobes past 30)".format(
                   clip.body.charge_scale, clip.body.punch_scale, clip.body.hold_scale,
                   100.0 * clip.body.loop_pulse, clip.focus, clip.charge_spike,
                   "/".join("%.2f" % clip.rings[ring].burst_spike for ring in (OUTER, MID, CORE)),
                   clip.shake, spin))
    LOG.append("live phase turns {} sixths a loop (outer/mid/core), fastest {:.1f} deg/frame; {}".format(
        "/".join(str(clip.rings[ring].loop_sixths) for ring in (OUTER, MID, CORE)), loop_spin,
        ", ".join("%s judders %.0f units" % (ring, clip.rings[ring].loop_shake)
                  for ring in (OUTER, MID, CORE) if clip.rings[ring].loop_shake) or "nothing judders"))
    LOG.append("")
    LOG.append("frame  drive  shake  body    outer          mid            core           blade past face   room inside")
    LOG.append("               x   y  scale   yaw   scale    yaw   scale    yaw   scale    max    out (18)  core   mid")

    poses = {}
    for frame in marks:
        local = poses[frame] = snapshot(APE.get_anim_pose_at_time(sequence, frame / float(FPS), options))
        past, gaps = reach[frame]
        LOG.append("%5d %+6.2f %4.0f %3.0f %6.3f " % ((frame, drive(frame, clip)) + local[ROOT][0][:2]
                                                      + (local[ROOT][2][0],))
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
    LOG.append("FireLoop closes: silhouette gap %.4f units, scale gap %.5f, ring yaw gap %.4f deg (mod 60)" % (
        max(abs(a - b) for a, b in zip(*closure)),
        max(abs(rings[0][part][2][0] - rings[1][part][2][0]) for part in list(clip.rings) + [ROOT]),
        max(abs((rings[0][ring][1] - rings[1][ring][1] + 30.0) % 60.0 - 30.0) for ring in clip.rings)))
    LOG.append("closest the rings ever come: %.1f units%s" % (room, "" if room > 0.0 else "  <-- THEY OVERLAP"))
    scales = [overall(frame, clip) for frame in range(frames + 1)]
    over = [frame for frame, scale in enumerate(scales) if scale > HITBOX_SCALE]
    LOG.append("against the capsule: peaks at %.2f of base, the loop never passes %.2f, and the only frames past"
               " %.2f are %s" % (max(scales), max(scales[burst_end(clip):loop_end(clip) + 1]), HITBOX_SCALE,
                                 over or "none"))


try:
    # Run the same way a script is, so the silhouette maths lives in one place.
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
    # A judder closes only over whole beats.
    ragged = [clip.name for clip in CLIPS if clip.loop % GRIND_BEAT
              and any(ring.loop_shake for ring in clip.rings.values())]
    if ragged:
        raise RuntimeError("{} judder over a loop that is not whole {}-frame beats, so it would not close".format(
            ragged, GRIND_BEAT))

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
