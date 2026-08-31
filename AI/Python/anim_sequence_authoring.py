"""Generic helpers for reading and authoring skeletal rigs and AnimSequences.

Run via mcp-unreal execute_script against the editor world. Covers what Python can do on animation
assets: report which bones a sequence actually animates, dump a skeleton's reference pose and hierarchy,
report which bones actually deform the mesh, recover the mesh's radial layout, add bones and re-bind
vertices to them, write bone tracks from a caller-supplied per-frame sampler, and build the montage that
plays the result.

Adjust the example call at the bottom for the asset you are working on.
"""
import math
import random

import unreal

APE = unreal.AnimPoseExtensions


def get_or_create_asset(package_path, asset_name, asset_class, factory):
    """Load the asset if it exists, else create it.

    Loaded rather than looked up in the asset registry: a registry still scanning reports an asset that is on disk
    as missing, and creating over it then fails and hands back nothing. Never deletes: deleting a loaded asset
    leaves the package unloadable for the rest of the editor session.
    """
    asset = unreal.load_asset("{}/{}".format(package_path, asset_name))
    return asset or unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, asset_class, factory)


def reference_pose_table(skeleton_path):
    """Skeleton path -> {bone: (local_transform, component_transform)} for the reference pose."""
    skeleton = unreal.load_asset(skeleton_path)
    ref = APE.get_reference_pose(skeleton)
    table = {}
    for bone in APE.get_bone_names(ref):
        name = str(bone)
        table[name] = (APE.get_bone_pose(ref, name, unreal.AnimPoseSpaces.LOCAL),
                       APE.get_bone_pose(ref, name, unreal.AnimPoseSpaces.WORLD))
    return table


def bone_parents(skeletal_mesh_path):
    """The hierarchy: {bone: parent bone} for every bone the mesh carries, 'None' at the root.

    Read by name from the modifier rather than derived from positions, which cannot separate bones that sit at the
    same place.
    """
    modifier = unreal.SkeletonModifier()
    modifier.set_skeletal_mesh(unreal.load_asset(skeletal_mesh_path))
    return {str(bone): str(modifier.get_parent_name(bone)) for bone in modifier.get_all_bone_names()}


def report_moving_bones(sequence_path, samples=12, tolerance=0.05):
    """Which bones a sequence actually animates, and by how much.

    The legacy track/curve accessors report empty for every sequence, so this evaluates the pose
    instead. Returns {bone: max_delta} for bones that move.
    """
    seq = unreal.load_asset(sequence_path)
    length = seq.get_editor_property("sequence_length")
    options = unreal.AnimPoseEvaluationOptions()

    base = {}
    first = APE.get_anim_pose_at_time(seq, 0.0, options)
    names = [str(b) for b in APE.get_bone_names(first)]
    for name in names:
        # Extract immediately: evaluated poses alias one shared buffer.
        base[name] = _snapshot(APE.get_bone_pose(first, name, unreal.AnimPoseSpaces.LOCAL))

    moving = {}
    for i in range(1, samples + 1):
        pose = APE.get_anim_pose_at_time(seq, length * i / float(samples), options)
        for name in names:
            delta = _delta(_snapshot(APE.get_bone_pose(pose, name, unreal.AnimPoseSpaces.LOCAL)), base[name])
            if delta > tolerance:
                moving[name] = max(moving.get(name, 0.0), delta)
    return moving


def report_skin_weights(skeletal_mesh_path):
    """Which bones actually deform the mesh -> {bone: (vertex_count, total_weight)}.

    A bone a sequence animates is not necessarily a bone that carries much of the mesh, so weigh a bone's
    influence with this before building motion on it. Takes the skeletal mesh, not the skeleton.
    """
    modifier = unreal.SkinWeightModifier()
    modifier.set_skeletal_mesh(unreal.load_asset(skeletal_mesh_path))
    counts, totals = {}, {}
    for index in range(modifier.get_num_vertices()):
        for bone, weight in modifier.get_vertex_weights(index).items():
            if weight > 0.001:
                name = str(bone)
                counts[name] = counts.get(name, 0) + 1
                totals[name] = totals.get(name, 0.0) + weight
    return {name: (counts[name], round(totals[name], 2)) for name in counts}


