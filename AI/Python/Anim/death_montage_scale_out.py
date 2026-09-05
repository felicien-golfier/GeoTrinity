"""Death montages for the Circle and the Triangle, cut to the same beat as the Square's.

The Square's clip is one gesture: the shape swells a fifth over eight frames on a smoothstep, sheds its debris
mid-swell, and is gone on the ninth — straight to zero, held for the rest of the half second. Only X and Y move.
Under the top-down orthographic camera the Z extent is what a shape is drawn through rather than what it is drawn
as, so scaling it is spent for nothing.

The scale is written on the root alone and reaches the whole mesh through it: every other bone descends from it, so
a non-uniform root scale carries down into their offsets and their scales alike, and a vertex blended across
several of them still lands exactly where one scaled transform would put it.

What the root's own translation decides is where the shrink converges. The Square and the Circle sit their root on
the axis, so scaling it in place already converges on the actor origin; the Cone's root sits behind its base, and
scaling it in place would drag the triangle backwards as it went. Scaling the root's translation by the same factor
is the one rule that converges all three on the actor origin — the point each shape turns about, and so the point
it reads as centred on.

Each montage plays its whole sequence as a single section that ends there, with the class's debris Niagara on a
notify at the frame the Square sheds its own. Both are then wired into BP_GeoPlayableCharacter's class table.

Run via mcp-unreal execute_script. Re-runnable: rewrites both sequences, both montages and the class table in place.
Report written to REPORT.
"""
import unreal

APE = unreal.AnimPoseExtensions
LIBRARY = unreal.AnimationLibrary

REPORT = ("C:/Users/Felou/AppData/Local/Temp/claude/C--GeoTrinity/"
          "67a34ba2-4901-4340-b56a-c4bce0ed0e3d/scratchpad/death_montage_scale_out.txt")

CHARACTER_BP = "/Game/Characters/Playable/BP_GeoPlayableCharacter"
ROOT_BONE = "Root"
SLOT = "DefaultSlot"
SECTION = "Default"
NOTIFY_TRACK = "1"

FPS = 30
FRAMES = 15          # half a second, the length of the Square's clip
SWELL_FRAMES = 8     # frames of swell before the shape goes
SWELL = 1.2
DEBRIS_FRAME = 4     # mid-swell, so the chunks lead the collapse rather than trail it
BLEND_TIME = 0.25

# (skeleton, anim folder, sequence name, montage name, debris system, the class that plays it)
CLIPS = [
    ("/Game/Characters/Meshes/Cylinder/SK_Cylinder", "/Game/Characters/Anim/Cylinder",
     "SK_Cylinder_Sequence_Death", "SK_CylinderDeath_Montage",
     "/Game/Art/VFX/Assets/NS_DeathDebrisCircle", unreal.PlayerClass.CIRCLE),
    ("/Game/Characters/Meshes/Cone/SK_Cone", "/Game/Characters/Anim/Cone",
     "SK_Cone_Sequence_Death", "SK_ConeDeath_Montage",
     "/Game/Art/VFX/Assets/NS_DeathDebrisTriangle", unreal.PlayerClass.TRIANGLE),
]

lines = []


def report(*values):
    lines.append(" ".join(str(value) for value in values))


def scale_at(frame):
    """The shape's XY scale on `frame`: a smoothstep swell, then gone."""
    if frame > SWELL_FRAMES:
        return 0.0
    alpha = frame / float(SWELL_FRAMES)
    return 1.0 + (SWELL - 1.0) * alpha * alpha * (3.0 - 2.0 * alpha)


def get_or_create(package_path, asset_name, asset_class, factory):
    """Load the asset if it is there, else create it.

    Loaded rather than looked up in the asset registry: a registry still scanning reports an asset that is on disk
    as missing, and creating over it then fails and hands back nothing. Never deletes: deleting a loaded asset
    leaves the package unloadable for the rest of the editor session.
    """
    asset = unreal.load_asset("{}/{}".format(package_path, asset_name))
    return asset or unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, asset_class, factory)


def write_sequence(skeleton_path, package_path, asset_name):
    """Write the scale-out onto every bone of the skeleton and return the finished sequence."""
    skeleton = unreal.load_asset(skeleton_path)
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", skeleton)
    sequence = get_or_create(package_path, asset_name, unreal.AnimSequence, factory)

    reference = APE.get_reference_pose(skeleton)
    rest = {str(bone): APE.get_bone_pose(reference, str(bone), unreal.AnimPoseSpaces.LOCAL)
            for bone in APE.get_bone_names(reference)}

    controller = sequence.get_editor_property("controller")
    controller.open_bracket("Death scale-out")
    controller.set_frame_rate(unreal.FrameRate(FPS, 1))
    controller.set_number_of_frames(unreal.FrameNumber(FRAMES))
    for bone, local in rest.items():
        rotation = local.rotation.rotator().quaternion()
        positions, rotations, scales = [], [], []
        for frame in range(FRAMES + 1):  # a sequence holds one more key than its frame count
            scale = scale_at(frame) if bone == ROOT_BONE else 1.0
            positions.append(unreal.Vector(local.translation.x * scale, local.translation.y * scale,
                                           local.translation.z))
            rotations.append(rotation)
            scales.append(unreal.Vector(scale, scale, local.scale3d.z))
        # Unconditional and result ignored: the adder reports failure for an existing track, and the key setter
        # reports success whether or not a track is there.
        controller.add_bone_curve(bone)
        controller.set_bone_track_keys(bone, positions, rotations, scales)
    controller.close_bracket()
    # The keys above land in the raw model; this builds the data that actually plays back from them.
    LIBRARY.finalize_bone_animation(sequence)
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(package_path, asset_name))
    return sequence


