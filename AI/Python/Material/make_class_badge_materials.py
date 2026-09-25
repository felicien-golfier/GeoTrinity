"""Class badge body material: the class colour with the logo's line art on both caps, one instance per class.

The badges carry no UVs. The mask is projected from the pre-skinned local position instead, through the same pixel
frame generate_class_badge_meshes.py extruded the badge from, so the lines sit on the geometry and ride each bone.
The projection runs straight through along Z, so the bottom cap shows the lines mirrored, as its outline is.
Pre-skinned data is vertex-shader only: the mask UV and the cap-face flag cross to the pixel shader through one
vertex interpolator, exact on the flat caps. The shield-burst gauge glow of MAT_Cube_Alive is carried over as is.

Points DA_PlayerClassData's AliveMaterial and each SKM_<Class>Badge slot 0 at the class's instance. An existing
instance keeps its tuned values; only its texture and mask frame are set again. Run through execute_script, outside
PIE. Report written to Saved/class_badge_materials.txt.
"""
import unreal

FOLDER = "/Game/Characters/Meshes/Class/Materials"
MESH_FOLDER = "/Game/Characters/Meshes/Class"
CLASS_DATA = "/Game/Characters/Playable/DA_PlayerClassData"
MASK_FOLDER = unreal.Paths.project_dir() + "GamePictures/"
REPORT = unreal.Paths.project_saved_dir() + "class_badge_materials.txt"
CAP_FACE_MIN_NORMAL_Z = 0.5  # the needle's sloped faces count as caps

# Badge -> (class, mask file, colour, line colour). Colours are those of the class's previous alive material.
BADGES = {
    "Square": (unreal.PlayerClass.SQUARE, "tank_NB.png",
               unreal.LinearColor(0.0, 0.0, 1.0, 1.0), unreal.LinearColor(0.6, 0.6, 1.0, 1.0)),
    "Triangle": (unreal.PlayerClass.TRIANGLE, "damage_NB.png",
                 unreal.LinearColor(1.0, 1.0, 0.0, 1.0), unreal.LinearColor(1.0, 1.0, 0.6, 1.0)),
    "Circle": (unreal.PlayerClass.CIRCLE, "heal_NB.png",
               unreal.LinearColor(0.0, 1.0, 0.0, 1.0), unreal.LinearColor(0.6, 1.0, 0.6, 1.0)),
}

toolkit_path = unreal.Paths.project_dir() + "AI/Python/Material/material_graph_authoring.py"
toolkit = {}
exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
mel = toolkit["mel"]
load = toolkit["load"]
save = toolkit["save"]

importer_path = unreal.Paths.project_dir() + "AI/Python/Asset/import_textures.py"
importer = {"__name__": "import_textures"}
exec(compile(open(importer_path).read(), importer_path, "exec"), importer)

mesh_path = unreal.Paths.project_dir() + "AI/Python/Mesh/generate_class_badge_meshes.py"
mesh_script = {"__name__": "class_badge_meshes"}
exec(compile(open(mesh_path).read(), mesh_path, "exec"), mesh_script)

LOG = []