def radial_vertex_rings(static_mesh_path, tolerance=1.0):
    """Vertex layout in the XY plane -> [(radius, [angles])], outermost ring first.

    Bounds cannot stand in for this: a mesh's sphere radius is derived from its box corner rather than its
    geometry, so features have to be recovered from vertex positions.
    """
    mesh = unreal.load_asset(static_mesh_path)
    vertices = unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh, 0, 0)[0]
    rings = {}
    for vertex in vertices:
        radius = math.hypot(vertex.x, vertex.y)
        if radius >= tolerance:
            key = round(round(radius / tolerance) * tolerance, 2)
            rings.setdefault(key, set()).add(round(math.degrees(math.atan2(vertex.y, vertex.x)) % 360.0, 1))
    return [(radius, sorted(rings[radius])) for radius in sorted(rings, reverse=True)]


def vertex_weight_report(skeletal_mesh_path):
    """Every vertex paired with its weights -> [(index, position, radius, angle_degrees, {bone: weight})].

    Nothing reaches a skeletal mesh's geometry from script, so positions come from the editor shim; it walks the
    same mesh description the weight modifier indexes, which is what makes the two line up per index.
    """
    mesh = unreal.load_asset(skeletal_mesh_path)
    positions = unreal.get_default_object(unreal.GeoAnimBuilderUtil).get_skeletal_mesh_vertex_positions(mesh)
    modifier = unreal.SkinWeightModifier()
    modifier.set_skeletal_mesh(mesh)
    report = []
    for index in range(modifier.get_num_vertices()):
        position = positions[index]
        report.append((index, position, math.hypot(position.x, position.y),
                       math.degrees(math.atan2(position.y, position.x)) % 360.0,
                       {str(bone): weight for bone, weight in modifier.get_vertex_weights(index).items()}))
    return report


def radial_slots(report, slot_count, radius_ratio=0.9):
    """Split the outermost ring of a vertex_weight_report into evenly spaced directions.

    Returns (ring_radius, phase_radians, {slot: [vertex index]}). Multiplying every angle by the slot count
    collapses evenly spaced features onto one direction whose circular mean recovers the phase, so nothing has to
    be assumed to sit at zero degrees.
    """
    ring_radius = max(entry[2] for entry in report)
    ring = [(entry[0], math.radians(entry[3])) for entry in report if entry[2] > ring_radius * radius_ratio]
    phase = math.atan2(sum(math.sin(slot_count * angle) for _, angle in ring),
                       sum(math.cos(slot_count * angle) for _, angle in ring)) / slot_count
    slots = {}
    for index, angle in ring:
        slots.setdefault(int(round(slot_count * (angle - phase) / (2.0 * math.pi))) % slot_count, []).append(index)
    return ring_radius, phase, slots


def skinned_vertex_positions(skeletal_mesh_path, sequence, times):
    """Where a sequence puts every vertex of a mesh -> [[position per vertex] per time].

    Blends each vertex through its bones the way the renderer does, so this is the silhouette a pose actually has:
    a vertex split across bones lands between what those bones do, and no bone track reads as that on its own.
    Vertex positions come from the editor shim and weights from the modifier, which index alike.
    """
    mesh = unreal.load_asset(skeletal_mesh_path)
    rest = unreal.get_default_object(unreal.GeoAnimBuilderUtil).get_skeletal_mesh_vertex_positions(mesh)
    modifier = unreal.SkinWeightModifier()
    modifier.set_skeletal_mesh(mesh)
    weights = [modifier.get_vertex_weights(index) for index in range(modifier.get_num_vertices())]

    # Extract the whole reference pose before evaluating any other: evaluated poses alias one shared buffer.
    reference = _component_transforms(APE.get_reference_pose(mesh.get_editor_property("skeleton")))

    options = unreal.AnimPoseEvaluationOptions()
    sampled = []
    for time in times:
        posed = _component_transforms(APE.get_anim_pose_at_time(sequence, time, options))
        positions = []
        for index, vertex_weights in enumerate(weights):
            blended = unreal.Vector(0.0, 0.0, 0.0)
            for bone, weight in vertex_weights.items():
                local = unreal.MathLibrary.inverse_transform_location(reference[str(bone)], rest[index])
                blended = blended + unreal.MathLibrary.transform_location(posed[str(bone)], local) * weight
            positions.append(blended)
        sampled.append(positions)
    return sampled


