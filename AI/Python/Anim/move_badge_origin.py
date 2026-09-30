"""Rebuilds one class badge from the generator and rig as they stand, and rewrites every clip on its skeleton so each
keeps the same motion relative to the mesh wherever the rebuild moved the badge's origin.

The origin's move is read off the rig, as the change in the rest pose of the root's children. The root's keys are
conjugated by that move, so a root turn or scale still pivots where it did on the mesh, and the root's children are
shifted by it; every bone further down is in their space and keeps its keys. Keys are read raw before the rebuild. The
logo mask is re-centred on the new frame, and slot 0's material, which the rebuild resets, is put back.

Run via mcp-unreal execute_script after changing where generate_class_badge_meshes.py frames the badge. A second run
finds no move and rewrites every clip as it was. Report written to AI/Output/badge_origin_move.json.
"""
import json

import unreal

APE = unreal.AnimPoseExtensions
MEL = unreal.MaterialEditingLibrary

BADGE = "Triangle"
FOLDER = "/Game/Characters/Meshes/Class"
MATERIAL = "/Game/Characters/Meshes/Class/Materials/MI_{}Badge".format(BADGE)
TOLERANCE = 0.01  # units a bone may land off where the old clip put it, less the move


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


def rest_pose(skeleton):
    return copy_pose(APE.get_reference_pose(skeleton), unreal.AnimPoseSpaces.LOCAL)


def badge_sequences(skeleton):
    """Every AnimSequence playing on `skeleton`."""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    return [data.get_asset() for data in registry.get_assets_by_class(
                unreal.TopLevelAssetPath("/Script/Engine", "AnimSequence"))
            if skeleton.get_path_name() in str(data.get_tag_value("Skeleton"))]


def moved_key(translation, rotation, scale, move, is_root):
    """One key once the origin moves by `move`: the root's conjugated by it, a child of the root's shifted back."""
    if is_root:
        scaled = unreal.Vector(scale.x * move.x, scale.y * move.y, scale.z * move.z)
        return translation + rotation.rotate_vector(scaled) - move, rotation, scale
    return translation - move, rotation, scale


def rewrite(sequence, keys, move, root, root_children):
    controller = sequence.get_editor_property("controller")
    controller.open_bracket("Move badge origin")
    for bone in keys[0]:
        frames = [moved_key(*frame[bone], move, bone == root) if bone == root or bone in root_children
                  else frame[bone] for frame in keys]
        controller.set_bone_track_keys(bone, [key[0] for key in frames], [key[1] for key in frames],
                                       [key[2] for key in frames])
    controller.close_bracket()
    unreal.AnimationLibrary.finalize_bone_animation(sequence)
    unreal.EditorAssetLibrary.save_loaded_asset(sequence)


def worst_miss(before, after, move, root):
    """How far any bone but the root lands from where the old clip put it less the move, in units."""
    worst = 0.0
    for old, new in zip(before, after):
        for bone, (translation, rotation, scale) in old.items():
            if bone != root:
                new_translation, new_rotation, new_scale = new[bone]
                worst = max(worst, (new_translation - (translation - move)).length(),
                            (new_scale - scale).length() * 100.0,
                            (1.0 - abs(new_rotation.x * rotation.x + new_rotation.y * rotation.y
                                       + new_rotation.z * rotation.z + new_rotation.w * rotation.w)) * 1000.0)
    return worst


def restore_material(mesh, material, generator):
    """Slot 0 back on `material`, and its logo mask centred on the badge's current frame."""
    slots = list(mesh.get_editor_property("materials"))
    # A slot edited inside the array never writes back: clone it, set the clone, reassign the array.
    slot = unreal.SkeletalMaterial()
    slot.import_text(slots[0].export_text())
    slot.set_editor_property("material_interface", material)
    mesh.set_editor_property("materials", [slot] + slots[1:])
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)

    instance = unreal.load_asset(MATERIAL)
    centre_x, centre_y, _ = generator["frame"]("SM_{}Badge".format(BADGE))
    size = MEL.get_material_instance_texture_parameter_value(instance, "Mask").blueprint_get_size_x()
    MEL.set_material_instance_vector_parameter_value(
        instance, "MaskCenterUV", unreal.LinearColor(centre_x / size, centre_y / size, 0.0, 0.0))
    MEL.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_loaded_asset(instance)


def main():
    generator = load_script("AI/Python/Mesh/generate_class_badge_meshes.py", "badge_generator")
    rig = load_script("AI/Python/Mesh/rig_class_badges.py", "badge_rig")
    mesh = unreal.load_asset("{}/SKM_{}Badge".format(FOLDER, BADGE))
    skeleton = mesh.get_editor_property("skeleton")
    modifier = unreal.SkeletonModifier()
    modifier.set_skeletal_mesh(mesh)
    root = rig["ROOT"]
    root_children = [str(bone) for bone in modifier.get_all_bone_names()
                     if str(modifier.get_parent_name(bone)) == root]

    old_rest = rest_pose(skeleton)
    material = mesh.get_editor_property("materials")[0].get_editor_property("material_interface")
    sequences = badge_sequences(skeleton)
    if not sequences:
        raise RuntimeError("no sequence found on {}".format(skeleton.get_path_name()))

    keys ={sequence: sample(sequence, unreal.AnimPoseSpaces.LOCAL) for sequence in sequences}
    placed = {sequence: sample(sequence, unreal.AnimPoseSpaces.WORLD) for sequence in sequences}

    name = "SM_{}Badge".format(BADGE)
    parts = generator["to_world"](name)
    for part in parts:
        if generator["signed_area"](part.outline) < 0.0:
            part.outline.reverse()
    generator["build"](name, parts)
    rigged = rig["rig_badge"](generator, BADGE, rig["BADGES"][BADGE])

    new_rest = rest_pose(skeleton)
    moves = [old_rest[bone][0] - new_rest[bone][0] for bone in root_children]
    move = moves[0]
    if any((other - move).length() > TOLERANCE for other in moves):
        raise RuntimeError("the root's children moved apart: {}".format([str(other) for other in moves]))

    restore_material(mesh, material, generator)
    report = {"move": [move.x, move.y, move.z], "root_children": root_children, "bones": rigged["bones"],
              "sequences": {}}
    for sequence in sequences:
        rewrite(sequence, keys[sequence], move, root, root_children)
        miss = worst_miss(placed[sequence], sample(sequence, unreal.AnimPoseSpaces.WORLD), move, root)
        report["sequences"][sequence.get_name()] = round(miss, 5)
        if miss > TOLERANCE:
            raise RuntimeError("{} lands {:.3f} off its old motion".format(sequence.get_name(), miss))
    return report


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
    with open(unreal.Paths.project_dir() + "AI/Output/badge_origin_move.json", "w") as f:
        json.dump(result, f, indent=2)
