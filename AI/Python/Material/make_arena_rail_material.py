"""Arena border rail, in two passes on each wall's top face.

Each wall's line runs down its middle and stops EndInset half widths before each end, where a wall meeting it at a
corner has its own line, so the lines meet rather than cross.
The wall's own material, M_ArenaRailHalo, is masked: a halo of the rail colour whose dithered coverage fades from
full on the line to none half a wall width away, rounding off past the line's ends, so what lies under the wall shows
through. Each pixel is pushed down by its distance from the line: where walls overlap, the depth test keeps the wall
whose line is nearer, so overlapping halos join as one shape instead of adding up.
Its overlay material, M_ArenaRail, is additive: a near-white core line and small quads riding it, hollow and solid in
turn, travelling and turning, each shown whole while its centre is on the line. Those do add up where walls cross.

Everything is measured in world cm, so the rail keeps its size while a barrier scales a wall in or out. The quads
travel along the wall's local +X (AI/ArenaWalls.md, "Orientation"), and are placed from the world origin, so two walls
on one line draw the same quads. One overlay instance, MI_ArenaRail, serves every wall.

Puts each placed wall on the engine's VR frame material, or already on the rail, onto the halo and the overlay, and
deletes the per-arena instances MI_ArenaRail_* it replaces. The level is left dirty, never saved. Run through
execute_script, outside PIE. Report written to AI/Output/arena_rail_material.txt.
"""
import unreal

FOLDER = "/Game/Art/VFX/Arena"
FUNCTION_FOLDER = FOLDER + "/Functions"
GENERIC = "/Game/Art/VFX/Generic/Materials/Functions"
DITHER = "/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA"
CATEGORY = "GeoTrinity|Arena"
REPORT = (unreal.Paths.project_dir() + "AI/Output/") + "arena_rail_material.txt"
OLD_WALL_MATERIAL = "/Engine/VREditor/UI/FrameMaterial.FrameMaterial"
HALF_CUBE = 50.0  # the engine cube spans -50..50 cm on each local axis
HALF_SQRT2 = 0.7071067811865476
RAIL_COLOR = (0.45, 0.2, 1.0, 1.0)

OLD_OVERLAYS = ["Star", "Hex", "Tutorial", "Entrance"]  # the per-arena instances this script replaces

toolkit_path = unreal.Paths.project_dir() + "AI/Python/Material/material_graph_authoring.py"
toolkit = {}
exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
mel = toolkit["mel"]
load = toolkit["load"]
save = toolkit["save"]

scalar = unreal.MaterialExpressionScalarParameter
vector = unreal.MaterialExpressionVectorParameter
LOG = []