def rigid_vertex_groups(skeletal_mesh_path, skeleton_path):
    """Vertices grouped by the single bone that drives each -> {bone: [position in that bone's space]}.

    For a rig that binds every vertex wholly to one bone, this is all a silhouette needs: putting a vertex back
    out through its bone's posed transform is exactly where the renderer draws it, with none of the blending a
    split vertex forces. Raises on any vertex carrying more than one influence rather than picking one of them.
    """
    mesh = unreal.load_asset(skeletal_mesh_path)
    positions = unreal.get_default_object(unreal.GeoAnimBuilderUtil).get_skeletal_mesh_vertex_positions(mesh)
    modifier = unreal.SkinWeightModifier()
    modifier.set_skeletal_mesh(mesh)
    reference = _component_transforms(APE.get_reference_pose(unreal.load_asset(skeleton_path)))

    groups = {}
    for index, position in enumerate(positions):
        weights = modifier.get_vertex_weights(index)
        if len(weights) != 1:
            raise RuntimeError("vertex {} is split across {} bones".format(index, len(weights)))
        bone = str(next(iter(weights)))
        groups.setdefault(bone, []).append(unreal.MathLibrary.inverse_transform_location(reference[bone], position))
    return groups


def local_pose_table(pose):
    """An evaluated pose in parent space, as plain numbers -> {bone: ((x, y, z), yaw, (sx, sy, sz))}.

    Copied out rather than held as transforms, since evaluated poses alias one shared buffer. Yaw alone stands for
    the rotation on a rig whose bones are all placed with one.
    """
    table = {}
    for bone in APE.get_bone_names(pose):
        local = APE.get_bone_pose(pose, str(bone), unreal.AnimPoseSpaces.LOCAL)
        table[str(bone)] = ((local.translation.x, local.translation.y, local.translation.z),
                            local.rotation.rotator().yaw,
                            (local.scale3d.x, local.scale3d.y, local.scale3d.z))
    return table


def feature_directions(posed, marker):
    """Features whose bones are named `<parent><marker><n>` -> {feature: (parent, resting direction in degrees)}.

    Read from where each bone sits rather than from a list, so it follows the rig instead of restating it.
    """
    layout = {}
    for bone, transform in posed.items():
        if marker in bone:
            position = transform.translation
            layout[bone] = (bone.split(marker)[0], math.degrees(math.atan2(position.y, position.x)))
    return layout


def place_groups(groups, posed):
    """Every group's vertices where a pose puts them -> {bone: [component-space position]}.

    Takes the posed component transforms already extracted from an evaluated pose, since those alias one buffer.
    """
    return {bone: [unreal.MathLibrary.transform_location(posed[bone], local) for local in locals]
            for bone, locals in groups.items()}


def feature_protrusion(placed, posed, feature_bone, parent_bone):
    """How far a feature stands past the surface it grows out of, in units; negative means it is still buried.

    A shape's outermost radius cannot answer this — its corners sit further out than any of its faces, so a
    feature clears its own surface long before it beats that radius. Both sets of vertices are projected onto the
    feature's own outward direction, taken from where its bone sits rather than from its axis, which the scaling
    that hides such a feature would collapse.
    """
    axis = posed[feature_bone].translation
    length = math.hypot(axis.x, axis.y)
    along = lambda group: max(v.x * axis.x / length + v.y * axis.y / length for v in placed[group])
    return along(feature_bone) - along(parent_bone)


def polygon_fit(vertices):
    """The polygon a ring of vertices outlines -> (centre x, centre y, circumradius, phase).

    The centre is fitted rather than assumed to sit on the axis, so a ring an animation has drifted off it is not
    mistaken for one that has shrunk. Circumradius and phase come from the vertex nearest that centre.
    """
    centre_x = sum(vertex.x for vertex in vertices) / float(len(vertices))
    centre_y = sum(vertex.y for vertex in vertices) / float(len(vertices))
    radius, phase = min((math.hypot(vertex.x - centre_x, vertex.y - centre_y),
                         math.atan2(vertex.y - centre_y, vertex.x - centre_x)) for vertex in vertices)
    return centre_x, centre_y, radius, phase


