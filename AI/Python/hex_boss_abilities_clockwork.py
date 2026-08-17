"""Hex boss ability animations, clockwork cut: the same three abilities as `hex_boss_abilities.py`, moved as a
mechanism rather than as a muscle.

Nothing here eases. Every part holds dead still, crosses to its next position in two frames with one of them
mid-flight, and holds again — so the clip is a sequence of stops, and the stillness between them is what makes
each one land. Where the other cut winds up on one accelerating curve and recoils on a spring, this one winds up
on a run of clicks whose gaps shrink, slams once, ratchets while it runs, and is let go click by click.

The three rings answer the wind-up's clicks in turn rather than together, so they fall like tumblers instead of
moving as one block. That replaces the other cut's per-part delays: the same refusal to move as a single object,
reached by taking turns rather than by lagging.

A click can never be a whole sixth of a turn. A sixth maps a hexagon onto itself, so a ring stepped by exactly
that much looks untouched — the ratchet has to have finer teeth than its own symmetry, with a whole number of
clicks making up the sixths that let a loop close and a clip end mid-turn.

Every clip starts and ends on the reference pose with the spikes folded, so both cuts blend against the idle and
against each other. Sections match the other cut's: the wind-up stretches to the ability's fire delay, the slam
is the frame the pattern goes live, the ratchet holds until the pattern ends.

Nothing moves along Z and nothing scales on it: under an orthographic camera that motion is spent for nothing.

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
          "0b3b8214-c633-4920-a4b1-c68d4e9de203/scratchpad/hex_boss_clockwork.txt")

OUTER, MID, CORE = "HexOuter", "HexMid", "HexCore"
SPIKE_MARK = "_Spike_"
SYMMETRY = 60.0     # degrees that map a hexagon onto itself

FPS = 30
SAMPLE = 6          # frames between report rows

SNAP = 2            # frames a click takes, one of them mid-flight, so the spacing itself reads as speed
STILL = 8           # frames held dead still after the last wind-up click, which is what makes the slam land
SETTLE = 4          # frames held after the slam before the ratchet takes over
TAIL = 4            # frames held at rest after the last release click

# (which of the wind-up's clicks it answers, degrees it turns per wind-up click, degrees per ratchet click,
#  its scale wound tightest, its scale deployed)
Ring = collections.namedtuple("Ring", "phase wind_degrees loop_degrees charge_scale burst_scale")

# (asset suffix, frames between wind-up clicks, ratchet frames, clicks in the ratchet, frames between release
#  clicks, how sharply the spikes favour the front, spike reach wound tightest, spike reach deployed, the rings)
Clip = collections.namedtuple(
    "Clip", "name wind_gaps loop beats release_gaps focus charge_spike burst_spike rings")

CLIPS = [
    # Sweeping laser: nine clicks winding tighter, one slam, then a fine fast ratchet with all three rings
    # running at their own rate and every blade out, because the beam covers the whole arc.
    Clip("SweepBeam", [9, 8, 8, 7, 6, 6, 5, 4, 3], 40, 8, [5, 8, 12], 0.0, 0.30, 0.95, {
        OUTER: Ring(0, 20.0, 7.5, 0.88, 1.12),
        MID: Ring(1, -20.0, -15.0, 0.85, 1.06),
        CORE: Ring(2, 20.0, 22.5, 1.20, 1.05)}),
    # Tile-carving ray: direction locks at activation, so nothing turns while it winds — the tell is the idle's
    # motion stopping and the rings clamping down in stages. The ratchet then walks all three round together,
    # handing the aimed slot from one blade to the next like a chamber coming round.
    Clip("CarvingRay", [12, 10, 9, 7, 6, 4], 24, 4, [5, 8, 11], 8.0, 0.45, 1.00, {
        OUTER: Ring(0, 0.0, 15.0, 0.82, 1.16),
        MID: Ring(1, 0.0, 15.0, 0.84, 1.12),
        CORE: Ring(2, 0.0, 15.0, 1.20, 1.10)}),
    # Cone spray: the outer rings hold as the fan's frame while the core alone steps round, one ratchet click
    # per salve, so each salve chambers a fresh set of blades into the cone.
    Clip("ConeSpray", [7, 7, 6, 5, 5, 4, 4, 3, 3], 15, 3, [4, 6, 9], 3.0, 0.25, 0.85, {
        OUTER: Ring(0, 0.0, 0.0, 0.92, 1.08),
        MID: Ring(1, 0.0, 0.0, 0.88, 1.12),
        CORE: Ring(2, 20.0, 20.0, 1.20, 1.18)}),
]

LOG = []


def running(gaps, first):
    """Frames the clicks land on, `gaps` frames apart, the first that far after `first`."""
    frames, at = [], first
    for gap in gaps:
        at += gap
        frames.append(at)
    return frames


def wind_clicks(clip):
    return running(clip.wind_gaps, 0)


def slam(clip):
    return wind_clicks(clip)[-1] + STILL


def burst_end(clip):
    return slam(clip) + SNAP + SETTLE


def loop_clicks(clip):
    """The ratchet's clicks, the first landing as the ratchet opens and the last a whole beat before it closes."""
    return [burst_end(clip) + beat * clip.loop // clip.beats for beat in range(clip.beats)]


def loop_end(clip):
    return burst_end(clip) + clip.loop


def release_clicks(clip):
    return running(clip.release_gaps, loop_end(clip))


def total_frames(clip):
    return release_clicks(clip)[-1] + SNAP + TAIL


def stepped(frame, clicks):
    """Clicks landed by `frame`, crossing between whole numbers over SNAP frames and holding still between.

    Exactly an integer everywhere except during a click, which is what makes the stillness read as stillness.
    """
    return sum(min(1.0, max(0.0, (frame - start) / float(SNAP))) for start in clicks)


def ring_clicks(clicks, phase, count):
    """The clicks one phase answers: every `count`-th one, so the rings take turns instead of moving together."""
    return [start for index, start in enumerate(clicks) if index % count == phase]


def deploy(frame, clip, ring):
    """-1 wound tightest, +1 deployed, 0 at rest for one ring, stepped the whole way.

    Wound by the share of the wind-up's clicks this ring answers, thrown over by the slam that takes all three at
    once, and let back by its share of the release's. The release runs in the reverse order to the wind-up, from
    the inside out: an outer ring letting go first would close onto one still standing at its deployed size.
    """
    count = len(clip.rings)
    wound = stepped(frame, ring_clicks(wind_clicks(clip), ring.phase, count)) \
        / float(len(clip.wind_gaps) // count)
    freed = stepped(frame, ring_clicks(release_clicks(clip), count - 1 - ring.phase, count)) \
        / float(len(clip.release_gaps) // count)
    thrown = stepped(frame, [slam(clip)])
    return -wound * (1.0 - thrown) + thrown * (1.0 - freed)


def ring_yaw(frame, clip, ring):
    """Degrees the ring has turned by `frame`, kept rather than unwound.

    Each run of clicks adds up to a whole number of sixths, so every hold lands on an orientation the eye cannot
    tell from rest — which is what lets the ratchet close its loop and the clip end mid-turn.
    """
    return (ring.wind_degrees * stepped(frame, ring_clicks(wind_clicks(clip), ring.phase, len(clip.rings)))
            + ring.loop_degrees * stepped(frame, loop_clicks(clip)))


def ring_scale(frame, clip, ring):
    """The ring's XY scale, stepping between rest, its wound size and its deployed one."""
    value = deploy(frame, clip, ring)
    return 1.0 + value * ((1.0 - ring.charge_scale) if value < 0.0 else (ring.burst_scale - 1.0))


def spike_reach(frame, clip, ring, angle):
    """A spike's X scale: 0 folded into its face, 1 the full blade.

    Weighted by where the spike points at this frame rather than where it rests, so a ratcheting ring hands the
    aimed slot from one blade to the next. A focus of zero flattens that to every spike alike.
    """
    value = deploy(frame, clip, ring)
    reach = -value * clip.charge_spike if value < 0.0 else value * clip.burst_spike
    return reach * max(0.0, math.cos(math.radians(angle))) ** clip.focus


def key(frame, bone, clip, rest, layout):
    """The bone's local transform at `frame` -> (translation, yaw, scale)."""
    (x, y, z), yaw, (sx, sy, sz) = rest[bone]
    if bone in clip.rings:
        ring = clip.rings[bone]
        yaw += ring_yaw(frame, clip, ring)
        factor = ring_scale(frame, clip, ring)
        sx, sy = sx * factor, sy * factor
    elif bone in layout:
        parent, angle = layout[bone]
        ring = clip.rings[parent]
        sx = spike_reach(frame, clip, ring, angle + ring_yaw(frame, clip, ring))
    return (x, y, z), yaw, (sx, sy, sz)


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


def check_symmetry(clip):
    """Every run of clicks has to add up to whole sixths, and no single click may reach half of one.

    A click of a whole sixth would be invisible and one of half a sixth reads as likely to be turning either way,
    so both bound the ratchet's tooth size from opposite sides.
    """
    for name, ring in clip.rings.items():
        for label, degrees, count in (("wind-up", ring.wind_degrees, len(clip.wind_gaps) // len(clip.rings)),
                                      ("ratchet", ring.loop_degrees, clip.beats)):
            if abs(degrees) >= SYMMETRY / 2.0:
                raise RuntimeError("{} {} click of {}deg reaches half a sixth and reads either way".format(
                    name, label, degrees))
            if round(degrees * count, 6) % SYMMETRY:
                raise RuntimeError("{} {} turns {}deg, which is not whole sixths".format(
                    name, label, degrees * count))


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
    name = "SK_HexBoss_Sequence_{}_Clockwork".format(clip.name)
    sequence = get_or_create(name, unreal.AnimSequence, factory)
    frames = total_frames(clip)

    controller = sequence.get_editor_property("controller")
    controller.open_bracket("Build {} clockwork".format(clip.name))
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
    name = "SK_HexBoss_Montage_{}_Clockwork".format(clip.name)
    montage = get_or_create(name, unreal.AnimMontage, factory)

    names = ["Start", "Fire", "FireLoop", "End"]
    starts = [0.0, slam(clip) / float(FPS), burst_end(clip) / float(FPS), loop_end(clip) / float(FPS)]
    following = ["Fire", "FireLoop", "FireLoop", "None"]

    util = unreal.get_default_object(unreal.GeoAnimBuilderUtil)
    util.set_montage_slot_segment(montage, sequence, "DefaultSlot")
    util.set_montage_sections(montage, [unreal.Name(n) for n in names], starts,
                              [unreal.Name(n) for n in following])
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(ANIM_PACKAGE, name))
    return montage, names, starts


def report(clip, sequence, toolkit, groups, layout, montage_names, montage_starts):
    """Evaluate the finished sequence — the track list reads a legacy path and reports empty for every one."""
    frames = total_frames(clip)
    options = unreal.AnimPoseEvaluationOptions()
    marks = sorted(set(list(range(0, frames + 1, SAMPLE))
                       + wind_clicks(clip) + [slam(clip), burst_end(clip), loop_end(clip), frames]))
    inner = {CORE: [b for b in groups if b.startswith(CORE)], MID: [b for b in groups if b.startswith(MID)]}

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
    LOG.append("=" * 108)
    LOG.append("SK_HexBoss_Sequence_{}_Clockwork — {} frames at {} fps = {:.2f}s, {} sampled keys (expect {})".format(
        clip.name, frames, FPS, frames / float(FPS),
        sequence.get_editor_property("number_of_sampled_keys"), frames + 1))
    LOG.append("sections " + ", ".join("%s f%d (%.2fs)" % (name, round(start * FPS), start)
                                       for name, start in zip(montage_names, montage_starts)))
    LOG.append("{} wind-up clicks {} frames apart, slam f{}, {} ratchet clicks every {} frames, {} released".format(
        len(clip.wind_gaps), clip.wind_gaps, slam(clip), clip.beats, clip.loop // clip.beats,
        len(clip.release_gaps)))
    LOG.append("focus {:.0f}; per click outer {:+.1f}/{:+.1f}deg, mid {:+.1f}/{:+.1f}, core {:+.1f}/{:+.1f}"
               " (wind/ratchet, a sixth is {:.0f})".format(
                   clip.focus, *([v for ring in (OUTER, MID, CORE)
                                  for v in (clip.rings[ring].wind_degrees, clip.rings[ring].loop_degrees)]
                                 + [SYMMETRY])))
    LOG.append("")
    LOG.append("frame  outer          mid            core           blade past face   room inside")
    LOG.append("       yaw   scale    yaw   scale    yaw   scale    max    out (18)  core   mid")

    for frame in marks:
        local = snapshot(APE.get_anim_pose_at_time(sequence, frame / float(FPS), options))
        past, gaps = measured[frame]
        LOG.append("%5d " % frame
                   + " ".join("%6.1f %6.3f " % (local[ring][1], local[ring][2][0]) for ring in (OUTER, MID, CORE))
                   + "  %6.1f %5d  %6.1f %6.1f  %s" % (
                       max(past.values()), sum(1 for value in past.values() if value > 0.0), gaps[0], gaps[1],
                       "#" * int(round(max(0.0, max(past.values())) / 6.0))))

    folded = [max(measured[frame][0].values()) for frame in (0, frames)]
    closure = [sorted(measured[burst_end(clip)][0].values()), sorted(measured[loop_end(clip)][0].values())]
    rings = [snapshot(APE.get_anim_pose_at_time(sequence, frame / float(FPS), options))
             for frame in (burst_end(clip), loop_end(clip))]
    room = min(min(gaps) for _, gaps in measured.values())
    LOG.append("")
    LOG.append("folded at both ends: frame 0 max blade %.1f, frame %d max blade %.1f (negative = buried)" % (
        folded[0], frames, folded[1]))
    LOG.append("FireLoop closes: silhouette gap %.4f units, ring scale gap %.5f, ring yaw gap %.4f deg (mod 60)" % (
        max(abs(a - b) for a, b in zip(*closure)),
        max(abs(rings[0][ring][2][0] - rings[1][ring][2][0]) for ring in clip.rings),
        max(abs((rings[0][ring][1] - rings[1][ring][1] + SYMMETRY / 2.0) % SYMMETRY - SYMMETRY / 2.0)
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

    LOG.append("hex boss ability clips, clockwork cut — stepped throughout, nothing eased")
    LOG.append("every clip starts and ends on the reference pose with the spikes folded, so it blends against")
    LOG.append("the idle and against the other cut")
    for clip in CLIPS:
        check_symmetry(clip)
        sequence = build_sequence(clip, rest, layout)
        montage, names, starts = build_montage(clip, sequence)
        report(clip, sequence, toolkit, groups, layout, names, starts)
        LOG.append("montage sections read back: {}".format(
            [str(montage.get_section_name(i)) for i in range(montage.get_num_sections())]))
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
