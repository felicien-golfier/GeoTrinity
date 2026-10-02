"""Re-rigs every class badge through AI/Python/Mesh/rig_class_badges.py and rewrites every clip on each skeleton so it
plays exactly as before on the new rest pose.

A bone whose rest moves by d keeps its vertices where they were by carrying d through its own keys, and its children
take d back: each key becomes T(-d parent) * key * T(d bone). Rest moves are read in component space off the skeleton
before and after, which stands for parent space because no badge bone rests turned or scaled. Keys are read raw
before the rebuild; each badge's slot 0 material, which the rebuild resets, is put back.

The report lists, per clip, how far each bone's translation ranges, so a bone still translating while it only turns
shows up as sitting off the centre of what it carries.

Run via mcp-unreal execute_script after changing where rig_class_badges.py places bones. Report written to
AI/Output/badge_rerig.json.
"""
import json

import unreal

APE = unreal.AnimPoseExtensions

FOLDER = "/Game/Characters/Meshes/Class"
TOLERANCE = 0.01  # units a bone may land off where the old clip put it


def load_script(relative_path, name):
    """Another script's namespace, without running its main."""
    path = unreal.Paths.project_dir() + relative_path
    namespace = {"__name__": name}
    exec(compile(open(path, encoding="utf-8").read(), path, "exec"), namespace)
    return namespace


def copy_pose(pose, space):
    """{bone: (translation, rotation, scale)} copied out, since evaluated poses alias one shared buffer."""
    table = {}
    for bone in APE.get_bone_names(pose):
        transform = APE.get_bone_pose(pose, str(bone), space)
        table[str(bone)] = (unreal.Vector(transform.translation.x, transform.translation.y, transform.translation.z),
                            unreal.Quat(transform.rotation.x, transform.rotation.y, transform.rotation.z,
                                        transform.rotation.w),
                            unreal.Vector(transform.scale3d.x, transform.scale3d.y, transform.scale3d.z))
    return table


def sample(sequence, space):
    """The sequence's raw pose on every key -> [{bone: (translation, rotation, scale)}]."""
    frames = unreal.AnimationLibrary.get_num_frames(sequence)
    length = sequence.get_editor_property("sequence_length")
    options = unreal.AnimPoseEvaluationOptions()
    options.set_editor_property("evaluation_type", unreal.AnimDataEvalType.RAW)
    return [copy_pose(APE.get_anim_pose_at_time(sequence, length * frame / frames, options), space)
            for frame in range(frames + 1)]


def rest_locations(skeleton):
    return {bone: translation for bone, (translation, _, _) in
            copy_pose(APE.get_reference_pose(skeleton), unreal.AnimPoseSpaces.WORLD).items()}


def badge_sequences(skeleton):
    """Every AnimSequence playing on `skeleton`."""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    return [data.get_asset() for data in registry.get_assets_by_class(
                unreal.TopLevelAssetPath("/Script/Engine", "AnimSequence"))
            if skeleton.get_path_name() in str(data.get_tag_value("Skeleton"))]


def carried(translation, rotation, scale, offset):
    """`translation` plus `offset` taken through the key's own rotation and scale: the key times T(offset)."""
    return translation + rotation.rotate_vector(unreal.Vector(scale.x * offset.x, scale.y * offset.y,
                                                              scale.z * offset.z))


def rewrite(sequence, keys, parents, moves):
    controller = sequence.get_editor_property("controller")
    controller.open_bracket("Re-rig badge")
    for bone in keys[0]:
        parent_move = moves.get(parents[bone], unreal.Vector(0.0, 0.0, 0.0))
        frames = [(carried(*frame[bone], moves[bone]) - parent_move, frame[bone][1], frame[bone][2])
                  for frame in keys]
        controller.set_bone_track_keys(bone, [key[0] for key in frames], [key[1] for key in frames],
                                       [key[2] for key in frames])
    controller.close_bracket()
    unreal.AnimationLibrary.finalize_bone_animation(sequence)
    unreal.EditorAssetLibrary.save_loaded_asset(sequence)


