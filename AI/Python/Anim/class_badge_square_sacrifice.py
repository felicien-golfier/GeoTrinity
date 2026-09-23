"""Square badge sacrifice: the block draws what the channel feeds it into its keyhole, over and over, winding itself
round as it does, then fires it all back out and is slammed back and rolled over by the shot. Clips in the bottom
slot, so they play beside the auto-fire's montage in the top one and the mandibles are left to it.

    SK_SquareBadge_Montage_Sacrifice       GA_Square_SpecAlt_SacrificeBeam (Martyr Beam)
        Start     drawn out along the aim, then kicked back and rolled a little the wrong way as the beam leaves
        Channel   a pump: drawn into the keyhole accelerating and straining, a hard swallow, opened back up; three
                  pulls, rolling over about the aim all the while, one whole roll a loop: creeping on while drawn
                  in, lurching on at each swallow; looping until the channel stops it
    SK_SquareBadge_Montage_SacrificeSpit   GA_Square_SpecAlt_SacrificeDetonate (Martyr's Wrath)
        Start     drawn into the keyhole, straining, rolled the wrong way, held dead still into the shot
        End       slammed back and squashed flat by the ray, thrown into one whole roll over about the aim that slows
                  through the blast, pinned there, hauled slowly home

Nothing springs: every pose eases to a stop and nothing crosses back past rest, so the block reads as a mass pulled
and pushed rather than a jelly struck. The block's own scale and position carry that, about the body bone on the
block's centre line. Shrinking converges on the keyhole, so what shrinks reads as drawn into it. Swelling grows from
the front edge instead: that edge faces the mandibles, which the auto-fire rams flush against the block at rest, so
it never comes further forward than rest.

Both rolls are the block's alone, about the aim through the badge's mid-plane, which the mandibles sit in front of,
so the block turns over without reaching them; the Wrath's ray leaves from a mandible, so it still leaves straight
ahead. Every roll is a whole one.

Each ability plays Start stretched to its FireDelay and keeps that play rate for what follows, so everything is
authored at 30 fps against that rate.

Run AFTER AI/Python/Mesh/rig_class_badges.py and AI/Python/Anim/class_badge_square_fire_piston.py, via mcp-unreal
execute_script. Re-runnable: rewrites both sequences and montages in place. Report written to
Saved/class_badge_square_sacrifice.txt.
"""
import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/Class/SK_SquareBadge"
MESH_PATH = "/Game/Characters/Meshes/Class/SKM_SquareBadge"
GENERATOR = "AI/Python/Mesh/generate_class_badge_meshes.py"
ANIM_PACKAGE = "/Game/Characters/Anim/ClassBadge/Square"
AUTO_FIRE_SEQUENCE = ANIM_PACKAGE + "/SK_SquareBadge_Sequence_FirePiston"
KEYHOLE_SOCKET = "SacrificeHole"
REPORT = unreal.Paths.project_saved_dir() + "class_badge_square_sacrifice.txt"

BODY, LEFT, RIGHT = "Bottom", "MandibleLeft", "MandibleRight"
SLOT = "Bottom"

FPS = 30
START = 12  # both Start sections, stretched to the abilities' 0.2 s FireDelay


def smooth(alpha):
    return alpha * alpha * (3.0 - 2.0 * alpha)


def accelerate(alpha):
    return alpha ** 3


def snap(alpha):
    return alpha


def decelerate(alpha):
    return 1.0 - (1.0 - alpha) ** 2


REST = (1.0, 1.0, 0.0)
PULL = 24
DRAWN = (0.84, 0.86, 0.0)
SWALLOWED = (0.78, 0.80, 0.0)


def pull(first):
    """One pump from rest on frame `first`, back at rest by `first` + PULL."""
    return [(first + 14, DRAWN, accelerate),    # drawn into the keyhole
            (first + 16, DRAWN, smooth),        # held
            (first + 17, SWALLOWED, snap),      # the swallow
            (first + PULL, REST, decelerate)]   # opened back up


def wind(first, rolled):
    """One pull's share of the channel's roll, from `rolled` degrees on frame `first`: creeping on while the block is
    drawn in, tightening as it holds, lurching on at the swallow and running on as it opens back up."""
    share = 360.0 / PULLS
    return [(first + 14, (rolled + share * 0.45,), snap),   # creeping
            (first + 16, (rolled + share * 0.5,), snap),    # tightening
            (first + 17, (rolled + share * 0.68,), snap),   # the swallow
            (first + PULL, (rolled + share,), snap)]        # running on


def strain(first, last, peak):
    """Sideways tremble alternating every frame, growing from nothing on `first` to `peak` on `last`."""
    return [(frame, peak * (frame - first + 1) / float(last - first + 1) * (1 if frame % 2 else -1))
            for frame in range(first, last + 1)]