def polygon_reach(radius, phase, angle, sides=6):
    """Distance from a regular polygon's centre out to its boundary in direction `angle` (radians).

    Measured from a corner, so at the phase itself this gives back the circumradius and a half-span round from it
    the apothem. The nearest vertex distance on its own reports the boundary as closer than it is everywhere
    except at the corners, which is exactly where a ring inside it aims its features.
    """
    span = math.pi / sides
    return radius * math.cos(span) / math.cos((angle - phase) % (2.0 * span) - span)


def nested_clearance(placed, inner_bones, boundary_bone, sides=6):
    """Smallest gap in units between the parts on `inner_bones` and the wall of `boundary_bone`.

    Negative means the two interpenetrate, which no bone track shows — nested parts have to be checked for it
    numerically. Everything is measured from the boundary's own fitted centre, so a boundary that has drifted is
    compared against where the inner parts sit relative to it rather than to the axis.
    """
    centre_x, centre_y, radius, phase = polygon_fit(placed[boundary_bone])
    gaps = []
    for bone in inner_bones:
        for vertex in placed[bone]:
            offset_x, offset_y = vertex.x - centre_x, vertex.y - centre_y
            gaps.append(polygon_reach(radius, phase, math.atan2(offset_y, offset_x), sides)
                        - math.hypot(offset_x, offset_y))
    return min(gaps)


def part_separations(placed, posed, members):
    """Units between each pair of whole parts -> {(part, part): gap}, negative where their footprints touch.

    Nesting clearance cannot answer this, since a part not yet home sits outside the shell it will end up in by
    definition. `members` maps each part's bone to the bones its geometry hangs off.
    """
    reach, centre = {}, {}
    for part, bones in members.items():
        origin = posed[part].translation
        centre[part] = origin
        reach[part] = max(math.hypot(vertex.x - origin.x, vertex.y - origin.y)
                          for bone in bones for vertex in placed[bone])
    parts, gaps = list(members), {}
    for index, part in enumerate(parts):
        for other in parts[index + 1:]:
            gaps[(part, other)] = math.hypot(centre[part].x - centre[other].x,
                                             centre[part].y - centre[other].y) - reach[part] - reach[other]
    return gaps


def normalised_spin(rate, frames):
    """A turn integrated from a per-frame rate -> [fraction of the whole turn, per frame].

    Multiplying this by a whole number of a part's symmetry is what lets that part accelerate, stop and start
    however it likes and still land on a pose indistinguishable from the one it left, since only the total is
    ever scaled by it. Whatever units `rate(frame)` returns divide out.
    """
    cumulative, total = [0.0], 0.0
    for frame in range(1, frames + 1):
        total += rate(frame)
        cumulative.append(total)
    return [value / total for value in cumulative]


def accelerating_schedule(first, count, gap, ratio, minimum=1):
    """Frames a run of arrivals lands on, each gap `ratio` of the one before it -> [frame].

    A rhythm that tightens on its own, so it follows however many parts the rig turns out to carry rather than
    being keyed beat by beat.
    """
    frames, step, frame = [], float(gap), float(first)
    for _ in range(count):
        frames.append(int(round(frame)))
        frame += step
        step = max(minimum, step * ratio)
    return frames


