"""The Cat's two walk loops, blended by its speed in ABP_Cat: Cat_Walk_Prowl (slow), Cat_Walk_Fast (fast), same length and pattern: only amplitudes differ. One evil-face flash a stride (cat_face.py), the least a loop can carry.

Why they are not bumpy: nothing scales (no bob), each paw rides a plain sine (no speed jump where it lifts and lands),
the body yaws once a stride (not twice), and every wave has one cycle or two, never a stack. Four-beat order: left foot, left hand,
right foot, right hand, a quarter stride apart. One loop is one stride; every term is a
whole number of cycles.

Prowl and Fast are the samples of the blend space at PROWL_SPEED and FAST_SPEED (cm/s; cat_anim_blueprint.py reads
them by loading this file with another __name__): Prowl up to PROWL_SPEED, blended into Fast up to FAST_SPEED, the
most the Cat walks at. Chosen, not matched to the strides: the feet slide off the floor at speeds away from the clips'.

Run via mcp-unreal execute_script. Re-runnable. Reports: AI/Output/cat_Cat_Walk_<name>.json.
"""
import math

import unreal

_CLIP_PATH = unreal.Paths.project_dir() + "AI/Python/Cat/Anim/cat_clip.py"
CLIP = {}
exec(compile(open(_CLIP_PATH, encoding="utf-8").read(), _CLIP_PATH, "exec"), CLIP)
wander, run, EAR_DETACH = CLIP["wander"], CLIP["run"], CLIP["EAR_DETACH"]

_FACE_PATH = unreal.Paths.project_dir() + "AI/Python/Cat/Anim/cat_face.py"
FACE = {}
exec(compile(open(_FACE_PATH, encoding="utf-8").read(), _FACE_PATH, "exec"), FACE)

PAW_PHASES = {"FootLeft": 0.0, "HandLeft": 0.25, "FootRight": 0.5, "HandRight": 0.75}  # turns
TAIL_COUNT = 9
HEAD_PEEK_DEGREES = 20.0  # FACE_PEEK_DEGREES of CatAnimation.md: the face reads from above
EVIL_FACE = (20, 4)  # start frame, frames held, in the stride
TAIL_STRAIGHTEN_DEGREES = 8.0  # yaw per segment from Tail1 on, cancels the mesh's curl
TAIL_LAG = 3  # frames per segment

# Per walk: stride frames, paw reach (units, hands / feet), body yaw and side slide (amplitude), head counter-yaw (of the
# body's, read late), tail: base pitch (negative raises it), hook on the last three, yaw wave, pitch wave (degrees at the
# base, grown to the tip), balance (of the body's yaw, inverted)
WALKS = {
    "Prowl": {"frames": 56, "hand": 5.0, "foot": 6.5, "yaw": 2.0, "side": 0.8, "head": -0.8,
              "tail": (-6.0, 0.0, 7.0, 0.0, 0.6)},
    "Fast": {"frames": 56, "hand": 8.0, "foot": 11.0, "yaw": 4.5, "side": 1.8, "head": -0.5,
              "tail": (-60.0, 16.0, 6.0, 2.5, 1.0)},
}

PROWL_SPEED = 150.0
FAST_SPEED = 300.0


def make_pose(spec):
    frames = spec["frames"]
    base_pitch, hook, yaw_wave, pitch_wave, balance = spec["tail"]
    body_yaw = [(1, spec["yaw"], 0.1)]
    head_lag = frames // 12

    def pose(frame):
        deltas = {"Body": ((0, wander(frame, frames, [(1, spec["side"], 0.25)]), 0), (0, 0, wander(frame, frames, body_yaw)),
                           (1, 1, 1)),
                  "Head": ((0, 0, 0), (0, HEAD_PEEK_DEGREES, spec["head"] * wander(frame - head_lag, frames, body_yaw)), (1, 1, 1))}
        for paw, phase in PAW_PHASES.items():
            reach = spec["hand"] if paw.startswith("Hand") else spec["foot"]
            deltas[paw] = ((reach * math.cos(2.0 * math.pi * (frame / float(frames) - phase)), 0, 0), (0, 0, 0), (1, 1, 1))

        for ear in ("EarLeft", "EarRight"):
            deltas[ear] = (EAR_DETACH[ear], (0, 0, 0), (1, 1, 1))

        for index in range(TAIL_COUNT):
            late = frame - TAIL_LAG * (index + 1)
            grown = 1.0 + 0.3 * index
            yaw = wander(late, frames, [(1, yaw_wave, 0.0)]) * grown
            pitch = wander(late, frames, [(2, pitch_wave, 0.4)]) * grown
            if index == 0:
                yaw -= balance * wander(late, frames, body_yaw)
                pitch += base_pitch
            else:
                yaw += TAIL_STRAIGHTEN_DEGREES

            if index >= TAIL_COUNT - 3:
                pitch += hook
            deltas["Tail{}".format(index)] = ((0, 0, 0), (0, pitch, yaw), (1, 1, 1))
        return FACE["apply_evil"](deltas, frame, *EVIL_FACE)
    return pose


if __name__ == "__main__":
    allowed = {"Body|Head", "Body|Tail0"} | {"Tail{}|Tail{}".format(i, i + 1) for i in range(TAIL_COUNT - 1)}
    for name, spec in WALKS.items():
        run("Cat_Walk_" + name, spec["frames"], make_pose(spec), allowed_overlaps=allowed)