# Keys per clip: (frame, (X scale, Y scale, backward throw), easing into this key). The Channel ends on its own
# first pose, so the loop closes; PULL is even, so each pull's tremble alternates the same way.
PULLS = 3
CHANNEL_END = START + PULLS * PULL
SPIT_HELD = (0.74, 1.17, -15.0)
SPIT_END = 66
CLIPS = {
    "Sacrifice": {
        "sections": [("Start", 0, "Channel"), ("Channel", START, "Channel")],
        "frames": CHANNEL_END,
        "keys": [(0, REST, smooth),
                 (4, (1.04, 0.97, 0.0), smooth),     # drawn out along the aim
                 (6, (1.04, 0.97, 0.0), smooth),     # held
                 (8, (0.88, 1.08, -5.0), snap),      # kicked back as the beam leaves
                 (START, REST, smooth)]
                + [key for index in range(PULLS) for key in pull(START + index * PULL)],
        "tremble": dict(frame for index in range(PULLS)
                        for frame in strain(START + index * PULL + 8, START + index * PULL + 13, 1.3)),
        # Degrees the block rolls over about the aim: a little the wrong way as the beam leaves, then winding on
        # through the whole channel, one whole roll a loop, so the loop comes back on the same rotation.
        "roll": [(0, (0.0,), smooth), (6, (-15.0,), smooth), (START, (0.0,), smooth)]
                + [key for index in range(PULLS) for key in wind(START + index * PULL, index * 360.0 / PULLS)],
        "blend": (0.1, 0.2),
    },
    "SacrificeSpit": {
        "sections": [("Start", 0, "End"), ("End", START, "None")],
        "frames": SPIT_END + 1,
        "keys": [(0, REST, smooth),
                 (9, (0.80, 0.82, 0.0), accelerate),     # drawn into the keyhole
                 (START, (0.80, 0.82, 0.0), smooth),     # held dead still into the shot
                 (13, (0.75, 1.02, -9.0), snap),         # mid-flight
                 (14, (0.70, 1.22, -18.0), snap),        # slammed back, squashed flat, past where it holds
                 (16, (0.73, 1.18, -16.0), smooth),
                 (34, SPIT_HELD, smooth),                # pinned by the ray through the blast
                 (62, REST, smooth),                     # hauled home
                 (SPIT_END, REST, smooth)],
        "tremble": dict(strain(4, 8, 1.5)
                        + [(frame, 1.4 * (34 - frame) / 17.0 * (1 if frame % 2 else -1)) for frame in range(17, 34)]),
        # Degrees the block rolls over about the aim: wound the wrong way, then one whole roll thrown by the shot.
        "roll": [(0, (0.0,), smooth),
                 (9, (-25.0,), smooth),
                 (START, (-25.0,), smooth),
                 (13, (30.0,), snap),
                 (14, (75.0,), snap),
                 (34, (360.0,), decelerate),
                 (SPIT_END, (360.0,), smooth),
                 (SPIT_END + 1, (0.0,), snap)],   # the same rotation, a whole turn round
        "blend": (0.05, 0.2),
    },
}

LOG = []


def interpolate(keys, frame):
    for (before, start, _), (after, end, ease) in zip(keys, keys[1:]):
        if before <= frame <= after:
            alpha = ease((frame - before) / float(after - before))
            return tuple(a + (b - a) * alpha for a, b in zip(start, end))
    return keys[-1][1]


def body_state(clip, frame):
    """(X scale, Y scale, backward throw, sideways tremble, roll) of the block at a frame of a clip."""
    return interpolate(clip["keys"], frame) + (clip["tremble"].get(frame, 0.0), interpolate(clip["roll"], frame)[0])


def body_forward(scale_x, keyhole, front):
    """How far the block moves along X so a shrink converges on the keyhole and a swell grows from the front edge.

    `front` is the front edge's distance from the bone at the largest block scale the auto-fire holds.
    """
    return (1.0 - scale_x) * (keyhole if scale_x < 1.0 else front)


def key(toolkit, clip, keyhole, front, frame, bone, rest_local):
    translation, scale = rest_local.translation, rest_local.scale3d
    scale_x, scale_y, thrown, tremble, roll = body_state(clip, frame)
    if bone != BODY:
        return translation, rest_local.rotation, scale
    # The body bone sits below the badge's mid-plane, which is what the roll turns about.
    moved, rotation = toolkit["turn_about"](
        unreal.Vector(translation.x + body_forward(scale_x, keyhole, front) + thrown, translation.y + tremble,
                      translation.z),
        unreal.Rotator(roll=roll), unreal.Vector(0.0, 0.0, -translation.z))
    return moved, rotation, unreal.Vector(scale.x * scale_x, scale.y * scale_y, scale.z)


def build(toolkit, name, clip, keyhole, front):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    sequence = toolkit["get_or_create_asset"](ANIM_PACKAGE, "SK_SquareBadge_Sequence_" + name, unreal.AnimSequence,
                                              factory)
    # Rewritten in place, which would keep an additive type the asset was given.
    sequence.set_editor_property("additive_anim_type", unreal.AdditiveAnimationType.AAT_NONE)
    toolkit["write_bone_tracks"](sequence, SKELETON_PATH, FPS, clip["frames"],
                                 lambda frame, bone, rest_local: key(toolkit, clip, keyhole, front, frame, bone,
                                                                     rest_local),
                                 "Build square badge " + name)

    montage_name = "SK_SquareBadge_Montage_" + name
    montage = toolkit["build_montage"](sequence, ANIM_PACKAGE, montage_name, [n for n, _, _ in clip["sections"]],
                                       [start / float(FPS) for _, start, _ in clip["sections"]],
                                       [following for _, _, following in clip["sections"]], SLOT)
    for blend_name, seconds in zip(("blend_in", "blend_out"), clip["blend"]):
        blend = montage.get_editor_property(blend_name)
        blend.set_editor_property("blend_time", seconds)
        montage.set_editor_property(blend_name, blend)
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(ANIM_PACKAGE, montage_name))
    return sequence, montage