def build_material(default_mask):
    graph = toolkit["open_material"](FOLDER, "M_ClassBadge")
    node, op, connect, parameter = graph.node, graph.op, graph.connect, graph.parameter
    Vector, Scalar = unreal.MaterialExpressionVectorParameter, unreal.MaterialExpressionScalarParameter

    # Vertex stage: pre-skinned position -> mask UV, pre-skinned normal -> top-face flag.
    position = node(unreal.MaterialExpressionLocalPosition, -2000, 0,
                    local_origin=unreal.LocalPositionOrigin.INSTANCE_PRE_SKINNING)
    # Image right is +Y, image down is -X.
    minus_x = node(unreal.MaterialExpressionMultiply, -1600, 60, const_b=-1.0)
    connect(graph.mask(position, "r", -1800, 60), minus_x)
    image_direction = op(unreal.MaterialExpressionAppendVector, -1400, 0,
                         A=graph.mask(position, "g", -1800, -60), B=minus_x)
    uv_per_cm = parameter(Scalar, "MaskUVPerCm", 0.01, "02 Mask", 2,
                          "Mask UV per cm of badge; set by make_class_badge_materials.py from the mesh frame",
                          -1600, 200)
    center = parameter(Vector, "MaskCenterUV", unreal.LinearColor(0.5, 0.5, 0.0, 0.0), "02 Mask", 3,
                       "Mask UV at the badge's origin, in RG; set by make_class_badge_materials.py", -1600, 320)
    uv = op(unreal.MaterialExpressionAdd, -1000, 0,
            op(unreal.MaterialExpressionMultiply, -1200, 0, image_direction, B=uv_per_cm),
            B=graph.mask(center, "rg", -1200, 320))
    normal_z = graph.mask(node(unreal.MaterialExpressionPreSkinnedNormal, -1400, 480), "b", -1300, 480)
    cap_face = op(unreal.MaterialExpressionStep, -1000, 480, X=op(unreal.MaterialExpressionAbs, -1150, 480, normal_z))
    cap_face.set_editor_property("const_y", CAP_FACE_MIN_NORMAL_Z)
    to_pixel = op(unreal.MaterialExpressionVertexInterpolator, -600, 200,
                  op(unreal.MaterialExpressionAppendVector, -800, 200, A=uv, B=cap_face))

    # Pixel stage: the mask's lines, transparent pixels dropped, top and bottom faces only.
    sample = node(unreal.MaterialExpressionTextureSampleParameter2D, -300, 100, parameter_name="Mask",
                  texture=default_mask, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR,
                  group="02 Mask", sort_priority=0, desc="White lines on the badge; alpha 0 draws nothing")
    connect(graph.mask(to_pixel, "rg", -450, 100), sample, "UVs")
    line = op(unreal.MaterialExpressionMultiply, 100, 200,
              op(unreal.MaterialExpressionMultiply, -50, 150, (sample, "R"), B=(sample, "A")),
              B=graph.mask(to_pixel, "b", -450, 300))
    color = parameter(Vector, "Color", unreal.LinearColor(0.0, 0.0, 1.0, 1.0), "01 Body", 0,
                      "Class colour, emissive", 100, 0)
    line_color = parameter(Vector, "LineColor", unreal.LinearColor(0.6, 0.6, 1.0, 1.0), "02 Mask", 1,
                           "Colour of the mask's lines, emissive", 100, 100)
    body = op(unreal.MaterialExpressionLinearInterpolate, 350, 100, A=color, B=line_color, Alpha=line)

    # Shield-burst gauge glow, as in MAT_Cube_Alive: brightest at the UV border, filling in with FullGlowGauge.
    uvs = node(unreal.MaterialExpressionTextureCoordinate, -1200, 800)
    border = []
    for index, channel in enumerate("rg"):
        coordinate = graph.mask(uvs, channel, -1000, 700 + 200 * index)
        opposite = node(unreal.MaterialExpressionSubtract, -800, 760 + 200 * index, const_a=1.0)
        connect(coordinate, opposite, "B")
        border.append(op(unreal.MaterialExpressionMin, -600, 700 + 200 * index, coordinate, B=opposite))
    full = parameter(Scalar, "FullGlowGauge", 0.0, "03 Shield Gauge", 1, "0-1, widens the glow to the whole face",
                     -800, 1100)
    reach = op(unreal.MaterialExpressionLinearInterpolate, -600, 1100, Alpha=full)
    reach.set_editor_property("const_a", 0.1)
    glow_shape = op(unreal.MaterialExpressionOneMinus, -100, 800,
                    op(unreal.MaterialExpressionSaturate, -250, 800,
                       op(unreal.MaterialExpressionDivide, -400, 800,
                          op(unreal.MaterialExpressionMin, -450, 750, border[0], B=border[1]), B=reach)))
    gauge_color = parameter(Vector, "GlowGaugeColor", unreal.LinearColor(0.0, 0.876863, 1.0, 1.0), "03 Shield Gauge",
                            2, "Gauge glow colour", -100, 650)
    gauge = parameter(Scalar, "GlowGauge", 0.0, "03 Shield Gauge", 0, "0-1, gauge fill; written by the shield burst",
                      100, 950)
    glow = op(unreal.MaterialExpressionMultiply, 350, 800,
              op(unreal.MaterialExpressionMultiply, 150, 800, gauge_color, B=glow_shape), B=gauge)
    graph.to_property(op(unreal.MaterialExpressionAdd, 600, 300, glow, B=body),
                      unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    material = graph.asset
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("used_with_skeletal_mesh", True)
    graph.finish()
    stats = mel.get_statistics(material)
    LOG.append(f"M_ClassBadge: {mel.get_num_material_expressions(material)} nodes, vertex "
               f"{stats.num_vertex_shader_instructions} / pixel {stats.num_pixel_shader_instructions} instructions")
    assert stats.num_pixel_shader_instructions > 0, "M_ClassBadge did not compile, see the material editor"
    return material


def build_instance(badge, parent, texture, color, line_color):
    center_x, center_y, cm_per_pixel = mesh_script["frame"](f"SM_{badge}Badge")
    size = texture.blueprint_get_size_x()
    assert size == texture.blueprint_get_size_y(), f"{texture.get_name()}: mask must be square"
    name = f"MI_{badge}Badge"
    is_new = not unreal.EditorAssetLibrary.does_asset_exist(f"{FOLDER}/{name}")
    instance = toolkit["load_or_create"](FOLDER, name, unreal.MaterialInstanceConstant,
                                         unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(instance, parent)
    if is_new:
        mel.set_material_instance_vector_parameter_value(instance, "Color", color)
        mel.set_material_instance_vector_parameter_value(instance, "LineColor", line_color)

    mel.set_material_instance_texture_parameter_value(instance, "Mask", texture)
    mel.set_material_instance_vector_parameter_value(
        instance, "MaskCenterUV", unreal.LinearColor(center_x / size, center_y / size, 0.0, 0.0))
    mel.set_material_instance_scalar_parameter_value(instance, "MaskUVPerCm", 1.0 / (cm_per_pixel * size))
    mel.update_material_instance(instance)
    save(instance)
    LOG.append(f"{name}: centre px ({center_x}, {center_y}), {cm_per_pixel:.4f} cm per px, mask {size} px")
    return instance


def wire(instances):
    """Each class's AliveMaterial, and its badge's slot 0 for the editor's previews."""
    data_asset = load(CLASS_DATA)
    rebuilt = {}
    for player_class, entry in data_asset.get_editor_property("ClassData").items():
        # EditDefaultsOnly fields refuse the setter even on a copy; exported text merges in.
        clone = unreal.PlayerClassData()
        clone.import_text(entry.export_text())
        if player_class in instances:
            instance = instances[player_class]
            clone.import_text(f'(AliveMaterial="{instance.get_class().get_path_name()}\'{instance.get_path_name()}\'")')

        rebuilt[player_class] = clone

    data_asset.set_editor_property("ClassData", rebuilt)
    save(data_asset)
    for player_class, entry in load(CLASS_DATA).get_editor_property("ClassData").items():
        LOG.append(f"class data {player_class}: alive material {entry.get_editor_property('alive_material').get_name()}")

    for badge, (player_class, _, _, _) in BADGES.items():
        mesh = load(f"{MESH_FOLDER}/SKM_{badge}Badge")
        slots = list(mesh.get_editor_property("materials"))
        # A slot edited inside the array never writes back: clone it, set the clone, reassign the array.
        slot = unreal.SkeletalMaterial()
        slot.import_text(slots[0].export_text())
        slot.set_editor_property("material_interface", instances[player_class])
        mesh.set_editor_property("materials", [slot] + slots[1:])
        save(mesh)
        previous = slots[0].get_editor_property("material_interface")  # empty after the rig rebuilds the mesh
        LOG.append(f"{mesh.get_name()} slot 0: {previous.get_name() if previous else 'empty'} -> "
                   f"{instances[player_class].get_name()}")


try:
    textures = {badge: importer["import_texture"](MASK_FOLDER + file_name, FOLDER, f"T_{badge}Badge_Mask", srgb=False)
                for badge, (_, file_name, _, _) in BADGES.items()}
    parent = build_material(textures["Square"])
    wire({player_class: build_instance(badge, parent, textures[badge], color, line_color)
          for badge, (player_class, _, color, line_color) in BADGES.items()})
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