def stride_order(count):
    """The order to wake a ring of `count` like features in -> [feature index], one stride apart.

    A stride coprime with the ring visits every feature exactly once, which reads as the shape waking all over,
    where stepping to the neighbour reads as a sweep round it.
    """
    stride = next((step for step in range(count // 2, 1, -1) if math.gcd(step, count) == 1), 1)
    return [(place * stride) % count for place in range(count)]


def scrambled_order(count, seed=17):
    """The order to fire a ring of `count` like features in with no pattern to it -> [feature index].

    A stride's gaps are all the same, which reads as a shape working correctly however far apart they are; only
    uneven gaps read as one firing them off wrongly. Shuffled rather than built from a rule, since every rule
    fine enough to state is regular enough to be seen. Fixed seed, so a clip is the same every time it is built.
    """
    order = list(range(count))
    random.Random(seed).shuffle(order)
    return order


def add_child_bones(skeletal_mesh_path, parent_bone, locations):
    """Add a bone per entry of locations ({bone name: component-space Vector}) under parent_bone, and commit.

    The modifier wants each transform in its parent's space. Commit the skeleton before any weighting — a vertex
    cannot be bound to a bone the mesh does not carry yet. Adding leaf bones keeps the commit off the path that
    raises a modal merge dialog.
    """
    mesh = unreal.load_asset(skeletal_mesh_path)
    modifier = unreal.SkeletonModifier()
    modifier.set_skeletal_mesh(mesh)
    skeleton = mesh.get_editor_property("skeleton")
    parent = APE.get_bone_pose(APE.get_reference_pose(skeleton), parent_bone, unreal.AnimPoseSpaces.WORLD)

    names, parents, transforms = [], [], []
    for name, location in locations.items():
        placed = unreal.Transform()
        placed.translation = location
        names.append(unreal.Name(name))
        parents.append(unreal.Name(parent_bone))
        transforms.append(unreal.MathLibrary.make_relative_transform(placed, parent))
    modifier.add_bones(names, parents, transforms)
    modifier.commit_skeleton_to_skeletal_mesh()

    unreal.EditorAssetLibrary.save_asset(skeletal_mesh_path)
    unreal.EditorAssetLibrary.save_loaded_asset(skeleton)


def bind_vertices(skeletal_mesh_path, assignments):
    """Bind vertices wholly to bones ({bone name: [vertex index]}) and commit.

    Replacing a vertex's weights drops its other influences, so a re-bound vertex stops following the bones it
    shared before and moves rigidly with its new one.
    """
    modifier = unreal.SkinWeightModifier()
    modifier.set_skeletal_mesh(unreal.load_asset(skeletal_mesh_path))
    for bone, indices in assignments.items():
        for index in indices:
            modifier.set_vertex_weights(index, {unreal.Name(bone): 1.0}, True)
    modifier.commit_weights_to_skeletal_mesh()
    unreal.EditorAssetLibrary.save_asset(skeletal_mesh_path)


def write_bone_tracks(sequence, skeleton_path, fps, frames, sampler, bracket="Author animation"):
    """Write bone tracks into `sequence` from `sampler`.

    sampler(frame, bone, rest_local_transform) -> (translation, rotation_quat, scale) for that frame.
    Returns the sequence. Verify the result with report_moving_bones, not the track list, and verify that it plays
    with playable_key_count.
    """
    rest = {name: local for name, (local, _) in reference_pose_table(skeleton_path).items()}

    controller = sequence.get_editor_property("controller")
    controller.open_bracket(bracket)
    controller.set_frame_rate(unreal.FrameRate(fps, 1))
    controller.set_number_of_frames(unreal.FrameNumber(frames))
    for bone, rest_local in rest.items():
        positions, rotations, scales = [], [], []
        for frame in range(frames + 1):  # a sequence holds one more key than its frame count
            translation, rotation, scale = sampler(frame, bone, rest_local)
            positions.append(translation)
            rotations.append(rotation)
            scales.append(scale)
        # Unconditional and result ignored: the adder reports failure for an existing track, and the
        # key setter reports success whether or not a track is there.
        controller.add_bone_curve(bone)
        controller.set_bone_track_keys(bone, positions, rotations, scales)
    controller.close_bracket()
    # The keys above are the raw model; this builds the data that actually plays back from them.
    unreal.AnimationLibrary.finalize_bone_animation(sequence)
    unreal.EditorAssetLibrary.save_asset(sequence.get_path_name().split(".")[0])
    return sequence


def playable_key_count(sequence):
    """Keys in the built data, which is one more than the frame count once the sequence plays back.

    Pose evaluation reads the raw model and reports the same whether or not that build ever happened, so this is
    what says a written sequence plays.
    """
    return sequence.get_editor_property("number_of_sampled_keys")


def build_montage(sequence, package_path, asset_name, section_names, section_starts, section_next,
                  slot_name="DefaultSlot"):
    """Create or load a montage playing the whole of `sequence` and give it its sections.

    The factory builds the slot track and its segment itself when handed a source animation. Sections have
    no scripting path, so they are written only where the C++ editor shim is compiled.
    """
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("target_skeleton", sequence.get_skeleton())
    factory.set_editor_property("source_animation", sequence)
    montage = get_or_create_asset(package_path, asset_name, unreal.AnimMontage, factory)
    if hasattr(unreal, "GeoAnimBuilderUtil"):
        util = unreal.get_default_object(unreal.GeoAnimBuilderUtil)
        util.set_montage_slot_segment(montage, sequence, slot_name)
        util.set_montage_sections(montage, [unreal.Name(n) for n in section_names], section_starts,
                                  [unreal.Name(n) for n in section_next])
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(package_path, asset_name))
    return montage


def montage_sections(montage):
    """Section names in order — the only part of a montage's layout Python reads back."""
    return [str(montage.get_section_name(i)) for i in range(montage.get_num_sections())]


def set_notify(anim, track_name, time, notify_class, properties, clear=True):
    """Put one notify of `notify_class` on `anim` at `time` and return it, `properties` set on the notify itself.

    A sequence's notify array is not readable as a property, so both halves of this go through the animation
    library. Clearing every track first drops the notifies on them, which is what lets a re-run replace rather
    than stack. A notify on a montage links to the segment beneath it, so add it only once the slot track is there.
    """
    if clear:
        unreal.AnimationLibrary.remove_all_animation_notify_tracks(anim)
    unreal.AnimationLibrary.add_animation_notify_track(anim, track_name)
    notify = unreal.AnimationLibrary.add_animation_notify_event(anim, track_name, time, notify_class)
    for name, value in properties.items():
        notify.set_editor_property(name, value)
    return notify


def notify_events(anim):
    """Every notify on `anim` -> [(trigger time, the notify object)].

    The time comes from the library rather than off the event, whose link fields are not exposed.
    """
    return [(unreal.AnimationLibrary.get_anim_notify_event_trigger_time(event),
             event.get_editor_property("notify"))
            for event in unreal.AnimationLibrary.get_animation_notify_events(anim)]


def _snapshot(transform):
    return (unreal.Vector(transform.translation.x, transform.translation.y, transform.translation.z),
            unreal.Vector(transform.scale3d.x, transform.scale3d.y, transform.scale3d.z),
            transform.rotation.rotator().yaw)


def _delta(current, base):
    return max((current[0] - base[0]).length(), (current[1] - base[1]).length() * 100.0,
               abs(current[2] - base[2]))


def _component_transforms(pose):
    """Every bone of an evaluated pose, copied out of its shared buffer. Transforms take the location first."""
    transforms = {}
    for bone in APE.get_bone_names(pose):
        component = APE.get_bone_pose(pose, str(bone), unreal.AnimPoseSpaces.WORLD)
        transforms[str(bone)] = unreal.Transform(component.translation, component.rotation.rotator(),
                                                 component.scale3d)
    return transforms


# --- Example calls — uncomment and adjust paths -------------------------------------------------

# Report which bones a sequence animates, and by how much
# print(report_moving_bones("/Game/Characters/Anim/Star/SK_Star_Sequence_Start"))

# Dump the rig: reference-pose transforms, and the hierarchy
# print(reference_pose_table("/Game/Characters/Meshes/Star/SK_Star"))
# print(bone_parents("/Game/Characters/Meshes/Star/SKM_Star"))

# Which bones actually carry the mesh, and where its features sit
# print(report_skin_weights("/Game/Characters/Meshes/Star/SKM_Star"))
# print(radial_vertex_rings("/Game/Characters/Meshes/Star/Star"))

# What a pose actually looks like: every vertex where the renderer would put it, frame by frame
# SEQUENCE, FPS = unreal.load_asset("/Game/Characters/Anim/Star/SK_Star_Idle"), 30
# frames = range(int(round(SEQUENCE.get_editor_property("sequence_length") * FPS)) + 1)
# for frame, positions in enumerate(skinned_vertex_positions(
#         "/Game/Characters/Meshes/Star/SKM_Star", SEQUENCE, [f / float(FPS) for f in frames])):
#     print(frame, max(math.hypot(p.x, p.y) for p in positions))

# Check what a clip actually looks like: does a feature clear its own face, and do nested parts collide
# MESH, SKELETON = "/Game/Characters/Meshes/HexBoss/SKM_HexBoss", "/Game/Characters/Meshes/HexBoss/SK_HexBoss"
# SEQUENCE, FPS = unreal.load_asset("/Game/Characters/Anim/HexBoss/SK_HexBoss_Idle"), 30
# groups = rigid_vertex_groups(MESH, SKELETON)
# layout = feature_directions(_component_transforms(APE.get_reference_pose(unreal.load_asset(SKELETON))), "_Spike_")
# for frame in range(0, 241, 12):
#     pose = APE.get_anim_pose_at_time(SEQUENCE, frame / float(FPS), unreal.AnimPoseEvaluationOptions())
#     local, posed = local_pose_table(pose), _component_transforms(pose)
#     placed = place_groups(groups, posed)
#     print(frame, local["HexOuter"][1], feature_protrusion(placed, posed, "HexOuter_Spike_0", "HexOuter"),
#           nested_clearance(placed, [b for b in groups if b.startswith("HexMid")], "HexOuter"))

# Schedule a shape assembling itself: when each part arrives, in what order, and how far apart they stay
# ORDER = stride_order(8)
# print(ORDER, accelerating_schedule(first=34, count=len(ORDER), gap=16.0, ratio=0.78, minimum=3))
# members = {part: [b for b in groups if b.startswith(part)] for part in ("HexOuter", "HexMid", "HexCore")}
# print(part_separations(place_groups(groups, posed), posed, members))

# A turn that accelerates and still lands on the part's own symmetry, whatever it did in between
# spun = normalised_spin(lambda frame: 1.0 if frame < 100 else 12.0, 220)
# print([round(60.0 * 12 * spun[f], 2) for f in (0, 100, 220)])

# Give each of a ring's features its own bone, so one can be moved without the others
# MESH, RING_PARENT, SLOT_COUNT = "/Game/Characters/Meshes/Star/SKM_Star", "apexes_outside", 8
# radius, phase, slots = radial_slots(vertex_weight_report(MESH), SLOT_COUNT)
# angles = {slot: phase + slot * 2.0 * math.pi / SLOT_COUNT for slot in slots}
# add_child_bones(MESH, RING_PARENT, {"spike_{}".format(slot): unreal.Vector(
#     radius * math.cos(angle), radius * math.sin(angle), 0.0) for slot, angle in angles.items()})
# bind_vertices(MESH, {"spike_{}".format(slot): indices for slot, indices in slots.items()})

# Author a sequence: a sampler scaling one bone up then back down over the clip
# SKELETON, PACKAGE, FPS, FRAMES, BONE = "/Game/Characters/Meshes/Star/SK_Star", "/Game/Characters/Anim/Star", 30, 30, "mid"
#
# def pulse(frame, bone, rest_local):
#     alpha = 1.0 - abs(frame / float(FRAMES) * 2.0 - 1.0)
#     scale = 1.0 + 2.0 * alpha if bone == BONE else 1.0
#     return (unreal.Vector(rest_local.translation.x, rest_local.translation.y, rest_local.translation.z),
#             rest_local.rotation.rotator().quaternion(),
#             unreal.Vector(rest_local.scale3d.x * scale, rest_local.scale3d.y * scale, rest_local.scale3d.z))
#
# factory = unreal.AnimSequenceFactory()
# factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON))
# target = get_or_create_asset(PACKAGE, "SK_Example_Pulse", unreal.AnimSequence, factory)
# write_bone_tracks(target, SKELETON, FPS, FRAMES, pulse)

# Wrap it in a montage, then confirm the sections exist before anything is pointed at it
# montage = build_montage(target, PACKAGE, "SK_Example_Pulse_Montage",
#                         ["Start", "Fire", "End"], [0.0, 0.1, 0.7], ["Fire", "End", "None"])
# print(montage_sections(montage))

# Fire an effect part-way through it — added after the slot track, so the notify links to the segment
# set_notify(montage, "1", 0.1, unreal.AnimNotify_PlayNiagaraEffect,
#            {"template": unreal.load_asset("/Game/Art/VFX/Assets/NS_DeathDebris")})
# print(notify_events(montage))