def build_montage(sequence, package_path, asset_name, debris_path):
    """Create or rewrite the montage that plays the whole sequence, debris notify included."""
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("target_skeleton", sequence.get_skeleton())
    factory.set_editor_property("source_animation", sequence)
    montage = get_or_create(package_path, asset_name, unreal.AnimMontage, factory)

    util = unreal.get_default_object(unreal.GeoAnimBuilderUtil)
    util.set_montage_slot_segment(montage, sequence, SLOT)
    util.set_montage_sections(montage, [unreal.Name(SECTION)], [0.0], [unreal.Name("None")])
    for blend_name in ("blend_in", "blend_out"):
        blend = montage.get_editor_property(blend_name)
        blend.set_editor_property("blend_time", BLEND_TIME)
        blend.set_editor_property("blend_option", unreal.AlphaBlendOption.HERMITE_CUBIC)
        montage.set_editor_property(blend_name, blend)

    # Added after the slot track exists: a notify on a montage links to the segment under it.
    LIBRARY.remove_all_animation_notify_tracks(montage)
    LIBRARY.add_animation_notify_track(montage, NOTIFY_TRACK)
    notify = LIBRARY.add_animation_notify_event(montage, NOTIFY_TRACK, DEBRIS_FRAME / float(FPS),
                                                unreal.AnimNotify_PlayNiagaraEffect)
    notify.set_editor_property("template", unreal.load_asset(debris_path))
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(package_path, asset_name))
    return montage


def wire_class_data(montages):
    """Point each class's DeathMontage at the montage built for it."""
    blueprint = unreal.load_asset(CHARACTER_BP)
    cdo = unreal.get_default_object(blueprint.generated_class())
    table = cdo.get_editor_property("class_data")

    rebuilt = {}
    for player_class in table:
        # EditDefaultsOnly struct fields refuse the property setter even on a copy, so each entry is cloned
        # through its exported text; a single-field import merges and preserves the rest.
        entry = unreal.PlayerClassData()
        entry.import_text(table[player_class].export_text())
        montage = montages.get(player_class)
        if montage:
            entry.import_text('(DeathMontage="{}\'{}\'")'.format(
                montage.get_class().get_path_name(), montage.get_path_name()))
        rebuilt[player_class] = entry
    cdo.set_editor_property("class_data", rebuilt)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)

    for player_class, entry in rebuilt.items():
        report("%-24s DeathMontage = %s" % (player_class, entry.get_editor_property("death_montage")))


def verify(sequence):
    """Read the built data back: the scale that plays, and where the shrink converges.

    Pose evaluation reads the raw model and reports the same whether or not the build happened, so this samples the
    animation library instead. The convergence point is the root's translation with its own scaling undone — the
    actor origin, so (0, 0, 0) on every frame whatever the rig hangs its root off.
    """
    reference = APE.get_bone_pose(APE.get_reference_pose(sequence.get_skeleton()), ROOT_BONE,
                                  unreal.AnimPoseSpaces.LOCAL).translation
    report("  keys %d for %d frames" % (sequence.get_editor_property("number_of_sampled_keys"), FRAMES))
    for frame in range(FRAMES + 1):
        root = LIBRARY.get_bone_pose_for_frame(sequence, ROOT_BONE, frame, False)
        report("  f%02d scale %.4f  converges on (%.2f, %.2f)" % (
            frame, root.scale3d.x,
            root.translation.x - root.scale3d.x * reference.x,
            root.translation.y - root.scale3d.y * reference.y))


montages = {}
for skeleton_path, package_path, sequence_name, montage_name, debris_path, player_class in CLIPS:
    sequence = write_sequence(skeleton_path, package_path, sequence_name)
    montage = build_montage(sequence, package_path, montage_name, debris_path)
    montages[player_class] = montage

    report("=== %s -> %s" % (sequence_name, montage_name))
    report("  montage %.3fs  sections %s" % (
        montage.get_editor_property("sequence_length"),
        [str(montage.get_section_name(index)) for index in range(montage.get_num_sections())]))
    for event in LIBRARY.get_animation_notify_events(montage):
        report("  notify %.4fs  %s" % (LIBRARY.get_anim_notify_event_trigger_time(event),
                                       event.get_editor_property("notify").get_editor_property("template")))
    verify(sequence)

wire_class_data(montages)

with open(REPORT, "w", encoding="utf-8") as handle:
    handle.write("\n".join(lines))
