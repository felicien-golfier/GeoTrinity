"""Cat_Reassemble: the Cat lies splashed apart on the floor, stirs, gathers itself part by part, stretches, shakes and
settles onto the first pose of Cat_Idle. Montage AM_Cat_Reassemble, DefaultSlot, no blend-in (it starts far from any pose).

    Splat    every part flat on the floor, flung round the seat, none touching, trembling once then still
    Stir     eyes pulse first, then the other parts twitch, each at its own moment
    Gather   parts come home one at a time on an accelerating schedule (Body, Head, ears, face, feet, hands); the tail
             chain is dragged after the Body; each hops (scales up mid-flight), overshoots and settles
    Stretch  hands forward, feet back, the Body long; held with a tremble; released past rest
    Settle   a head shake, an ear flick, the idle's first pose

Every bone is placed in world space: a splat state blended into the assembled state by the bone's own progress, then
turned into a local transform against its parent's world transform at that frame. A part waiting on the floor therefore
stays put while its parent comes home. The maths is plain Python (quaternions in UE's convention, checked against the
engine's own conversion before writing).

Run AFTER AI/Python/Mesh/rig_cat.py and cat_idle.py, via mcp-unreal execute_script. Re-runnable: rewrites the sequence and
montage in place. Report: AI/Output/cat_Cat_Reassemble.json.
"""
import math
import random

import unreal

_CLIP_PATH = unreal.Paths.project_dir() + "AI/Python/Anim/Cat/cat_clip.py"
CLIP = {}
exec(compile(open(_CLIP_PATH, encoding="utf-8").read(), _CLIP_PATH, "exec"), CLIP)
TOOLKIT, attack_release, curve, run, smooth = (CLIP["TOOLKIT"], CLIP["attack_release"], CLIP["curve"], CLIP["run"],
                                               CLIP["smooth"])

_IDLE_PATH = unreal.Paths.project_dir() + "AI/Python/Anim/Cat/cat_idle.py"
IDLE = {"__name__": "cat_idle_library"}
exec(compile(open(_IDLE_PATH, encoding="utf-8").read(), _IDLE_PATH, "exec"), IDLE)

FRAMES = 240  # 8 s
MONTAGE = "AM_Cat_Reassemble"
SEED = 7
SEAT = (-20.0, 0.0)  # centre of the splat on the floor
SPLAT_RADIUS = 230.0
SPLAT_GAP = 4.0  # units kept between two parts' reach
SPLAT_SCALE = 1.15  # flattened parts spread a little
TUMBLE_DEGREES = 12.0
TREMBLE = (4, 10, 3, 2.5)  # start frame, frames it lasts, period in frames, degrees (all parts, damped)
STIR_FIRST = {"EyeLeft": 24, "EyeRight": 27}  # frame each part's twitch starts; the rest are drawn from STIR_RANGE
STIR_RANGE = (28, 46)
STIR_DEGREES, STIR_PERIOD, STIR_LENGTH = 9.0, 8, 16
GATHER_ORDER = ["Body", "Head", "EarLeft", "EarRight", "EyeLeft", "EyeRight", "Nose", "Mouth0", "Mouth1", "FootLeft",
                "FootRight", "HandLeft", "HandRight"]
GATHER_FIRST, GATHER_GAP, GATHER_RATIO, GATHER_MIN_GAP = 76, 10.0, 0.85, 3
TAIL_DRAG = 3  # frames each tail segment arrives after the one before it
TRAVEL = 14  # frames a part is in flight
OVERSHOOT = 1.2  # ease-out-back strength
HOP_SCALE = 0.25  # extra size at mid-flight
STRETCH = [(150, 0.0), (172, 1.0), (196, 1.0), (204, -0.12), (214, 0.0)]  # how far into the stretch, per frame
TREMBLE_HOLD = (172, 196, 3, 0.8)  # start, end, period, degrees
SETTLE = (206, 30)  # start frame, frames to the idle's first pose
SHAKE = (210, 20, 10, 14.0)  # start, frames, period, degrees
EAR_FLICK = ("EarRight", 214, 15.0)
FLAT_BONES = ["Body"] + ["Tail{}".format(index) for index in range(9)]  # lie as they are; the rest lie on their faces


