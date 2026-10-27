"""The Cat's gallop loop, Cat_Run: a rotary gallop, not a faster walk (AI/Tutorial/CatAnimation.md, "Run").

Two beats a stride: the hands reach together while the feet drive back together (Body stretched along X, the one scaled
part), then the paws swap sides (Body gathered). Head peeks up with a small counter-yaw, ears pinned back, tail streaming
low and level with a small flutter, one evil-face flash (cat_face.py). Every term is a whole number of cycles over the stride, so it loops.

RUN_SPEED is the blend space sample position when ABP_Cat adds the run (cm/s; cat_anim_blueprint.py would read it by
loading this file with another __name__).

Run via mcp-unreal execute_script. Re-runnable. Report: AI/Output/cat_Cat_Run.json.
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

FRAMES = 24
RUN_SPEED = 600.0  # cm/s

PAW_PHASES = {"HandLeft": 0.0, "HandRight": 0.04, "FootLeft": 0.5, "FootRight": 0.54}  # turns: hands, then feet
HAND_REACH = 9.0
FOOT_REACH = 12.0
BODY_STRETCH = 0.2  # of the Body's length, +/- along X
BODY_YAW = 3.0  # degrees, once a stride
HEAD_PEEK_DEGREES = 20.0  # FACE_PEEK_DEGREES of CatAnimation.md
HEAD_COUNTER_YAW = -0.6  # of the body's yaw, read late
HEAD_LAG = 2  # frames
EAR_PINNED_BACK = -1.5  # X, on top of EAR_DETACH
EVIL_FACE = (4, 0)  # start frame, frames held
TAIL_COUNT = 9
TAIL_STRAIGHTEN_DEGREES = 8.0  # yaw per segment from Tail1 on, cancels the mesh's curl
TAIL_LAG = 2  # frames per segment
TAIL_YAW_WAVE = 6.0  # degrees at the base, grown to the tip
TAIL_PITCH_WAVE = 3.0
TAIL_BALANCE = 0.5  # of the body's yaw, inverted


def pose(frame):
    body_yaw = [(1, BODY_YAW, 0.1)]
    stretch = 1.0 + BODY_STRETCH * math.cos(2.0 * math.pi * frame / FRAMES)
    counter_scale = (1.0 / stretch, 1, 1)  # children keep their size
    deltas = {"Body": ((0, 0, 0), (0, 0, wander(frame, FRAMES, body_yaw)), (stretch, 1, 1)),
              "Head": ((0, 0, 0), (0, HEAD_PEEK_DEGREES, HEAD_COUNTER_YAW * wander(frame - HEAD_LAG, FRAMES, body_yaw)),
                       counter_scale)}
    for paw, phase in PAW_PHASES.items():
        reach = HAND_REACH if paw.startswith("Hand") else FOOT_REACH
        deltas[paw] = ((reach * math.cos(2.0 * math.pi * (frame / float(FRAMES) - phase)), 0, 0), (0, 0, 0),
                       counter_scale)

    for ear, offset in EAR_DETACH.items():
        deltas[ear] = ((offset[0] + EAR_PINNED_BACK, offset[1], offset[2]), (0, 0, 0), (1, 1, 1))

    for index in range(TAIL_COUNT):
        late = frame - TAIL_LAG * (index + 1)
        grown = 1.0 + 0.3 * index
        yaw = wander(late, FRAMES, [(1, TAIL_YAW_WAVE, 0.0)]) * grown
        pitch = wander(late, FRAMES, [(2, TAIL_PITCH_WAVE, 0.4)]) * grown
        if index == 0:
            yaw -= TAIL_BALANCE * wander(late, FRAMES, body_yaw)
        else:
            yaw += TAIL_STRAIGHTEN_DEGREES

        deltas["Tail{}".format(index)] = ((0, 0, 0), (0, pitch, yaw), counter_scale if index == 0 else (1, 1, 1))
    return FACE["apply_evil"](deltas, frame, *EVIL_FACE)


if __name__ == "__main__":
    allowed = {"Body|Head", "Body|Tail0"} | {"Tail{}|Tail{}".format(i, i + 1) for i in range(TAIL_COUNT - 1)}
    run("Cat_Run", FRAMES, pose, allowed_overlaps=allowed)