def local_to_world(graph, source):
    """A local vector in world space, scaled by the actor's scale."""
    node = graph.node(unreal.MaterialExpressionTransform, transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL,
                      transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    graph.connect(source, node)
    return node


def build_frame():
    graph = toolkit["open_function"](FUNCTION_FOLDER, "MF_RailFrame",
                                     "Where a pixel of a wall sits on it, in world cm. The wall is the engine cube, "
                                     "scaled and turned.",
                                     CATEGORY)
    op = graph.op
    end_inset = graph.input("EndInset", "Scalar", 1, "How far before each end of the wall its line stops, in half "
                            "widths of the wall.")
    offset = op(unreal.MaterialExpressionSubtract, A=graph.node(unreal.MaterialExpressionWorldPosition),
                B=graph.node(unreal.MaterialExpressionObjectPositionWS))
    axes = []
    for index, local_axis in enumerate(((1.0, 0.0, 0.0), (0.0, 1.0, 0.0))):
        constant = graph.node(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(*local_axis, 0.0))
        axes.append(local_to_world(graph, constant))

    length_axis = op(unreal.MaterialExpressionNormalize, axes[0])
    width_axis = op(unreal.MaterialExpressionNormalize, axes[1])
    across = op(unreal.MaterialExpressionDotProduct, A=offset, B=width_axis)
    half_width = graph.node(unreal.MaterialExpressionMultiply, const_b=HALF_CUBE)
    graph.connect(op(unreal.MaterialExpressionLength, axes[1]), half_width, "A")
    half_length = graph.node(unreal.MaterialExpressionMultiply, const_b=HALF_CUBE)
    graph.connect(op(unreal.MaterialExpressionLength, axes[0]), half_length, "A")
    line_half_length = op(unreal.MaterialExpressionSubtract, A=half_length,
                          B=op(unreal.MaterialExpressionMultiply, A=end_inset, B=half_width))

    normal_z = graph.mask(graph.node(unreal.MaterialExpressionVertexNormalWS), "b")
    top_face = graph.node(unreal.MaterialExpressionStep, const_y=0.5)
    graph.connect(normal_z, top_face, "X")

    # From the world origin rather than the wall's centre, so walls on one line agree.
    along = op(unreal.MaterialExpressionDotProduct, A=graph.node(unreal.MaterialExpressionWorldPosition), B=length_axis)
    from_centre = op(unreal.MaterialExpressionDotProduct, A=offset, B=length_axis)
    beyond_end = op(unreal.MaterialExpressionSubtract, A=op(unreal.MaterialExpressionAbs, from_centre), B=line_half_length)
    past_end = graph.node(unreal.MaterialExpressionMax, const_b=0.0)
    graph.connect(beyond_end, past_end, "A")
    segment_distance = op(unreal.MaterialExpressionLength, op(unreal.MaterialExpressionAppendVector, A=across, B=past_end))

    graph.output("Along", 0, "Position along the wall's line from the world origin, in world cm, growing along the wall's "
                 "local +X.", along)
    graph.output("Across", 1, "Signed distance across the wall's width from its centre line, in world cm.", across)
    graph.output("HalfWidth", 2, "Half the wall's width, in world cm.", half_width)
    graph.output("TopFace", 3, "1 on the face pointing up, 0 on the sides.", top_face)
    graph.output("FromCentre", 4, "Position along the wall from its centre, in world cm, growing the way Along does.",
                 from_centre)
    graph.output("LineHalfLength", 5, "Half the length of the wall's line, EndInset short of each end, in world cm.",
                 line_half_length)
    graph.output("SegmentDistance", 6, "Distance to the wall's line, ends included, in world cm: round past its ends.",
                 segment_distance)
    graph.finish()
    return graph.asset


def build_quads():
    graph = toolkit["open_function"](FUNCTION_FOLDER, "MF_RailQuads",
                                     "Squares spaced evenly along a line, alternating hollow and solid by their index "
                                     "along it, each shown whole while its centre is on the line.", CATEGORY)
    op = graph.op
    along = graph.input("Along", "Scalar", 0, "Position along the line, in cm.")
    across = graph.input("Across", "Scalar", 1, "Distance across the line from its middle, in cm.")
    spacing = graph.input("Spacing", "Scalar", 2, "Distance between neighbouring squares' centres, in cm.")
    radius = graph.input("Radius", "Scalar", 3, "Centre to corner of each square, in cm.")
    outline = graph.input("OutlineWidth", "Scalar", 4, "Line width of the hollow squares, in cm.")
    rotation = graph.input("Rotation", "Scalar", 5, "Counter-clockwise turn of every square, in turns.")
    from_centre = graph.input("FromCentre", "Scalar", 6, "Position along the line from its middle, growing the way "
                              "Along does, in cm.")
    line_half_length = graph.input("LineHalfLength", "Scalar", 7, "Half the line's length, in cm.")
    polygon_distance = load(f"{GENERIC}/MF_PolygonDistance")
    stroke_hard = load(f"{GENERIC}/MF_Stroke_Hard")

    in_cells = op(unreal.MaterialExpressionDivide, A=along, B=spacing)
    cell_index = op(unreal.MaterialExpressionFloor, in_cells)
    centred = graph.node(unreal.MaterialExpressionSubtract, const_b=0.5)
    graph.connect(op(unreal.MaterialExpressionFrac, in_cells), centred, "A")
    in_cell = op(unreal.MaterialExpressionMultiply, A=centred, B=spacing)
    position = op(unreal.MaterialExpressionAppendVector, A=in_cell, B=across)
    cell_from_centre = op(unreal.MaterialExpressionSubtract, A=from_centre, B=in_cell)
    # Step(Y, X) is 1 where X >= Y: the square shows whole while its centre is on the line.
    on_line = op(unreal.MaterialExpressionStep, X=line_half_length,
                 Y=op(unreal.MaterialExpressionAbs, cell_from_centre))
    sides = graph.node(unreal.MaterialExpressionConstant, r=4.0)

    to_outline = graph.call(polygon_distance, Position=position, Sides=sides, Radius=radius, Rotation=rotation)
    hollow = graph.call(stroke_hard, Distance=(to_outline, "Distance"), Width=outline)
    # A zero-radius polygon measures along the nearest edge normal: under the apothem is inside.
    to_centre = graph.call(polygon_distance, Position=position, Sides=sides,
                           Radius=graph.node(unreal.MaterialExpressionConstant, r=0.0), Rotation=rotation)
    apothem = graph.node(unreal.MaterialExpressionMultiply, const_b=2.0 * HALF_SQRT2)
    graph.connect(radius, apothem, "A")
    solid_width = op(unreal.MaterialExpressionAdd, A=apothem, B=outline)
    solid = graph.call(stroke_hard, Distance=(to_centre, "Distance"), Width=solid_width)

    half_index = graph.node(unreal.MaterialExpressionMultiply, const_b=0.5)
    graph.connect(cell_index, half_index, "A")
    is_odd = graph.node(unreal.MaterialExpressionMultiply, const_b=2.0)
    graph.connect(op(unreal.MaterialExpressionFrac, half_index), is_odd, "A")
    mask = op(unreal.MaterialExpressionLinearInterpolate, A=(hollow, "Mask"), B=(solid, "Mask"), Alpha=is_odd)
    graph.output("Mask", 0, "1 on the squares, 0 between them.",
                 op(unreal.MaterialExpressionMultiply, A=mask, B=on_line))
    graph.finish()
    return graph.asset


def build_material(frame_function, quads_function):
    graph = toolkit["open_material"](FOLDER, "M_ArenaRail")
    node, op, parameter = graph.node, graph.op, graph.parameter

    # Stops where a wall meeting this one at a corner has its own centre line, so the two lines meet, not cross.
    end_inset = parameter(scalar, "EndInset", 1.0, "01 Line", 2,
                          "How far before each end of the wall the line stops, in half widths of the wall; keep equal "
                          "to MI_ArenaRailHalo's EndInset")
    frame = graph.call(frame_function, EndInset=end_inset)

    # Core line over the distance to the wall's line segment.
    core_width = parameter(scalar, "CoreWidth", 1.5, "01 Line", 0, "Width of the core at half brightness, in cm")
    core = graph.call(load(f"{GENERIC}/MF_Stroke_Smooth"), Distance=(frame, "SegmentDistance"), Width=core_width)
    core_color = parameter(vector, "CoreColor", unreal.LinearColor(0.9, 0.9, 1.0, 1.0), "03 Colour", 1,
                           "Colour of the core line, near white")
    core_brightness = parameter(scalar, "CoreBrightness", 8.0, "01 Line", 1, "Core emissive, 6-15")
    rail_color = parameter(vector, "RailColor", unreal.LinearColor(*RAIL_COLOR), "03 Colour", 0,
                           "Hue of the quads; keep equal to MI_ArenaRailHalo's HaloColor. Mirrors Game Data Settings > "
                           "ColorPalette > DeployableBlockingAll, which a material cannot read")
    line = op(unreal.MaterialExpressionMultiply, A=(core, "Mask"),
              B=op(unreal.MaterialExpressionMultiply, A=core_color, B=core_brightness))

    # Quads: constant drift along the wall, constant turn.
    time = node(unreal.MaterialExpressionTime)
    speed = parameter(scalar, "QuadSpeed", 120.0, "02 Quads", 0, "Travel speed along the wall, in cm per second")
    travel = op(unreal.MaterialExpressionMultiply, A=time, B=speed)
    spin = parameter(scalar, "QuadSpinSpeed", 0.25, "02 Quads", 1, "Turns per second of every quad")
    quads = graph.call(quads_function, Along=op(unreal.MaterialExpressionAdd, A=(frame, "Along"), B=travel),
                       Across=(frame, "Across"),
                       Spacing=parameter(scalar, "QuadSpacing", 240.0, "02 Quads", 2,
                                         "Distance between quad centres, in cm"),
                       Radius=parameter(scalar, "QuadRadius", 22.0, "02 Quads", 3,
                                        "Centre to corner of each quad, in cm; keep under the wall's half width"),
                       OutlineWidth=parameter(scalar, "QuadOutlineWidth", 3.0, "02 Quads", 4,
                                              "Line width of the hollow quads, in cm"),
                       Rotation=op(unreal.MaterialExpressionMultiply, A=time, B=spin),
                       FromCentre=(frame, "FromCentre"), LineHalfLength=(frame, "LineHalfLength"))
    quad_brightness = parameter(scalar, "QuadBrightness", 2.0, "02 Quads", 5, "Quad emissive, 1.5-4")
    quad_light = op(unreal.MaterialExpressionMultiply, A=(quads, "Mask"),
                    B=op(unreal.MaterialExpressionMultiply, A=rail_color, B=quad_brightness))

    light = op(unreal.MaterialExpressionAdd, A=line, B=quad_light)
    graph.to_property(op(unreal.MaterialExpressionMultiply, A=light, B=(frame, "TopFace")),
                      unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    material = graph.asset
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    return finish_material(graph)


def build_halo_material(frame_function):
    graph = toolkit["open_material"](FOLDER, "M_ArenaRailHalo")
    op, parameter = graph.op, graph.parameter
    end_inset = parameter(scalar, "EndInset", 1.0, "01 Halo", 2,
                          "How far before each end of the wall the line stops, in half widths of the wall; the halo "
                          "rounds off around that end. Keep equal to the overlay instances' EndInset")
    frame = graph.call(frame_function, EndInset=end_inset)
    distance = (frame, "SegmentDistance")
    halo = graph.call(load(f"{GENERIC}/MF_Stroke_Smooth"), Distance=distance, Width=(frame, "HalfWidth"))
    color = parameter(vector, "HaloColor", unreal.LinearColor(*RAIL_COLOR), "01 Halo", 0,
                      "Hue of the halo; keep equal to the overlay instances' RailColor")
    brightness = parameter(scalar, "HaloBrightness", 0.3, "01 Halo", 1,
                           "Halo emissive, 0.2-0.8; its coverage fades from full on the line to none half a wall "
                           "width away")
    coverage = op(unreal.MaterialExpressionMultiply, A=(halo, "Mask"), B=(frame, "TopFace"))
    depth_per_cm = parameter(scalar, "DepthPerCm", 1.0, "02 Overlap", 0,
                             "cm a pixel sinks per cm from the line: what makes the nearer wall win an overlap. "
                             "Keep the sink at the wall's side under the wall's height")
    # Python has no pixel depth offset material property; the attributes node carries it.
    dither = graph.call(load(DITHER), **{"Alpha Threshold": coverage})
    attributes = op(unreal.MaterialExpressionMakeMaterialAttributes, EmissiveColor=op(unreal.MaterialExpressionMultiply, A=color, B=brightness),
                    OpacityMask=(dither, "Result"),
                    PixelDepthOffset=op(unreal.MaterialExpressionMultiply, A=distance, B=depth_per_cm))
    graph.to_property(attributes, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)

    material = graph.asset
    material.set_editor_property("use_material_attributes", True)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    # Masked pixels still write depth, which the overlap relies on; the dither reads as partial cover under TSR.
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    return finish_material(graph)


def finish_material(graph):
    material = graph.asset
    graph.finish()
    stats = mel.get_statistics(material)
    LOG.append(f"{material.get_name()}: {mel.get_num_material_expressions(material)} nodes, vertex "
               f"{stats.num_vertex_shader_instructions} / pixel {stats.num_pixel_shader_instructions} instructions")
    assert stats.num_pixel_shader_instructions > 0, f"{material.get_name()} did not compile, see the material editor"
    return material


def build_instance(parent, name):
    instance = toolkit["load_or_create"](FOLDER, name, unreal.MaterialInstanceConstant,
                                         unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(instance, parent)
    mel.update_material_instance(instance)
    save(instance)
    return instance


def wire_walls(halo, overlay):
    """Every wall on the old frame material or already on the rail: the halo as its material, the overlay on top."""
    wall_materials = {OLD_WALL_MATERIAL, halo.get_path_name()}
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    for actor in actors:
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            material = component.get_material(0)
            if material and material.get_path_name() in wall_materials:
                actor.modify()
                component.modify()
                component.set_material(0, halo)
                component.set_editor_property("overlay_material", overlay)
                LOG.append(f"{actor.get_actor_label()} -> {halo.get_name()} + {overlay.get_name()}")


def delete_old_overlays():
    for suffix in OLD_OVERLAYS:
        path = f"{FOLDER}/MI_ArenaRail_{suffix}"
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            LOG.append(f"{path}: {'deleted' if unreal.EditorAssetLibrary.delete_asset(path) else 'NOT deleted'}")


try:
    frame = build_frame()
    halo = build_instance(build_halo_material(frame), "MI_ArenaRailHalo")
    wire_walls(halo, build_instance(build_material(frame, build_quads()), "MI_ArenaRail"))
    delete_old_overlays()
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
