"""Set the two material flags that decide how a translucent VFX material meets the deployable outline.

M_DeployableOutline is a post-process blendable at BL_SceneColorAfterDOF. With DOF disabled — which the
orthographic camera always is — the renderer composites post-DOF separate translucency AFTER that chain
(PostProcessing.cpp, "DOF passes were not added, therefore need to compose Separate translucency
manually"). A translucent material defaults to TranslucencyPass = After DOF, so its pixels land on top of
the finished outline. Additive blending is what makes that read as a recolor rather than a wash:
DeployableBlockingAll is (0,0,0), and additive green onto black IS green, so a blocking pillar standing in
a healing zone looked like it had taken the zone's outline color.

  TranslucencyPass = Before DOF   composites into scene color ahead of the whole post chain, so the
                                  outline draws over the VFX instead. Epic documents the property for
                                  exactly this ("avoid artifacts caused by certain post processing
                                  effects"). Costs nothing here: DOF is off, so the pass only picks order.
  AllowTranslucentCustomDepthWrites   lets the material into the custom-depth pass at all. Without it
                                  AGeoDeployableBase::ApplyOutlineStencil sets bRenderCustomDepth and a
                                  stencil value that the renderer then ignores, and the deployable has no
                                  outline of its own however its OutlineColor is authored.

Both are per-material discipline the engine does not enforce: a new VFX material defaults to After DOF
with custom depth writes off, so it silently tints every outline beneath it and shows none of its own.
Any additive or translucent material drawn where a deployable outline must stay readable belongs below.

Turning custom depth writes ON only reads correctly with the discontinuity form of the outline gate
(saturate(abs(neighbour - center)) in make_deployable_outline_material.py). Under the older outside-only
gate a stencil-writing zone SUPPRESSES the outline of anything standing inside it, so a pillar in a zone
loses its ring entirely — worse than the wrong color. Rerun that script if the graph predates the change.

Run outside PIE. Results go to the log as TRANSVFX:: lines; the audit at the end lists every other
translucent material under /Game/VFX still on After DOF, so the ones that matter get added deliberately
instead of being discovered in a screenshot. The audit only reports — it changes nothing.
"""
import unreal

# Material name -> the flags it needs. Both are spelled out per material: "under the outline" and "carries
# its own outline" are independent choices, and a decorative flash may well want neither.
MATERIALS = {
    "M_PulseCircle": {"before_dof": True, "custom_depth": True},
}

AUDIT_PATH = "/Game/VFX"
MARKER = "TRANSVFX"

# Defined by exclusion, the way the renderer's own IsTranslucentBlendMode is: the translucent family
# gains members between engine versions (5.7 alone adds AlphaHoldout and TranslucentColoredTransmittance),
# and listing them positively means a new one silently reads as opaque here.
OPAQUE = (unreal.BlendMode.BLEND_OPAQUE, unreal.BlendMode.BLEND_MASKED)
BEFORE_DOF = unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF
AFTER_DOF = unreal.MaterialTranslucencyPass.MTP_AFTER_DOF

registry = unreal.AssetRegistryHelpers.get_asset_registry()


def material_data(package_path):
    """AssetData for every Material under package_path. AssetData only — nothing is loaded here."""
    return [data for data in registry.get_assets_by_path(package_path, recursive=True)
            if str(data.asset_class_path.asset_name) == "Material"]


by_name = {}
for data in material_data("/Game"):
    by_name.setdefault(str(data.asset_name), []).append(data)

for name, flags in MATERIALS.items():
    matches = by_name.get(name, [])
    assert len(matches) == 1, \
        f"{name}: expected exactly one material, found {[str(m.package_name) for m in matches]}"

    mat = matches[0].get_asset()
    blend = mat.get_editor_property("blend_mode")
    assert blend not in OPAQUE, f"{name} is {blend.name} — neither flag has any effect on it"

    if flags["before_dof"]:
        mat.set_editor_property("TranslucencyPass", BEFORE_DOF)
    if flags["custom_depth"]:
        mat.set_editor_property("AllowTranslucentCustomDepthWrites", True)

    # A refused set is a silent no-op, and the symptom is a tinted outline with nothing pointing here.
    if flags["before_dof"]:
        assert mat.get_editor_property("TranslucencyPass") == BEFORE_DOF, \
            f"{name}: TranslucencyPass would not take"
    if flags["custom_depth"]:
        assert mat.get_editor_property("AllowTranslucentCustomDepthWrites"), \
            f"{name}: AllowTranslucentCustomDepthWrites would not take"

    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log(f"{MARKER}::set {matches[0].package_name} blend={blend.name} "
               f"beforeDOF={flags['before_dof']} customDepth={flags['custom_depth']}")

for data in material_data(AUDIT_PATH):
    if str(data.asset_name) in MATERIALS:
        continue
    mat = data.get_asset()
    blend = mat.get_editor_property("blend_mode")
    if blend not in OPAQUE and mat.get_editor_property("TranslucencyPass") == AFTER_DOF:
        unreal.log(f"{MARKER}::audit still After DOF: {data.package_name} ({blend.name}) — "
                   f"will tint any deployable outline it covers")

unreal.log(f"{MARKER}::done")
