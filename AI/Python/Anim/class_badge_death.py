"""Death and revive montages for the three class badges. The death is on the beat of the old shapes' own: the badge
swells a fifth over eight frames on a smoothstep, sheds its class's debris mid-swell, and is gone on the ninth. The
montage never blends out on its own, so the badge stays gone until the revive replaces it; the debris comes from a
GeoDeathDebrisNotify, which hands it to the character so the revive clears it too. The revive is the death mirrored:
the badge pops back from nothing a fifth too large, swings short of its size and settles, then blends out to the
idle. It blends in at once, so the pop is not smeared into the death's empty pose. Each starts its sound on its first
frame, on a track of its own.

The scale is written on the root, which sits on the actor origin, so the whole badge — body and parts alike —
shrinks onto the point it turns about. Only X and Y move: under the top-down orthographic camera Z is spent for
nothing. The montages play in DefaultSlot, which the badge AnimBlueprints put over both layers, so a death takes the
whole badge whatever its top and bottom layers are doing, and a revive playing in the same slot group stops the death.

Run AFTER AI/Python/Mesh/rig_class_badges.py, via mcp-unreal execute_script. Re-runnable: rewrites every sequence and
montage in place. Report written to AI/Output/class_badge_death.txt.
"""
import unreal

LIBRARY = unreal.AnimationLibrary

MESH_FOLDER = "/Game/Characters/Meshes/Class"
ANIM_FOLDER = "/Game/Characters/Anim/ClassBadge"
REPORT = (unreal.Paths.project_dir() + "AI/Output/") + "class_badge_death.txt"

ROOT = "Root"
SLOT = "DefaultSlot"
SECTION = "Default"
NOTIFY_TRACK = "1"
SOUND_TRACK = "2"

FPS = 30
BLEND_TIME = 0.25

DEATH_FRAMES = 15    # half a second
SWELL_FRAMES = 8     # frames of swell before the badge goes
SWELL = 1.2
DEBRIS_FRAME = 4     # mid-swell, so the chunks lead the collapse rather than trail it
DEATH_SOUND = "/Game/Art/SFX/Death/SW_Death_Collapse"  # Earthquake_3 with its head cut, so the rumble peaks on the pop
DEATH_SOUND_VOLUME = 1.0

REVIVE_FRAMES = 12
# (frame, scale) keys, smoothstepped between: the pop, the swing short, the settle
REVIVE_KEYS = [(0, 0.0), (2, SWELL), (5, 0.94), (8, 1.02), (10, 1.0)]
REVIVE_SOUND = "/Game/Art/SFX/Bank/magicspellssfx1/Heal/Heal_9"
REVIVE_SOUND_VOLUME = 1.0

DEBRIS = {
    "Square": "/Game/Art/VFX/Assets/NS_DeathDebris",
    "Triangle": "/Game/Art/VFX/Assets/NS_DeathDebrisTriangle",
    "Circle": "/Game/Art/VFX/Assets/NS_DeathDebrisCircle",
}

LOG = []


def smoothstep(alpha):
    return alpha * alpha * (3.0 - 2.0 * alpha)


def death_scale_at(frame):
    """The badge's XY scale on `frame` of its death: a smoothstep swell, then gone."""
    if frame > SWELL_FRAMES:
        return 0.0
    return 1.0 + (SWELL - 1.0) * smoothstep(frame / float(SWELL_FRAMES))


def revive_scale_at(frame):
    """The badge's XY scale on `frame` of its revive, through REVIVE_KEYS, then held at rest."""
    for (start, low), (end, high) in zip(REVIVE_KEYS, REVIVE_KEYS[1:]):
        if frame <= end:
            return low + (high - low) * smoothstep((frame - start) / float(end - start))
    return REVIVE_KEYS[-1][1]


def root_scaler(scale_at):
    def key(frame, bone, rest_local):
        scale = rest_local.scale3d
        factor = scale_at(frame) if bone == ROOT else 1.0
        return rest_local.translation, rest_local.rotation, unreal.Vector(scale.x * factor, scale.y * factor, scale.z)
    return key


def build(toolkit, badge, clip, frames, scale_at, blend_in_time, auto_blend_out, sound, sound_volume):
    """Build the badge's `clip` sequence and montage, its sound on frame 0; returns the montage."""
    skeleton_path = "{}/SK_{}Badge".format(MESH_FOLDER, badge)
    package = "{}/{}".format(ANIM_FOLDER, badge)
    sequence_name = "SK_{}Badge_Sequence_{}".format(badge, clip)
    montage_name = "SK_{}Badge_Montage_{}".format(badge, clip)

    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(skeleton_path))
    sequence = toolkit["get_or_create_asset"](package, sequence_name, unreal.AnimSequence, factory)
    toolkit["write_bone_tracks"](sequence, skeleton_path, FPS, frames, root_scaler(scale_at),
                                 "Build {} badge {}".format(badge, clip))

    montage = toolkit["build_montage"](sequence, package, montage_name, [SECTION], [0.0], ["None"], SLOT)
    for name, time in (("blend_in", blend_in_time), ("blend_out", BLEND_TIME)):
        blend = montage.get_editor_property(name)
        blend.set_editor_property("blend_time", time)
        blend.set_editor_property("blend_option", unreal.AlphaBlendOption.HERMITE_CUBIC)
        montage.set_editor_property(name, blend)
    montage.set_editor_property("enable_auto_blend_out", auto_blend_out)
    # Added after the slot track exists: a notify on a montage links to the segment under it.
    toolkit["set_notify"](montage, SOUND_TRACK, 0.0, unreal.AnimNotify_PlaySound,
                          {"sound": unreal.load_asset(sound), "volume_multiplier": sound_volume})
    return montage, sequence


def save(asset):
    unreal.EditorAssetLibrary.save_asset(asset.get_outermost().get_name(), only_if_is_dirty=False)


def report(toolkit, montage, sequence, frames):
    LOG.append("{}: {} keys for {} frames, sections {}, blend in {}, auto blend out {}, notifies {}".format(
        montage.get_name(), toolkit["playable_key_count"](sequence), frames, toolkit["montage_sections"](montage),
        montage.get_editor_property("blend_in").get_editor_property("blend_time"),
        montage.get_editor_property("enable_auto_blend_out"),
        [(round(time, 3), notify.get_class().get_name()) for time, notify in toolkit["notify_events"](montage)]))
    LOG.append("  root scale per frame: " + " ".join("%.2f" % LIBRARY.get_bone_pose_for_frame(
        sequence, ROOT, frame, False).scale3d.x for frame in range(frames + 1)))


def build_death(toolkit, badge):
    montage, sequence = build(toolkit, badge, "Death", DEATH_FRAMES, death_scale_at, BLEND_TIME, False, DEATH_SOUND,
                              DEATH_SOUND_VOLUME)
    toolkit["set_notify"](montage, NOTIFY_TRACK, DEBRIS_FRAME / float(FPS), unreal.GeoDeathDebrisNotify,
                          {"template": unreal.load_asset(DEBRIS[badge])}, clear=False)
    save(montage)
    report(toolkit, montage, sequence, DEATH_FRAMES)


def build_revive(toolkit, badge):
    montage, sequence = build(toolkit, badge, "Revive", REVIVE_FRAMES, revive_scale_at, 0.0, True, REVIVE_SOUND,
                              REVIVE_SOUND_VOLUME)
    save(montage)
    save(sequence)
    report(toolkit, montage, sequence, REVIVE_FRAMES)


try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
    for badge in DEBRIS:
        build_death(toolkit, badge)
        build_revive(toolkit, badge)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