def worst_miss(before, after, moves):
    """How far any bone lands from where the old clip put it once carried by its rest move, in units."""
    worst = 0.0
    for old, new in zip(before, after):
        for bone, (translation, rotation, scale) in old.items():
            new_translation, new_rotation, new_scale = new[bone]
            dot = (new_rotation.x * rotation.x + new_rotation.y * rotation.y + new_rotation.z * rotation.z
                   + new_rotation.w * rotation.w)
            worst = max(worst, (new_translation - carried(translation, rotation, scale, moves[bone])).length(),
                        (new_scale - scale).length() * 100.0, (1.0 - abs(dot)) * 1000.0)
    return worst


def translation_ranges(keys):
    """{bone: largest spread of its local translation over the clip, in units}, bones that never move left out."""
    ranges = {}
    for bone in keys[0]:
        spread = max(max(axis) - min(axis) for axis in zip(*[(frame[bone][0].x, frame[bone][0].y, frame[bone][0].z)
                                                             for frame in keys]))
        if spread > TOLERANCE:
            ranges[bone] = round(spread, 2)
    return ranges


def restore_material(mesh, material):
    slots = list(mesh.get_editor_property("materials"))
    # A slot edited inside the array never writes back: clone it, set the clone, reassign the array.
    slot = unreal.SkeletalMaterial()
    slot.import_text(slots[0].export_text())
    slot.set_editor_property("material_interface", material)
    mesh.set_editor_property("materials", [slot] + slots[1:])
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)


def rerig(generator, rig, badge):
    mesh = unreal.load_asset("{}/SKM_{}Badge".format(FOLDER, badge))
    skeleton = mesh.get_editor_property("skeleton")
    modifier = unreal.SkeletonModifier()
    modifier.set_skeletal_mesh(mesh)
    parents = {str(bone): str(modifier.get_parent_name(bone)) for bone in modifier.get_all_bone_names()}
    sequences = badge_sequences(skeleton)
    if not sequences:
        raise RuntimeError("no sequence found on {}".format(skeleton.get_path_name()))

    old_rest = rest_locations(skeleton)
    material = mesh.get_editor_property("materials")[0].get_editor_property("material_interface")
    keys = {sequence: sample(sequence, unreal.AnimPoseSpaces.LOCAL) for sequence in sequences}
    placed = {sequence: sample(sequence, unreal.AnimPoseSpaces.WORLD) for sequence in sequences}

    rigged = rig["rig_badge"](generator, badge, rig["BADGES"][badge])
    restore_material(mesh, material)
    new_rest = rest_locations(skeleton)
    moves = {bone: new_rest[bone] - old_rest[bone] for bone in parents}

    report = {"moves": {bone: [round(move.x, 3), round(move.y, 3), round(move.z, 3)] for bone, move in moves.items()},
              "bones": rigged["bones"], "sequences": {}}
    for sequence in sequences:
        rewrite(sequence, keys[sequence], parents, moves)
        miss = worst_miss(placed[sequence], sample(sequence, unreal.AnimPoseSpaces.WORLD), moves)
        if miss > TOLERANCE:
            raise RuntimeError("{} lands {:.3f} off its old motion".format(sequence.get_name(), miss))

        report["sequences"][sequence.get_name()] = {
            "miss": round(miss, 5),
            "translation_ranges": translation_ranges(sample(sequence, unreal.AnimPoseSpaces.LOCAL))}
    return report


def main():
    generator = load_script("AI/Python/Mesh/generate_class_badge_meshes.py", "badge_generator")
    rig = load_script("AI/Python/Mesh/rig_class_badges.py", "badge_rig")
    return {badge: rerig(generator, rig, badge) for badge in rig["BADGES"]}


if __name__ == "__main__":
    result = {}
    try:
        result = main()
        result["ok"] = True
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open(unreal.Paths.project_dir() + "AI/Output/badge_rerig.json", "w") as f:
        json.dump(result, f, indent=2)
