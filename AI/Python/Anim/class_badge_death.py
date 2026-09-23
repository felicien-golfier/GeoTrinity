"""Death montages for the three class badges, on the beat of the old shapes' own: the badge swells a fifth over eight
frames on a smoothstep, sheds its class's debris mid-swell, and is gone on the ninth, held gone for the rest of the
half second.

The scale is written on the root, which sits on the actor origin, so the whole badge — body and parts alike —
shrinks onto the point it turns about. Only X and Y move: under the top-down orthographic camera Z is spent for
nothing. The montage plays in DefaultSlot, which the badge AnimBlueprints put over both layers, so a death takes the
whole badge whatever its top and bottom layers are doing.

Run AFTER AI/Python/Mesh/rig_class_badges.py, via mcp-unreal execute_script. Re-runnable: rewrites every sequence and
montage in place. Report written to Saved/class_badge_death.txt.
"""
import unreal

LIBRARY = unreal.AnimationLibrary

MESH_FOLDER = "/Game/Characters/Meshes/Class"
ANIM_FOLDER = "/Game/Characters/Anim/ClassBadge"
REPORT = unreal.Paths.project_saved_dir() + "class_badge_death.txt"

ROOT = "Root"
SLOT = "DefaultSlot"
SECTION = "Default"
NOTIFY_TRACK = "1"

FPS = 30
FRAMES = 15          # half a second
SWELL_FRAMES = 8     # frames of swell before the badge goes
SWELL = 1.2
DEBRIS_FRAME = 4     # mid-swell, so the chunks lead the collapse rather than trail it
BLEND_TIME = 0.25

DEBRIS = {
    "Square": "/Game/Art/VFX/Assets/NS_DeathDebris",
    "Triangle": "/Game/Art/VFX/Assets/NS_DeathDebrisTriangle",
    "Circle": "/Game/Art/VFX/Assets/NS_DeathDebrisCircle",
}

LOG = []


def scale_at(frame):
    """The badge's XY scale on `frame`: a smoothstep swell, then gone."""
    if frame > SWELL_FRAMES:
        return 0.0
    alpha = frame / float(SWELL_FRAMES)
    return 1.0 + (SWELL - 1.0) * alpha * alpha * (3.0 - 2.0 * alpha)


def key(frame, bone, rest_local):
    scale = rest_local.scale3d
    factor = scale_at(frame) if bone == ROOT else 1.0
    return rest_local.translation, rest_local.rotation, unreal.Vector(scale.x * factor, scale.y * factor, scale.z)


def build(toolkit, badge):
    skeleton_path = "{}/SK_{}Badge".format(MESH_FOLDER, badge)
    package = "{}/{}".format(ANIM_FOLDER, badge)
    sequence_name = "SK_{}Badge_Sequence_Death".format(badge)
    montage_name = "SK_{}Badge_Montage_Death".format(badge)

    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(skeleton_path))
    sequence = toolkit["get_or_create_asset"](package, sequence_name, unreal.AnimSequence, factory)
    toolkit["write_bone_tracks"](sequence, skeleton_path, FPS, FRAMES, key, "Build {} badge death".format(badge))

    montage = toolkit["build_montage"](sequence, package, montage_name, [SECTION], [0.0], ["None"], SLOT)
    for name in ("blend_in", "blend_out"):
        blend = montage.get_editor_property(name)
        blend.set_editor_property("blend_time", BLEND_TIME)
        blend.set_editor_property("blend_option", unreal.AlphaBlendOption.HERMITE_CUBIC)
        montage.set_editor_property(name, blend)
    # Added after the slot track exists: a notify on a montage links to the segment under it.
    toolkit["set_notify"](montage, NOTIFY_TRACK, DEBRIS_FRAME / float(FPS), unreal.AnimNotify_PlayNiagaraEffect,
                          {"template": unreal.load_asset(DEBRIS[badge])})
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(package, montage_name))

    LOG.append("{}: {} keys for {} frames, sections {}, notifies {}".format(
        montage_name, toolkit["playable_key_count"](sequence), FRAMES, toolkit["montage_sections"](montage),
        [(round(time, 3), str(notify.get_editor_property("template").get_name()))
         for time, notify in toolkit["notify_events"](montage)]))
    LOG.append("  root scale per frame: " + " ".join("%.2f" % LIBRARY.get_bone_pose_for_frame(
        sequence, ROOT, frame, False).scale3d.x for frame in range(FRAMES + 1)))


try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
    for badge in DEBRIS:
        build(toolkit, badge)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
