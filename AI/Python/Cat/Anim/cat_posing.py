"""World-space posing of the Cat for the clips that start and end on Cat_Idle frame 0 (stretches, idle variants).

A clip gives `target(frame, weight)`: a spec of where the parts go in the world; this turns it into deltas on rest in
each bone's own space and eases it in from the idle's first pose and back out. Nothing is scaled except the Body (a
stretch along its own X) and the eyes; the Body's children are counter-scaled so they stay unscaled.

spec = {
    "body": (world offset, euler (roll, pitch, yaw), stretch along X),   the front stays put, the rear grows back
    "head": (world offset from rest, world euler),
    "paws": {"HandLeft" | "HandRight" | "FootLeft" | "FootRight": (world offset from rest, world euler)},
    "tail0": world euler of the tail's base,
    "tail": function(index) -> (pitch, yaw) of Tail<index> (1..8) relative to its parent,
    "eyes": (left, right) Z scale,
    "extra": {bone: (translation, euler)} optional, local deltas for the parts the spec does not place (ears, ...),
}
Helpers for clip scripts: `ease` (the 0..1 weight), `shake_between`, `paws`, `run_clips`, the idle's head pose and the tail
looks, `twist_spec` (the Twist stretch pose, shared by Cat_Stretch_Twist and Cat_Reassemble's stretch beat).
"""
import math

import unreal

_CLIP_PATH = unreal.Paths.project_dir() + "AI/Python/Cat/Anim/cat_clip.py"
CLIP = {}
exec(compile(open(_CLIP_PATH, encoding="utf-8").read(), _CLIP_PATH, "exec"), CLIP)
curve, run = CLIP["curve"], CLIP["run"]

_IDLE_PATH = unreal.Paths.project_dir() + "AI/Python/Cat/Anim/cat_idle.py"
IDLE = {"__name__": "cat_idle_library"}
exec(compile(open(_IDLE_PATH, encoding="utf-8").read(), _IDLE_PATH, "exec"), IDLE)

TAIL_COUNT = 9
BODY_REST = (-43.0, 0.0, 68.25)  # where the rig's bones rest in the world (AI/Output/cat_gen.json)
BODY_HALF_LENGTH = 35.0
FACE_PEEK_DEGREES = 20.0  # least head pitch of any clip: the face reads from the camera above
REST = {"Head": (15.0, 0.0, 69.0), "HandLeft": (16.0, -52.0, 62.0), "HandRight": (16.0, 52.0, 62.0),
        "FootLeft": (-56.0, -26.0, 44.0), "FootRight": (-56.0, 26.0, 44.0), "Tail0": (-84.5, 0.0, 62.0)}
SIT_HEAD = ((0.0, 0.0, IDLE["HEAD_LIFT"]), (0.0, IDLE["FACE_PITCH_DEGREES"], 0.0))  # the idle's head, lifted and pitched up
SIT_PAWS = {paw: ((0.0, 0.0, 0.0), (0.0, 0.0, 0.0)) for paw in ("HandLeft", "HandRight", "FootLeft", "FootRight")}
WRAPPED_TAIL = lambda index: (0.0, IDLE["TAIL_CURL_DEGREES"])  # round the feet
STRAIGHT_TAIL = lambda index: (0.0, 8.0)  # cancels the mesh's curl (TAIL_TURN_DEGREES)


def quat(euler):
    """(x, y, z, w) of an euler (roll, pitch, yaw) in degrees."""
    q = unreal.Rotator(roll=euler[0], pitch=euler[1], yaw=euler[2]).quaternion()
    return (q.x, q.y, q.z, q.w)


def multiply(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz)


def conjugate(q):
    return (-q[0], -q[1], -q[2], q[3])


def rotate(q, v):
    x, y, z, _ = multiply(multiply(q, (v[0], v[1], v[2], 0.0)), conjugate(q))
    return (x, y, z)


def lerp(a, b, t):
    return tuple(x + (y - x) * t for x, y in zip(a, b))


def nlerp(a, b, t):
    """a at 0, b at 1, along the shorter way."""
    if sum(x * y for x, y in zip(a, b)) < 0.0:
        b = tuple(-x for x in b)
    mixed = lerp(a, b, t)
    length = math.sqrt(sum(x * x for x in mixed))
    return tuple(x / length for x in mixed)


