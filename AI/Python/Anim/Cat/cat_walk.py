"""Cat_Walk: a cat's four-beat walk. Head held steady while the spine swings, each paw on its own quarter of the stride,
tail trailing late and wider toward its tip.

One loop is one stride of FRAMES frames (a whole number of quarters, so every footfall lands on a sample). Footfall
order is left foot, left hand, right foot, right hand. A planted paw slides back in a straight line, the swing is quicker
and eased. The bob is a scale squash on the body, four times a stride, since a lift is invisible from above.

Run via mcp-unreal execute_script. Re-runnable: rewrites the sequence in place. Report: AI/Output/cat_Cat_Walk.json.
"""
import unreal

_CLIP_PATH = unreal.Paths.project_dir() + "AI/Python/Anim/Cat/cat_clip.py"
CLIP = {}
exec(compile(open(_CLIP_PATH, encoding="utf-8").read(), _CLIP_PATH, "exec"), CLIP)
paw_slide, wander, run = CLIP["paw_slide"], CLIP["wander"], CLIP["run"]

FRAMES = 32  # 1.07 s stride
PAWS = {"FootLeft": (0.0, 9.0), "HandLeft": (0.25, 7.0), "FootRight": (0.5, 10.0), "HandRight": (0.75, 6.5)}  # phase in turns, reach in units
BODY_YAW = [(2, 4.0, 0.1), (1, 1.5, 0.0)]  # degrees: twice a stride, once on top
BODY_SIDE = [(1, 2.0, 0.25)]  # units
HEAD_COUNTER = -0.7  # of the body's yaw, read HEAD_LAG frames late
HEAD_LAG = 3
BOB = [(4, 0.02, 0.0), (2, 0.008, 0.3)]
TAIL_COUNT = 9
TAIL_CURL_DEGREES = -6.0
TAIL_SWAY = [(1, 6.0, 0.0), (2, 2.0, 0.4)]  # degrees at the base, grown toward the tip
TAIL_LAG = 3  # frames per segment
EAR_BOUNCE = {"EarLeft": [(2, 2.0, 0.0)], "EarRight": [(2, 1.4, 0.35)]}


def pose(frame):
    bob = 1.0 + wander(frame, FRAMES, BOB)
    deltas = {"Body": ((0, wander(frame, FRAMES, BODY_SIDE), 0), (0, 0, wander(frame, FRAMES, BODY_YAW)), (bob,) * 3),
              "Head": ((0, 0, 0), (0, 0, HEAD_COUNTER * wander(frame - HEAD_LAG, FRAMES, BODY_YAW)), (1, 1, 1))}
    for paw, (phase, reach) in PAWS.items():
        deltas[paw] = ((paw_slide(frame / float(FRAMES) - phase, reach), 0, 0), (0, 0, 0), (1, 1, 1))

    for ear, terms in EAR_BOUNCE.items():
        deltas[ear] = ((0, 0, 0), (wander(frame - HEAD_LAG, FRAMES, terms), 0, 0), (1, 1, 1))

    for index in range(TAIL_COUNT):
        sway = wander(frame - TAIL_LAG * (index + 1), FRAMES, TAIL_SWAY) * (1.0 + 0.3 * index)
        deltas["Tail{}".format(index)] = ((0, 0, 0), (0, 0, TAIL_CURL_DEGREES + sway), (1, 1, 1))
    return deltas


run("Cat_Walk", FRAMES, pose)
