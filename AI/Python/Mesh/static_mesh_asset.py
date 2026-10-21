"""
Toolkit for every script that writes a static mesh asset: generators (save_static_mesh) and scripts that only edit
one, such as its material slots (load_mesh, then save_mesh).

Always the full build (fast_build False). A fast build saves bDoFastBuild on the asset for good; whenever the mesh is
rebuilt without a derived-data cache hit (an edit, a cold cache) it takes the engine's fast path, which never sets the
bounds: NaN / garbage bounds, meshes culled, ensures (TroobleshootingLogs.md, "Gem meshes with NaN bounds"). Python
cannot write the flag: load_mesh clears it through unreal.GeoMeshBuilderUtil.clear_fast_build (GeoTrinityEditor).

Every edit waits out the mesh's async compile first (rebuilding mid-compile is a Fatal). The bounds are checked
against the mesh description's vertices before saving: a wrong mesh stops the script.

Load with exec, as the material toolkit: see generate_gem_meshes.py.
"""
import math

import unreal

BOUNDS_TOLERANCE = 0.01


def wait_for_compile(asset):
    """Finishes the mesh's async compile: replacing its render data mid-compile is a Fatal (FRenderResource deleted
    without being released). Every edit that rebuilds the mesh starts a new compile."""
    asset.get_num_vertices(0)


def description_bounds(desc):
    """Centre and half extent of the description's vertices."""
    positions = [desc.get_vertex_position(unreal.VertexID(i)) for i in range(desc.get_vertex_count())]
    low = [min(getattr(p, axis) for p in positions) for axis in "xyz"]
    high = [max(getattr(p, axis) for p in positions) for axis in "xyz"]
    return [(a + b) * 0.5 for a, b in zip(low, high)], [(b - a) * 0.5 for a, b in zip(low, high)]


def check_bounds(asset, desc):
    """Stops the script when the built bounds differ from desc's vertices."""
    bounds = asset.get_bounds()
    built = [bounds.origin.x, bounds.origin.y, bounds.origin.z, bounds.box_extent.x, bounds.box_extent.y,
             bounds.box_extent.z]
    centre, extent = description_bounds(desc)
    expected = centre + extent
    if not all(math.isfinite(v) and abs(v - e) <= BOUNDS_TOLERANCE for v, e in zip(built, expected)):
        raise RuntimeError(f"{asset.get_path_name()}: built bounds {built}, the vertices give {expected}")


def load_mesh(package):
    """The mesh at package, compiled and cleared of the fast-build flag, ready for an edit."""
    asset = unreal.load_asset(package)
    assert asset, f"{package}: missing"
    wait_for_compile(asset)
    unreal.GeoMeshBuilderUtil.clear_fast_build(asset)
    wait_for_compile(asset)
    return asset


def save_mesh(asset):
    """Saves the edited mesh once compiled and its bounds checked against its own mesh description."""
    wait_for_compile(asset)
    check_bounds(asset, asset.get_static_mesh_description(0))
    unreal.EditorAssetLibrary.save_loaded_asset(asset)


def save_static_mesh(folder, name, desc, materials):
    """Build desc into folder/name (created, or rewritten in place) with materials (StaticMaterial list), then save."""
    package = f"{folder}/{name}"
    # Rewritten in place: deleting a loaded asset leaves its package unloadable for the whole session.
    if unreal.EditorAssetLibrary.does_asset_exist(package):
        asset = load_mesh(package)
    else:
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.StaticMesh, None)

    asset.build_from_static_mesh_descriptions([desc], False, False)
    wait_for_compile(asset)
    asset.set_editor_property("static_materials", materials)
    save_mesh(asset)
    return asset
