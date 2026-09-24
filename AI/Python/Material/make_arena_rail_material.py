"""Arena border rail, in two passes on each wall's top face.

Each wall's line runs down its middle and stops EndInset half widths before each end, where a wall meeting it at a
corner has its own line, so the lines meet rather than cross.
The wall's own material, M_ArenaRailHalo, is masked: a halo of the rail colour whose dithered coverage fades from
full on the line to none half a wall width away, rounding off past the line's ends, so what lies under the wall shows
through. Each pixel is pushed down by its distance from the line: where walls overlap, the depth test keeps the wall
whose line is nearer, so overlapping halos join as one shape instead of adding up.
Its overlay material, M_ArenaRail, is additive: a near-white core line and small quads riding it, hollow and solid in
turn, travelling and turning, each shown whole while its centre is on the line. Those do add up where walls cross.

Everything is measured in world cm, so the rail keeps its size while a barrier scales a wall in or out. Each wall
picks its travel direction from its arena's centre, so every wall of one arena circulates the same way round, and
quads are placed from the world origin, so two walls on one line draw the same quads. One overlay instance per
arena, holding that centre.

Puts each placed wall on the engine's VR frame material, or already on the rail, onto the halo and its arena's
overlay. The level is left dirty, never saved. Run through execute_script, outside PIE. Report written to
Saved/arena_rail_material.txt.
"""
import unreal

FOLDER = "/Game/Art/VFX/Arena"
FUNCTION_FOLDER = FOLDER + "/Functions"
GENERIC = "/Game/Art/VFX/Generic/Materials/Functions"
DITHER = "/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA"
CATEGORY = "GeoTrinity|Arena"
REPORT = unreal.Paths.project_saved_dir() + "arena_rail_material.txt"
OLD_WALL_MATERIAL = "/Engine/VREditor/UI/FrameMaterial.FrameMaterial"
HALF_CUBE = 50.0  # the engine cube spans -50..50 cm on each local axis
HALF_SQRT2 = 0.7071067811865476
RAIL_COLOR = (0.45, 0.2, 1.0, 1.0)

# Arena actor label -> (instance suffix, centre the walls circulate around, world XY).
ARENAS = {
    "StarArena": ("Star", (2571.0, 0.0)),
    "HexArena": ("Hex", (6740.0, 0.0)),
    "Tuto_ZoneArena": ("Tutorial", (-6000.0, 0.0)),
    "EntranceArena": ("Entrance", (0.0, 0.0)),
}

toolkit_path = unreal.Paths.project_dir() + "AI/Python/Material/material_graph_authoring.py"
toolkit = {}
exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
mel = toolkit["mel"]
load = toolkit["load"]
save = toolkit["save"]

scalar = unreal.MaterialExpressionScalarParameter
vector = unreal.MaterialExpressionVectorParameter
LOG = []


