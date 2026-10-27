"""Cat_Idle: the Cat sits, head pitched up so the face reads from the camera above, and lives: a slow weight shift and
breath underneath, and over them unrelated beats at uneven moments (eye glances the head follows, ear swivels, blinks,
tail twitches) with stillness between, and once a loop the evil face (cat_face.py).

One 10 s loop. The underlay is a sum of whole-cycle sines of unequal counts, each part reading it a few frames late; every
event begins and ends at rest, so the last frame holds the first pose. The head is lifted in Z, not slid forward, so the
pitched head sits over the front of the body and is drawn over it.

`pose` is shared: Cat_Reassemble ends on frame 0 of this clip (loaded with a name other than __main__, which skips the
build).

Run via mcp-unreal execute_script. Re-runnable: rewrites the sequence in place. Report: AI/Output/cat_Cat_Idle.json.
"""
import unreal

_CLIP_PATH = unreal.Paths.project_dir() + "AI/Python/Cat/Anim/cat_clip.py"
CLIP = {}
exec(compile(open(_CLIP_PATH, encoding="utf-8").read(), _CLIP_PATH, "exec"), CLIP)
attack_release, curve, wander, run = CLIP["attack_release"], CLIP["curve"], CLIP["wander"], CLIP["run"]

_FACE_PATH = unreal.Paths.project_dir() + "AI/Python/Cat/Anim/cat_face.py"
FACE = {}
exec(compile(open(_FACE_PATH, encoding="utf-8").read(), _FACE_PATH, "exec"), FACE)

FRAMES = 300  # 10 s
FACE_PITCH_DEGREES = 80.0  # not 90, so the ears still read
HEAD_LIFT = 40.0  # units; clears the pitched head's back over the body top (Z 87)
HEAD_LAG = 4  # frames the head reads the body's underlay late
TAIL_LAG = 6  # per segment
TAIL_COUNT = 9
TAIL_CROSSING = [(40, 0.0), (75, 1.0), (135, 1.0), (170, 0.0)]  # frame, 0 = wrapped to its side, 1 = wrapped to the other side
TAIL_CROSSING_LAG = 3  # frames per segment, the tip crosses last
TAIL_CURL_DEGREES = -10.0  # per segment, on top of the rest curve; negative turns it toward +Y
BREATH = [(3, 0.022, 0.0), (7, 0.005, 0.3)]  # scale of the body
SHIFT_SIDE = [(1, 3.0, 0.0), (2, 1.2, 0.25)]  # units of sideways slide of the body
SHIFT_YAW = [(1, 2.0, 0.1), (3, 0.8, 0.6)]  # degrees
HEAD_DRIFT_PITCH = [(2, 1.5, 0.0), (5, 0.6, 0.4)]
HEAD_DRIFT_YAW = [(1, 3.0, 0.5), (4, 1.0, 0.2)]
TAIL_SWAY = [(2, 3.0, 0.0), (5, 1.5, 0.35)]  # degrees at the base, grown toward the tip
TAIL_PITCH_SWAY = [(3, 2.0, 0.1), (4, 1.0, 0.6)]  # degrees, unequal cycle count to the yaw so the tip loops
GLANCES = [(120, 3.5, 18.0), (210, -3.0, -14.0)]  # frame the eyes move, eye slide in units, head yaw in degrees
GLANCE_HEAD_LATE = 6  # frames the head follows the eyes
BLINKS = [(70, 3, 2, 8, 0.94), (175, 3, 2, 8, 0.94), (189, 3, 2, 8, 0.94), (250, 10, 20, 14, 0.9)]  # start, attack, hold, release, depth
EAR_SWIVELS = [("EarLeft", 40, -14.0), ("EarRight", 95, 12.0), ("EarLeft", 205, -8.0), ("EarRight", 262, 15.0)]
TAIL_TWITCH = (150, 14.0)  # start frame, degrees; only the last three segments
TAIL_FLICK = (230, -12.0, 3)  # start frame, degrees, frames of lag per segment
EVIL_FACE = (12, 36)  # start frame, frames held: before the first blink, clear of every glance


def pose(frame):
    body = (wander(frame, FRAMES, SHIFT_SIDE), wander(frame, FRAMES, SHIFT_YAW))
    lagged = frame - HEAD_LAG
    breath = 1.0 + wander(frame, FRAMES, BREATH)
    glance_yaw = sum(attack_release(frame, start + GLANCE_HEAD_LATE, 8, 40, 20, yaw) for start, _, yaw in GLANCES)

    deltas = {"Body": ((0, body[0], 0), (0, 0, body[1]), (breath,) * 3),
              "Head": ((0, 0, HEAD_LIFT),
                       (0, FACE_PITCH_DEGREES + wander(lagged, FRAMES, HEAD_DRIFT_PITCH),
                        wander(lagged, FRAMES, HEAD_DRIFT_YAW) + glance_yaw), (1, 1, 1)),
              "HandLeft": ((wander(frame, FRAMES, [(2, 0.8, 0.1)]), 0, 0), (0, 0, 0), (1, 1, 1)),
              "HandRight": ((wander(frame, FRAMES, [(3, 0.7, 0.6)]), 0, 0), (0, 0, 0), (1, 1, 1))}

    look = sum(attack_release(frame, start, 3, 50, 10, slide) for start, slide, _ in GLANCES)
    for side, eye in enumerate(("EyeLeft", "EyeRight")):
        closed = sum(attack_release(frame - side, start, attack, hold, release, depth)
                     for start, attack, hold, release, depth in BLINKS)
        deltas[eye] = ((0, look, 0), (0, 0, 0), (1, 1, 1.0 - closed))

    for ear in ("EarLeft", "EarRight"):
        drift = wander(frame, FRAMES, [(3, 1.5, 0.2)] if ear == "EarLeft" else [(2, 2.0, 0.7)])
        swivel = sum(attack_release(frame, start, 4, 10, 12, degrees) for name, start, degrees in EAR_SWIVELS
                     if name == ear)
        deltas[ear] = (CLIP["EAR_DETACH"][ear], (drift + swivel, 0, 0.4 * swivel), (1, 1, 1))

    twitch_start, twitch_degrees = TAIL_TWITCH
    flick_start, flick_degrees, flick_lag = TAIL_FLICK
    for index in range(TAIL_COUNT):
        sway = wander(frame - TAIL_LAG * index, FRAMES, TAIL_SWAY) * (1.0 + 0.25 * index)
        rise = wander(frame - TAIL_LAG * index, FRAMES, TAIL_PITCH_SWAY) * (1.0 + 0.25 * index)
        crossing = curve(frame - TAIL_CROSSING_LAG * index, TAIL_CROSSING) * -2.0 * TAIL_CURL_DEGREES
        flick = attack_release(frame, flick_start + flick_lag * index, 4, 0, 8, flick_degrees)
        twitch = 0.0
        if index >= TAIL_COUNT - 3:
            twitch = attack_release(frame, twitch_start + 2 * (index - TAIL_COUNT + 3), 2, 2, 6, twitch_degrees)
        deltas["Tail{}".format(index)] = ((0, 0, 0), (0, rise, TAIL_CURL_DEGREES + crossing + sway + flick + twitch), (1, 1, 1))
    return FACE["apply_evil"](deltas, frame, *EVIL_FACE)


if __name__ == "__main__":
    run("Cat_Idle", FRAMES, pose, step=10, allowed_overlaps={"Body|Head"})