def euler_quat(roll, pitch, yaw):
    """FRotator::Quaternion: (x, y, z, w) of roll, pitch, yaw in degrees."""
    sr, cr = math.sin(math.radians(roll) / 2.0), math.cos(math.radians(roll) / 2.0)
    sp, cp = math.sin(math.radians(pitch) / 2.0), math.cos(math.radians(pitch) / 2.0)
    sy, cy = math.sin(math.radians(yaw) / 2.0), math.cos(math.radians(yaw) / 2.0)
    return (cr * sp * sy - sr * cp * cy, -cr * sp * cy - sr * cp * sy, cr * cp * sy - sr * sp * cy,
            cr * cp * cy + sr * sp * sy)


def quat_multiply(a, b):
    return (a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1], a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0],
            a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3], a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2])


def quat_inverse(q):
    return (-q[0], -q[1], -q[2], q[3])


def quat_rotate(q, v):
    cross = lambda a, b: (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
    t = tuple(2.0 * c for c in cross(q[:3], v))
    u = cross(q[:3], t)
    return tuple(v[i] + q[3] * t[i] + u[i] for i in range(3))


def quat_slerp(a, b, t):
    dot = sum(x * y for x, y in zip(a, b))
    if dot < 0.0:
        b, dot = tuple(-x for x in b), -dot

    if dot > 0.9995:
        mixed = tuple(x + (y - x) * t for x, y in zip(a, b))
    else:
        angle = math.acos(dot)
        mixed = tuple((math.sin((1.0 - t) * angle) * x + math.sin(t * angle) * y) / math.sin(angle)
                      for x, y in zip(a, b))
    length = math.sqrt(sum(x * x for x in mixed))
    return tuple(x / length for x in mixed)


def world_of(local, parent):
    """A (position, quaternion, scale) transform under its parent's world transform."""
    scaled = tuple(parent[2][i] * local[0][i] for i in range(3))
    turned = quat_rotate(parent[1], scaled)
    return (tuple(parent[0][i] + turned[i] for i in range(3)), quat_multiply(parent[1], local[1]),
            tuple(parent[2][i] * local[2][i] for i in range(3)))


def local_of(world, parent):
    """The inverse of world_of."""
    turned = quat_rotate(quat_inverse(parent[1]), tuple(world[0][i] - parent[0][i] for i in range(3)))
    return (tuple(turned[i] / parent[2][i] for i in range(3)), quat_multiply(quat_inverse(parent[1]), world[1]),
            tuple(world[2][i] / parent[2][i] for i in range(3)))


IDENTITY = ((0.0, 0.0, 0.0), (0.0, 0.0, 0.0), (1.0, 1.0, 1.0))  # translation, euler degrees, scale


def mixed(a, b, t):
    """Delta triples blended: a at 0, b at 1."""
    return tuple(tuple(x + (y - x) * t for x, y in zip(part_a, part_b)) for part_a, part_b in zip(a, b))


def assembled_deltas(frame):
    """Deltas on rest of the assembled Cat at `frame`: rest until the stretch, then stretch, shake, settle onto idle."""
    stretch = curve(frame, STRETCH)
    tremble_start, tremble_end, period, degrees = TREMBLE_HOLD
    tremble = degrees * math.sin(2.0 * math.pi * (frame - tremble_start) / period) \
        if tremble_start <= frame < tremble_end else 0.0
    shake_start, shake_length, shake_period, shake_degrees = SHAKE
    shake = shake_degrees * math.sin(2.0 * math.pi * (frame - shake_start) / shake_period) \
        * (1.0 - (frame - shake_start) / float(shake_length)) if shake_start <= frame < shake_start + shake_length else 0.0

    deltas = {"Body": ((-12.0 * stretch, 0, 0), (0, 0, tremble), (1.0 + 0.35 * stretch, 1.0 - 0.1 * stretch, 1.0)),
              "Head": ((10.0 * stretch, 0, 0), (0, -25.0 * stretch, shake), (1, 1, 1)),
              "FootLeft": ((-10.0 * stretch, 0, 0), (0, 0, 0), (1, 1, 1)),
              "FootRight": ((-10.0 * stretch, 0, 0), (0, 0, 0), (1, 1, 1)),
              "HandLeft": ((28.0 * stretch + 0.5 * tremble, 0, 0), (0, 0, 0), (1, 1, 1)),
              "HandRight": ((28.0 * stretch - 0.5 * tremble, 0, 0), (0, 0, 0), (1, 1, 1))}
    for index in range(9):
        deltas["Tail{}".format(index)] = ((0, 0, 0), (0, 0, 6.0 * stretch), (1, 1, 1))

    ear, flick_start, flick_degrees = EAR_FLICK
    deltas[ear] = ((0, 0, 0), (attack_release(frame, flick_start, 4, 6, 10, flick_degrees), 0, 0), (1, 1, 1))

    settle_start, settle_length = SETTLE
    weight = smooth((frame - settle_start) / float(settle_length))
    idle = IDLE["pose"](0)
    return {bone: mixed(deltas.get(bone, IDENTITY), idle.get(bone, IDENTITY), weight)
            for bone in set(deltas) | set(idle)}


def splat_layout(bones, groups, rng):
    """{bone: (position, quaternion, scale, reach)} lying on the floor, no two parts' reaches touching."""
    reach = {bone: SPLAT_SCALE * max(math.sqrt(sum(c * c for c in v)) for v in map(lambda p: (p.x, p.y, p.z), groups[bone]))
             for bone in bones}
    placed, layout = [], {}
    for bone in sorted(bones, key=lambda b: -reach[b]):
        for _ in range(4000):
            angle, distance = rng.uniform(0.0, 2.0 * math.pi), (SPLAT_RADIUS - reach[bone]) * math.sqrt(rng.random())
            x, y = SEAT[0] + distance * math.cos(angle), SEAT[1] + distance * math.sin(angle)
            if all(math.hypot(x - px, y - py) >= reach[bone] + pr + SPLAT_GAP for px, py, pr in placed):
                break
        else:
            raise RuntimeError("no room on the floor for {}".format(bone))

        placed.append((x, y, reach[bone]))
        flat = bone in FLAT_BONES
        pitch = 0.0 if flat else 90.0 + rng.uniform(-TUMBLE_DEGREES, TUMBLE_DEGREES)
        up = max(abs(p.z if flat else p.x) for p in groups[bone])
        floor = SPLAT_SCALE * up + 0.25 * reach[bone] * (0.0 if flat else 1.0) + 1.5
        layout[bone] = ((x, y, floor), euler_quat(0.0, pitch, rng.uniform(-180.0, 180.0)),
                        (SPLAT_SCALE,) * 3, reach[bone])
    return layout


def gather_windows(bones):
    """{bone: (first frame in flight, arrival frame)}: the schedule, with the tail dragged after the Body."""
    frames, step, frame = [], GATHER_GAP, float(GATHER_FIRST)
    for _ in GATHER_ORDER:
        frames.append(int(round(frame)))
        frame += step
        step = max(GATHER_MIN_GAP, step * GATHER_RATIO)
    arrivals = dict(zip(GATHER_ORDER, frames))
    for index in range(9):
        arrivals["Tail{}".format(index)] = arrivals["Body"] + TAIL_DRAG * (index + 1)
    assert set(arrivals) == set(bones), "the gather schedule must name every bone"
    assert max(arrivals.values()) < STRETCH[0][0] - 1, "the gather must end before the stretch begins"
    return {bone: (arrival - TRAVEL, arrival) for bone, arrival in arrivals.items()}


def stir_state(frame, bone, state, stir_frame):
    """The splat state with the tremble, the part's own twitch and (eyes) its pulse on it."""
    position, rotation, scale, _ = state
    start, length, period, degrees = TREMBLE
    yaw = degrees * math.sin(2.0 * math.pi * (frame - start) / period) * (1.0 - (frame - start) / float(length)) \
        if start <= frame < start + length else 0.0
    if stir_frame <= frame < stir_frame + STIR_LENGTH:
        yaw += STIR_DEGREES * math.sin(2.0 * math.pi * (frame - stir_frame) / STIR_PERIOD) \
            * (1.0 - (frame - stir_frame) / float(STIR_LENGTH))

    pulse = 1.0 - 0.35 * attack_release(frame, stir_frame, 3, 3, 7) if bone in STIR_FIRST else 1.0
    return (position, quat_multiply(euler_quat(0.0, 0.0, yaw), rotation), tuple(s * pulse for s in scale))


def ease_out_back(t):
    c3 = OVERSHOOT + 1.0
    return 1.0 + c3 * (t - 1.0) ** 3 + OVERSHOOT * (t - 1.0) ** 2


def make_pose():
    table = TOOLKIT["reference_pose_table"](CLIP["SKELETON_PATH"])
    parents = TOOLKIT["bone_parents"](CLIP["MESH_PATH"])
    rest = {bone: (local.translation.x, local.translation.y, local.translation.z) for bone, (local, _) in table.items()}
    bones = list(table)
    groups = TOOLKIT["rigid_vertex_groups"](CLIP["MESH_PATH"], CLIP["SKELETON_PATH"])
    rng = random.Random(SEED)
    parts = [bone for bone in bones if bone in groups]
    layout = splat_layout(parts, groups, rng)
    windows = gather_windows(parts)
    stir = {bone: STIR_FIRST.get(bone, rng.randint(STIR_RANGE[0], min(STIR_RANGE[1], windows[bone][0] - STIR_LENGTH)))
            for bone in parts}

    def pose(frame):
        deltas = assembled_deltas(frame)
        world, final, deltas_out = {}, {}, {}
        for bone in bones:
            parent = parents[bone]
            delta = deltas.get(bone, IDENTITY)
            local = (tuple(rest[bone][i] + delta[0][i] for i in range(3)), euler_quat(*delta[1]), delta[2])
            final[bone] = world_of(local, final[parent]) if parent != "None" else local

            if bone not in layout:
                world[bone] = final[bone]
                deltas_out[bone] = (tuple(local[0][i] - rest[bone][i] for i in range(3)), local[1], local[2])
                continue

            start, arrival = windows[bone]
            flight = max(0.0, min(1.0, (frame - start) / float(arrival - start)))
            progress = ease_out_back(flight) if frame >= start else 0.0
            splat = stir_state(frame, bone, layout[bone], stir[bone])
            hop = 1.0 + HOP_SCALE * math.sin(math.pi * flight)
            blended = (tuple(a + (b - a) * progress for a, b in zip(splat[0], final[bone][0])),
                       quat_slerp(splat[1], final[bone][1], max(0.0, min(1.0, progress))),
                       tuple((a + (b - a) * progress) * hop for a, b in zip(splat[2], final[bone][2])))
            world[bone] = blended
            relative = local_of(blended, world[parent]) if parent != "None" else blended
            deltas_out[bone] = (tuple(relative[0][i] - rest[bone][i] for i in range(3)), relative[1], relative[2])
        return deltas_out
    return pose, windows


def add_montage(sequence):
    montage = TOOLKIT["build_montage"](sequence, CLIP["ANIM_PACKAGE"], MONTAGE, ["Default"], [0.0], ["None"])
    blend_in = montage.get_editor_property("blend_in").copy()
    blend_in.set_editor_property("blend_time", 0.0)
    montage.set_editor_property("blend_in", blend_in)
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(CLIP["ANIM_PACKAGE"], MONTAGE))

    idle = unreal.load_asset(CLIP["ANIM_PACKAGE"] + "/Cat_Idle")
    return {"montage_sections": TOOLKIT["montage_sections"](montage),
            "gap_to_idle_first_pose": CLIP["closure_gap"](CLIP["local_snapshot"](idle, 0.0),
                                                           CLIP["local_snapshot"](sequence, FRAMES / float(CLIP["FPS"])))}


def main():
    quaternion = unreal.Rotator(roll=20.0, pitch=35.0, yaw=50.0).quaternion()
    mine = euler_quat(20.0, 35.0, 50.0)
    assert max(abs(a - b) for a, b in zip(mine, (quaternion.x, quaternion.y, quaternion.z, quaternion.w))) < 1e-5, \
        "euler_quat no longer matches the engine's convention"
    pose, windows = make_pose()
    run("Cat_Reassemble", FRAMES, pose, step=2, after=lambda sequence: dict(add_montage(sequence), gather=windows),
        families=CLIP["PARTS"])


main()