def add(a, b):
    return tuple(x + y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def posed(spec):
    """{bone: (translation, quaternion, scale)} deltas on rest for a spec."""
    body_offset, body_euler, stretch = spec["body"]
    body_q = quat(body_euler)
    body_at = add(add(BODY_REST, body_offset), rotate(body_q, (-(stretch - 1.0) * BODY_HALF_LENGTH, 0.0, 0.0)))
    inverse = conjugate(body_q)

    def under_body(bone, world_offset, world_euler):
        """Translation, rotation and scale in the Body's space that put `bone` at its rest plus `world_offset`, turned
        to `world_euler`, whatever the Body does round it. The counter-scale is exact for a turn about X and the best
        diagonal for any other."""
        local_turn = multiply(inverse, quat(world_euler))
        place = rotate(inverse, sub(add(REST[bone], world_offset), body_at))
        place = (place[0] / stretch, place[1], place[2])
        along = rotate(conjugate(local_turn), (1.0, 0.0, 0.0))
        counter = tuple(1.0 + (1.0 / stretch - 1.0) * axis * axis for axis in along)
        return sub(place, sub(REST[bone], BODY_REST)), local_turn, counter

    deltas = {"Body": (sub(body_at, BODY_REST), body_q, (stretch, 1.0, 1.0))}
    deltas["Head"] = under_body("Head", *spec["head"])
    for paw, (offset, euler) in spec["paws"].items():
        deltas[paw] = under_body(paw, offset, euler)
    _, tail_turn, tail_scale = under_body("Tail0", (0.0, 0.0, 0.0), spec["tail0"])
    deltas["Tail0"] = ((0.0, 0.0, 0.0), tail_turn, tail_scale)
    for index in range(1, TAIL_COUNT):
        pitch, yaw = spec["tail"](index)
        deltas["Tail{}".format(index)] = ((0.0, 0.0, 0.0), quat((0.0, pitch, yaw)), (1.0, 1.0, 1.0))
    left, right = spec["eyes"]
    deltas["EyeLeft"] = ((0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0), (1.0, 1.0, left))
    deltas["EyeRight"] = ((0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0), (1.0, 1.0, right))
    for bone, (translation, euler) in spec.get("extra", {}).items():
        deltas[bone] = (translation, quat(euler), (1.0, 1.0, 1.0))
    return deltas


def ease(frame, frames, ease_in, ease_out):
    """0 on the idle's first pose, 1 from `ease_in` to `ease_out`, 0 again at `frames`."""
    return curve(frame, [(0, 0.0), (ease_in, 1.0), (ease_out, 1.0), (frames, 0.0)])


def make_pose(target, frames, ease_in, ease_out):
    """pose(frame) of a clip: `target(frame, weight)` (a spec) eased in from, and back to, Cat_Idle frame 0."""
    idle = IDLE["pose"](0)

    def pose(frame):
        weight = ease(frame, frames, ease_in, ease_out)
        goal = posed(target(frame, weight))
        result = {}
        for bone in set(idle) | set(goal):
            at_idle = idle.get(bone, ((0, 0, 0), (0, 0, 0), (1, 1, 1)))
            at_idle = (at_idle[0], quat(at_idle[1]), at_idle[2])
            at_goal = goal.get(bone, at_idle)
            result[bone] = (lerp(at_idle[0], at_goal[0], weight), nlerp(at_idle[1], at_goal[1], weight),
                            lerp(at_idle[2], at_goal[2], weight))
        return result
    return pose


def shake_between(frame, first, last):
    """-1..1 shaking from `first` to `last` (a fine shake through a held pose), 0 outside."""
    held = curve(frame, [(first - 4, 0.0), (first + 6, 1.0), (last - 6, 1.0), (last + 4, 0.0)])
    return held * math.sin(2.0 * math.pi * frame / 8.0)


def paws(left_hand, right_hand, left_foot, right_foot, shake, yaw=0.0):
    """Paw offsets (world) turned to `yaw` (the Body's, so a turned Body keeps them unskewed); the hands shake."""
    shaken = lambda hand: (hand[0] + 0.9 * shake, hand[1] + 0.5 * shake, hand[2])
    return {"HandLeft": (shaken(left_hand), (0, 0, yaw)), "HandRight": (shaken(right_hand), (0, 0, yaw)),
            "FootLeft": (left_foot, (0, 0, yaw)), "FootRight": (right_foot, (0, 0, yaw))}


def twist_spec(shake):
    """The Twist stretch: lying curved, front swung to one side, hands across, feet kicked the other way, head looking
    back, tail curled; `shake` (-1..1, `shake_between`) is the fine tremble on top."""
    return sit_spec(
        body=((0, 0, -20), (0, 0, 25 + 0.7 * shake), 1.25), head=((-2, 18, -28), (0, 80, -80 + 0.5 * shake)),
        paws=paws((40, 44, -16), (40, 30, -16), (-20, -16, -16), (-14, -22, -16), shake),
        tail0=(0, 0, 20), tail=lambda i: (0.0, -14.0 + 0.4 * i * shake), eyes=(0.2, 0.2))


def sit_spec(**changes):
    """The idle's sitting spec with `changes` on top."""
    spec = {"body": ((0.0, 0.0, 0.0), (0.0, 0.0, 0.0), 1.0), "head": SIT_HEAD, "paws": dict(SIT_PAWS),
            "tail0": (0.0, 0.0, IDLE["TAIL_CURL_DEGREES"]), "tail": WRAPPED_TAIL, "eyes": (1.0, 1.0)}
    spec.update(changes)
    return spec


def run_clips(clips, after=None):
    """Write and check each (name, frames, ease_in, ease_out, target) of `clips`; `after(sequence)` as in `run`."""
    allowed = {"Body|Head", "Body|Tail0"} | {"Tail{}|Tail{}".format(i, i + 1) for i in range(TAIL_COUNT - 1)}
    for name, frames, ease_in, ease_out, target in clips:
        run(name, frames, make_pose(target, frames, ease_in, ease_out), step=4, after=after, allowed_overlaps=allowed)
