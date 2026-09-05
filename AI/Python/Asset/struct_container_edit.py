"""Generic helpers for rewriting a struct array or a struct map on an asset or a Blueprint CDO.

A struct read out of a container is a by-value copy, and EditDefaultsOnly fields on it refuse the property setter,
so an edit rebuilds each element by round-tripping its exported text into a fresh struct and merging overrides — a
single-field import sets only that field and preserves the rest. The whole rebuilt container is then reassigned,
since the copies never wrote back, and the asset is saved unconditionally: a container edit can leave the package
clean, and a dirty-only save then skips it.

Run via mcp-unreal execute_script. Adjust the example call at the bottom for the asset you are working on.
"""
import unreal


def clone_with(struct, struct_type, overrides):
    """A fresh `struct_type` holding `struct`'s values, with `overrides` ({field: exported literal}) merged in."""
    clone = struct_type()
    clone.import_text(struct.export_text())
    for field, literal in overrides.items():
        clone.import_text("({}={})".format(field, literal))
    return clone


def object_literal(asset):
    """An object reference as exported text, which is the only form an import merges in."""
    return '"{}\'{}\'"'.format(asset.get_class().get_path_name(), asset.get_path_name())


def rebuild_array(owner, property_name, struct_type, overrides_for):
    """Rewrite a struct array in place, `overrides_for(index, element)` giving each element's overrides."""
    owner.set_editor_property(property_name, [
        clone_with(element, struct_type, overrides_for(index, element))
        for index, element in enumerate(owner.get_editor_property(property_name))])


def rebuild_map(owner, property_name, struct_type, overrides_for):
    """Rewrite a struct map in place, `overrides_for(key, value)` giving each entry's overrides."""
    table = owner.get_editor_property(property_name)
    owner.set_editor_property(property_name, {
        key: clone_with(table[key], struct_type, overrides_for(key, table[key])) for key in table})


# --- Example call — uncomment and adjust -------------------------------------------------------

# Point one entry of a Blueprint's class table at an asset, leaving every other entry untouched
# BLUEPRINT = unreal.load_asset("/Game/Characters/Playable/BP_GeoPlayableCharacter")
# MONTAGE = unreal.load_asset("/Game/Characters/Anim/Cone/SK_ConeDeath_Montage")
# rebuild_map(unreal.get_default_object(BLUEPRINT.generated_class()), "class_data", unreal.PlayerClassData,
#             lambda key, value: {"DeathMontage": object_literal(MONTAGE)}
#             if key == unreal.PlayerClass.TRIANGLE else {})
# unreal.EditorAssetLibrary.save_loaded_asset(BLUEPRINT, only_if_is_dirty=False)