def block_outline():
    """The block's world outline, from the generator that built it."""
    path = unreal.Paths.project_dir() + GENERATOR
    generator = {"__name__": "badge_generator"}
    exec(compile(open(path).read(), path, "exec"), generator)
    return generator["to_world"]("SM_SquareBadge")[0].outline


def auto_fire_frames(toolkit, groups):
    """Every frame of the auto-fire -> [(block X scale, {mandible: placed vertices})]."""
    sequence = unreal.load_asset(AUTO_FIRE_SEQUENCE)
    frames = int(round(sequence.get_editor_property("sequence_length") * FPS))
    options = unreal.AnimPoseEvaluationOptions()
    table = []
    for frame in range(frames + 1):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        placed = toolkit["place_groups"](groups, toolkit["_component_transforms"](pose))
        table.append((toolkit["local_pose_table"](pose)[BODY][2][0], {bone: placed[bone] for bone in (LEFT, RIGHT)}))
    return table


def fit_report(toolkit, name, clip, sequence, auto_fire, outline, heights, reference):
    """How close a mandible comes to the block, seen from above and in three dimensions, with this clip playing beside
    every frame of the auto-fire. The clip owns the block and the auto-fire the mandibles, so each is read from its own.
    """
    options = unreal.AnimPoseEvaluationOptions()
    hulls = [[toolkit["convex_hull"](vertices) for vertices in mandibles.values()] for _, mandibles in auto_fire]
    worst, where, deepest = float("inf"), None, 0.0
    for frame in range(clip["frames"] + 1):
        block = toolkit["_component_transforms"](
            APE.get_anim_pose_at_time(sequence, frame / float(FPS), options))[BODY]
        seen = toolkit["move_outline"](outline, reference[BODY], block)
        for index, (_, mandibles) in enumerate(auto_fire):
            gap = min(toolkit["outline_separation"](hull, seen) for hull in hulls[index])
            if gap < worst:
                worst, where = gap, (frame, index)
            deepest = max(deepest, toolkit["sunk_depth"]([v for vertices in mandibles.values() for v in vertices],
                                                         outline, heights[0], heights[1], reference[BODY], block))
    LOG.append("%s beside the auto-fire: closest a mandible comes to the block from above %.2f, at clip frame %d / "
               "auto-fire frame %d (-1 = overlapping); deepest one sits inside it %.2f" % ((name, worst) + where
                                                                                         + (deepest,)))


def clip_report(toolkit, clip, sequence, montage, keyhole, front):
    LOG.append("{} - {} keys for {} frames, sections {}, additive {}".format(
        sequence.get_name(), toolkit["playable_key_count"](sequence), clip["frames"],
        toolkit["montage_sections"](montage), sequence.get_editor_property("additive_anim_type")))
    LOG.append("frame      sx     sy  forward  tremble   roll")
    for frame in range(clip["frames"] + 1):
        scale_x, scale_y, thrown, tremble, roll = body_state(clip, frame)
        LOG.append("%5d  %6.3f %6.3f  %+7.2f  %+7.2f  %5.1f" % (
            frame, scale_x, scale_y, body_forward(scale_x, keyhole, front) + thrown, tremble, roll))


try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    groups = toolkit["rigid_vertex_groups"](MESH_PATH, SKELETON_PATH)
    reference = toolkit["_component_transforms"](APE.get_reference_pose(unreal.load_asset(SKELETON_PATH)))
    block_at_rest = toolkit["place_groups"](groups, reference)[BODY]
    front_edge = max(vertex.x for vertex in block_at_rest)
    block_heights = (min(vertex.z for vertex in block_at_rest), max(vertex.z for vertex in block_at_rest))
    keyhole_x = unreal.load_asset(MESH_PATH).find_socket(KEYHOLE_SOCKET).get_editor_property("relative_location").x
    auto_fire = auto_fire_frames(toolkit, groups)
    swelling_front = front_edge * max(scale_x for scale_x, _ in auto_fire)
    LOG.append("block front edge at x %.2f (%.2f at the auto-fire's largest block scale), keyhole at x %.2f" % (
        front_edge, swelling_front, keyhole_x))

    outline = block_outline()
    for clip_name, clip in CLIPS.items():
        built_sequence, built_montage = build(toolkit, clip_name, clip, keyhole_x, swelling_front)
        LOG.append("")
        clip_report(toolkit, clip, built_sequence, built_montage, keyhole_x, swelling_front)
        fit_report(toolkit, clip_name, clip, built_sequence, auto_fire, outline, block_heights, reference)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