def local_to_world(graph, source, x, y):
    """A local vector in world space, scaled by the actor's scale."""
    node = graph.node(unreal.MaterialExpressionTransform, x, y,
                      transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL,
                      transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    graph.connect(source, node)
    return node


def build_frame():
    graph = toolkit["open_function"](FUNCTION_FOLDER, "MF_RailFrame",
                                     "Where a pixel of a wall sits on it, in world cm. The wall is the engine cube, "
                                     "scaled and turned.",
                                     CATEGORY)
    op = graph.op
    center = graph.input("ArenaCenter", "Vector2", 0, "World XY every wall of the arena circulates around.", -1400, 700)
    end_inset = graph.input("EndInset", "Scalar", 1, "How far before each end of the wall its line stops, in half "
                            "widths of the wall.", -1400, 900)
    offset = op(unreal.MaterialExpressionSubtract, -1000, -100,
                A=graph.node(unreal.MaterialExpressionWorldPosition, -1200, -140),
                B=graph.node(unreal.MaterialExpressionObjectPositionWS, -1200, -40))
    axes = []
    for index, local_axis in enumerate(((1.0, 0.0, 0.0), (0.0, 1.0, 0.0))):
        constant = graph.node(unreal.MaterialExpressionConstant3Vector, -1400, 100 + 200 * index,
                              constant=unreal.LinearColor(*local_axis, 0.0))
        axes.append(local_to_world(graph, constant, -1200, 100 + 200 * index))

    length_axis = op(unreal.MaterialExpressionNormalize, -1000, 100, axes[0])
    width_axis = op(unreal.MaterialExpressionNormalize, -1000, 300, axes[1])
    across = op(unreal.MaterialExpressionDotProduct, -700, 60, A=offset, B=width_axis)
    half_width = graph.node(unreal.MaterialExpressionMultiply, -700, 400, const_b=HALF_CUBE)
    graph.connect(op(unreal.MaterialExpressionLength, -900, 400, axes[1]), half_width, "A")
    half_length = graph.node(unreal.MaterialExpressionMultiply, -700, 200, const_b=HALF_CUBE)
    graph.connect(op(unreal.MaterialExpressionLength, -900, 200, axes[0]), half_length, "A")
    line_half_length = op(unreal.MaterialExpressionSubtract, -400, 200, A=half_length,
                          B=op(unreal.MaterialExpressionMultiply, -550, 300, A=end_inset, B=half_width))

    normal_z = graph.mask(graph.node(unreal.MaterialExpressionVertexNormalWS, -1000, 520), "b", -850, 520)
    top_face = graph.node(unreal.MaterialExpressionStep, -700, 520, const_y=0.5)
    graph.connect(normal_z, top_face, "X")

    # The wall's length axis against the tangent (r.y, -r.x) at the wall's centre: +1 with it, -1 against.
    radial = op(unreal.MaterialExpressionSubtract, -1000, 700,
                A=graph.mask(graph.node(unreal.MaterialExpressionObjectPositionWS, -1300, 820), "rg", -1150, 820),
                B=center)
    minus_radial_x = graph.node(unreal.MaterialExpressionMultiply, -850, 760, const_b=-1.0)
    graph.connect(graph.mask(radial, "r", -900, 700), minus_radial_x, "A")
    tangent = op(unreal.MaterialExpressionAppendVector, -700, 720, A=graph.mask(radial, "g", -850, 660),
                 B=minus_radial_x)
    agreement = op(unreal.MaterialExpressionDotProduct, -550, 700, A=graph.mask(length_axis, "rg", -700, 620),
                   B=tangent)
    with_tangent = graph.node(unreal.MaterialExpressionStep, -400, 700, const_y=0.0)
    graph.connect(agreement, with_tangent, "X")
    doubled = graph.node(unreal.MaterialExpressionMultiply, -260, 700, const_b=2.0)
    graph.connect(with_tangent, doubled, "A")
    circulation = graph.node(unreal.MaterialExpressionSubtract, -120, 700, const_b=1.0)
    graph.connect(doubled, circulation, "A")
    # From the world origin rather than the wall's centre, so walls on one line agree.
    world_along = op(unreal.MaterialExpressionDotProduct, -700, -100,
                     A=graph.node(unreal.MaterialExpressionWorldPosition, -900, -240), B=length_axis)
    along = op(unreal.MaterialExpressionMultiply, 40, -100, A=world_along, B=circulation)
    from_centre = op(unreal.MaterialExpressionMultiply, 40, 0, B=circulation,
                     A=op(unreal.MaterialExpressionDotProduct, -700, -20, A=offset, B=length_axis))
    beyond_end = op(unreal.MaterialExpressionSubtract, -250, 100,
                    A=op(unreal.MaterialExpressionAbs, -400, 60, from_centre), B=line_half_length)
    past_end = graph.node(unreal.MaterialExpressionMax, -100, 100, const_b=0.0)
    graph.connect(beyond_end, past_end, "A")
    segment_distance = op(unreal.MaterialExpressionLength, 200, 100,
                          op(unreal.MaterialExpressionAppendVector, 50, 100, A=across, B=past_end))

    graph.output("Along", 0, "Position along the wall's line from the world origin, in world cm, growing the same way "
                 "round ArenaCenter on every wall.", along, 200, -100)
    graph.output("Across", 1, "Signed distance across the wall's width from its centre line, in world cm.", across,
                 -400, 60)
    graph.output("HalfWidth", 2, "Half the wall's width, in world cm.", half_width, -400, 400)
    graph.output("TopFace", 3, "1 on the face pointing up, 0 on the sides.", top_face, -400, 520)
    graph.output("FromCentre", 4, "Position along the wall from its centre, in world cm, growing the way Along does.",
                 from_centre, 200, 0)
    graph.output("LineHalfLength", 5, "Half the length of the wall's line, EndInset short of each end, in world cm.",
                 line_half_length, -250, 200)
    graph.output("SegmentDistance", 6, "Distance to the wall's line, ends included, in world cm: round past its ends.",
                 segment_distance, 350, 100)
    graph.finish()
    return graph.asset


def build_quads():
    graph = toolkit["open_function"](FUNCTION_FOLDER, "MF_RailQuads",
                                     "Squares spaced evenly along a line, alternating hollow and solid by their index "
                                     "along it, each shown whole while its centre is on the line.", CATEGORY)
    op = graph.op
    along = graph.input("Along", "Scalar", 0, "Position along the line, in cm.", -1400, -100)
    across = graph.input("Across", "Scalar", 1, "Distance across the line from its middle, in cm.", -1400, 40)
    spacing = graph.input("Spacing", "Scalar", 2, "Distance between neighbouring squares' centres, in cm.", -1400, 160)
    radius = graph.input("Radius", "Scalar", 3, "Centre to corner of each square, in cm.", -1400, 280)
    outline = graph.input("OutlineWidth", "Scalar", 4, "Line width of the hollow squares, in cm.", -1400, 400)
    rotation = graph.input("Rotation", "Scalar", 5, "Counter-clockwise turn of every square, in turns.", -1400, 520)
    from_centre = graph.input("FromCentre", "Scalar", 6, "Position along the line from its middle, growing the way "
                              "Along does, in cm.", -1400, 640)
    line_half_length = graph.input("LineHalfLength", "Scalar", 7, "Half the line's length, in cm.", -1400, 760)
    polygon_distance = load(f"{GENERIC}/MF_PolygonDistance")
    stroke_hard = load(f"{GENERIC}/MF_Stroke_Hard")

    in_cells = op(unreal.MaterialExpressionDivide, -1200, -100, A=along, B=spacing)
    cell_index = op(unreal.MaterialExpressionFloor, -1050, -200, in_cells)
    centred = graph.node(unreal.MaterialExpressionSubtract, -950, -60, const_b=0.5)
    graph.connect(op(unreal.MaterialExpressionFrac, -1050, -60, in_cells), centred, "A")
    in_cell = op(unreal.MaterialExpressionMultiply, -800, -60, A=centred, B=spacing)
    position = op(unreal.MaterialExpressionAppendVector, -650, 0, A=in_cell, B=across)
    cell_from_centre = op(unreal.MaterialExpressionSubtract, -1200, 700, A=from_centre, B=in_cell)
    # Step(Y, X) is 1 where X >= Y: the square shows whole while its centre is on the line.
    on_line = op(unreal.MaterialExpressionStep, -900, 760, X=line_half_length,
                 Y=op(unreal.MaterialExpressionAbs, -1050, 700, cell_from_centre))
    sides = graph.node(unreal.MaterialExpressionConstant, -650, 160, r=4.0)

    to_outline = graph.call(polygon_distance, -450, 0, Position=position, Sides=sides, Radius=radius, Rotation=rotation)
    hollow = graph.call(stroke_hard, -150, 0, Distance=(to_outline, "Distance"), Width=outline)
    # A zero-radius polygon measures along the nearest edge normal: under the apothem is inside.
    to_centre = graph.call(polygon_distance, -450, 300, Position=position, Sides=sides,
                           Radius=graph.node(unreal.MaterialExpressionConstant, -650, 360, r=0.0), Rotation=rotation)
    apothem = graph.node(unreal.MaterialExpressionMultiply, -650, 460, const_b=2.0 * HALF_SQRT2)
    graph.connect(radius, apothem, "A")
    solid_width = op(unreal.MaterialExpressionAdd, -450, 480, A=apothem, B=outline)
    solid = graph.call(stroke_hard, -150, 300, Distance=(to_centre, "Distance"), Width=solid_width)

    half_index = graph.node(unreal.MaterialExpressionMultiply, -900, -200, const_b=0.5)
    graph.connect(cell_index, half_index, "A")
    is_odd = graph.node(unreal.MaterialExpressionMultiply, -600, -200, const_b=2.0)
    graph.connect(op(unreal.MaterialExpressionFrac, -750, -200, half_index), is_odd, "A")
    mask = op(unreal.MaterialExpressionLinearInterpolate, 100, 100, A=(hollow, "Mask"), B=(solid, "Mask"), Alpha=is_odd)
    graph.output("Mask", 0, "1 on the squares, 0 between them.",
                 op(unreal.MaterialExpressionMultiply, 250, 100, A=mask, B=on_line), 400, 100)
    graph.finish()
    return graph.asset


def build_material(frame_function, quads_function):
    graph = toolkit["open_material"](FOLDER, "M_ArenaRail")
    node, op, parameter = graph.node, graph.op, graph.parameter

    center = parameter(vector, "ArenaCenter", unreal.LinearColor(0.0, 0.0, 0.0, 0.0), "04 Arena", 0,
                       "World XY, in RG, every wall of this arena's quads circulate around. Set per arena instance by "
                       "make_arena_rail_material.py", -2000, 900)
    # Stops where a wall meeting this one at a corner has its own centre line, so the two lines meet, not cross.
    end_inset = parameter(scalar, "EndInset", 1.0, "01 Line", 2,
                          "How far before each end of the wall the line stops, in half widths of the wall; keep equal "
                          "to MI_ArenaRailHalo's EndInset", -2000, 1000)
    frame = graph.call(frame_function, -1700, 300, ArenaCenter=graph.mask(center, "rg", -1850, 900),
                       EndInset=end_inset)

    # Core line over the distance to the wall's line segment.
    core_width = parameter(scalar, "CoreWidth", 1.5, "01 Line", 0, "Width of the core at half brightness, in cm",
                           -1400, -300)
    core = graph.call(load(f"{GENERIC}/MF_Stroke_Smooth"), -1100, -200,
                      Distance=(frame, "SegmentDistance"), Width=core_width)
    core_color = parameter(vector, "CoreColor", unreal.LinearColor(0.9, 0.9, 1.0, 1.0), "03 Colour", 1,
                           "Colour of the core line, near white", -1100, -520)
    core_brightness = parameter(scalar, "CoreBrightness", 8.0, "01 Line", 1, "Core emissive, 6-15", -1100, -420)
    rail_color = parameter(vector, "RailColor", unreal.LinearColor(*RAIL_COLOR), "03 Colour", 0,
                           "Hue of the quads; keep equal to MI_ArenaRailHalo's HaloColor. Mirrors Game Data Settings > "
                           "ColorPalette > DeployableBlockingAll, which a material cannot read", -1100, 100)
    line = op(unreal.MaterialExpressionMultiply, -700, -200, A=(core, "Mask"),
              B=op(unreal.MaterialExpressionMultiply, -850, -480, A=core_color, B=core_brightness))

    # Quads: constant drift along the wall, constant turn.
    time = node(unreal.MaterialExpressionTime, -1700, 700)
    speed = parameter(scalar, "QuadSpeed", 120.0, "02 Quads", 0, "Travel speed along the wall, in cm per second",
                      -1700, 800)
    travel = op(unreal.MaterialExpressionMultiply, -1500, 700, A=time, B=speed)
    spin = parameter(scalar, "QuadSpinSpeed", 0.25, "02 Quads", 1, "Turns per second of every quad", -1500, 1000)
    quads = graph.call(quads_function, -1000, 600,
                       Along=op(unreal.MaterialExpressionAdd, -1150, 500, A=(frame, "Along"), B=travel),
                       Across=(frame, "Across"),
                       Spacing=parameter(scalar, "QuadSpacing", 240.0, "02 Quads", 2,
                                         "Distance between quad centres, in cm", -1300, 850),
                       Radius=parameter(scalar, "QuadRadius", 22.0, "02 Quads", 3,
                                        "Centre to corner of each quad, in cm; keep under the wall's half width",
                                        -1300, 950),
                       OutlineWidth=parameter(scalar, "QuadOutlineWidth", 3.0, "02 Quads", 4,
                                              "Line width of the hollow quads, in cm", -1300, 1050),
                       Rotation=op(unreal.MaterialExpressionMultiply, -1300, 1150, A=time, B=spin),
                       FromCentre=(frame, "FromCentre"), LineHalfLength=(frame, "LineHalfLength"))
    quad_brightness = parameter(scalar, "QuadBrightness", 2.0, "02 Quads", 5, "Quad emissive, 1.5-4", -800, 800)
    quad_light = op(unreal.MaterialExpressionMultiply, -500, 600, A=(quads, "Mask"),
                    B=op(unreal.MaterialExpressionMultiply, -650, 750, A=rail_color, B=quad_brightness))

    light = op(unreal.MaterialExpressionAdd, -300, 200, A=line, B=quad_light)
    graph.to_property(op(unreal.MaterialExpressionMultiply, -100, 300, A=light, B=(frame, "TopFace")),
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
                          "rounds off around that end. Keep equal to the overlay instances' EndInset", -1500, 100)
    # The centre only drives Along, which is not wired here, so it never compiles in.
    frame = graph.call(frame_function, -1300, 0, EndInset=end_inset,
                       ArenaCenter=graph.node(unreal.MaterialExpressionConstant2Vector, -1500, 0, r=0.0, g=0.0))
    distance = (frame, "SegmentDistance")
    halo = graph.call(load(f"{GENERIC}/MF_Stroke_Smooth"), -800, -100, Distance=distance, Width=(frame, "HalfWidth"))
    color = parameter(vector, "HaloColor", unreal.LinearColor(*RAIL_COLOR), "01 Halo", 0,
                      "Hue of the halo; keep equal to the overlay instances' RailColor", -800, -300)
    brightness = parameter(scalar, "HaloBrightness", 0.3, "01 Halo", 1,
                           "Halo emissive, 0.2-0.8; its coverage fades from full on the line to none half a wall "
                           "width away", -800, -200)
    coverage = op(unreal.MaterialExpressionMultiply, -500, -100, A=(halo, "Mask"), B=(frame, "TopFace"))
    depth_per_cm = parameter(scalar, "DepthPerCm", 1.0, "02 Overlap", 0,
                             "cm a pixel sinks per cm from the line: what makes the nearer wall win an overlap. "
                             "Keep the sink at the wall's side under the wall's height", -800, 200)
    # Python has no pixel depth offset material property; the attributes node carries it.
    dither = graph.call(load(DITHER), -300, -100, **{"Alpha Threshold": coverage})
    attributes = op(unreal.MaterialExpressionMakeMaterialAttributes, 0, 0,
                    EmissiveColor=op(unreal.MaterialExpressionMultiply, -600, -250, A=color, B=brightness),
                    OpacityMask=(dither, "Result"),
                    PixelDepthOffset=op(unreal.MaterialExpressionMultiply, -400, 200, A=distance, B=depth_per_cm))
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


def build_instance(parent, name, center=None):
    instance = toolkit["load_or_create"](FOLDER, name, unreal.MaterialInstanceConstant,
                                         unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(instance, parent)
    if center:
        mel.set_material_instance_vector_parameter_value(instance, "ArenaCenter",
                                                         unreal.LinearColor(center[0], center[1], 0.0, 0.0))

    mel.update_material_instance(instance)
    save(instance)
    return instance


def wire_walls(halo, overlays):
    """Every wall on the old frame material or already on the rail: the halo as its material, its arena's overlay."""
    wall_materials = {OLD_WALL_MATERIAL, halo.get_path_name()}
    wall_materials |= {overlay.get_path_name() for overlay in overlays.values()}
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    for actor in actors:
        arena = actor.get_attach_parent_actor()
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            material = component.get_material(0)
            if material and material.get_path_name() in wall_materials:
                arena_label = arena.get_actor_label() if arena else None
                if arena_label in overlays:
                    actor.modify()
                    component.modify()
                    component.set_material(0, halo)
                    component.set_editor_property("overlay_material", overlays[arena_label])
                    LOG.append(f"{actor.get_actor_label()} -> {halo.get_name()} + {overlays[arena_label].get_name()}")
                else:
                    LOG.append(f"{actor.get_actor_label()}: arena {arena_label} has no rail instance, left as is")


try:
    frame = build_frame()
    halo = build_instance(build_halo_material(frame), "MI_ArenaRailHalo")
    parent = build_material(frame, build_quads())
    wire_walls(halo, {label: build_instance(parent, f"MI_ArenaRail_{suffix}", center)
                      for label, (suffix, center) in ARENAS.items()})
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
