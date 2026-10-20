"""Cat_PortalJump: the Cat opens a portal ahead of itself, crouches, hops onto it and sinks through the ground plane.

Seen from above, without perspective: height is invisible and nothing scales for nearness. The Cat lunges along X
(stretch) and the plane cuts it as it sinks, so the Body drops in Z while the Cat material masks everything below the
actor origin. Root never moves, so the PortalPoint socket stays where the portal opened.

    Open          raised paw swipes down; the portal notify state begins on the swipe (PortalPoint, ground ahead)
    Anticipation  crouch (squashed along X), hindquarter wiggle growing, then dead still
    Lunge         the Body crosses to the portal in two frames, stretched along X, and sinks as it arrives
    Dive          it keeps travelling forward past the portal while it sinks, light parts first, tail last; every
                  part ends below the plane
    Close         the notify state ends once the tail is gone and destroys the portal's component (a graceful
                  Deactivate left it alive for good)

The lunge reaches the socket; the socket's distance is rig_cat.py's PORTAL_X.

Run AFTER AI/Python/Mesh/rig_cat.py and cat_idle.py, via mcp-unreal execute_script. Re-runnable: rewrites the
sequence, the montage and its notify in place. Report: AI/Output/cat_Cat_PortalJump.json.
"""
import math

import unreal

_CLIP_PATH = unreal.Paths.project_dir() + "AI/Python/Anim/Cat/cat_clip.py"
CLIP = {}
exec(compile(open(_CLIP_PATH, encoding="utf-8").read(), _CLIP_PATH, "exec"), CLIP)
TOOLKIT, attack_release, curve, ramp, run = CLIP["TOOLKIT"], CLIP["attack_release"], CLIP["curve"], CLIP["ramp"], CLIP["run"]

FRAMES = 90  # 3 s
SINK_DEPTH = 130.0  # clears the highest part above the origin (the ear tips, 120)
SINK_START, SINK_END = 52, 78  # starts as the lunge reaches the portal: no stop between hop and sink
SINK_LEAD = 40.0  # extra units the head, hands and feet drop, so they go first and the tail last
PORTAL_OPEN_FRAME, PORTAL_CLOSE_FRAME = 10, 84
WIGGLE_START, WIGGLE_END, WIGGLE_DEGREES = 22, 42, 8.0
WIGGLE_PERIOD = 6  # frames a swing; divides the wiggle's end, so it stops at rest
TAIL_TIP_TWITCH = (42, 50, 4, 9.0)  # start, end, period in frames, degrees; the last three segments
EYES_WIDE = (16, 4, 36, 6, 0.25)  # start, attack, hold, release, growth; the right eye a frame late
EAR_PITCH = [(0, 0.0), (18, 0.0), (22, 10.0), (40, 10.0), (44, -14.0), (58, -14.0), (66, 0.0)]  # forward, then flat; the right ear 2 frames late, smaller
HOP_START = 50
PAW_X = [(0, 0.0), (8, 26.0), (14, 40.0), (22, 0.0)]
PAW_Y = [(0, 0.0), (8, 0.0), (14, -2.0), (22, 0.0)]
PAW_SCALE = [(0, 1.0), (8, 1.8), (14, 1.2), (22, 1.0)]
BODY_STRETCH = [(0, 1.0), (20, 1.0), (32, 0.85), (50, 0.85), (51, 1.45), (54, 1.3), (64, 1.1)]  # scale along X (Y the inverse): crouch squash, lunge stretch
HOP_PROGRESS = [(50, 0.0), (51, 0.55), (52, 1.0)]  # share of the distance to the portal
DIVE_DRIFT = 0.4  # extra share of that distance travelled between the portal and the end of the sink: it keeps going
PORTAL_SYSTEM = "/Game/Art/VFX/Teleporter/NS_TeleportPortal"
PORTAL_SOCKET = "PortalPoint"
PORTAL_TRACK = "Portal"
MONTAGE = "AM_Cat_PortalJump"
LIGHT_PARTS = ("Head", "HandLeft", "HandRight", "FootLeft", "FootRight")


def hop_distance():
    """Units from the Body's rest position to the PortalPoint socket on the ground."""
    socket = unreal.load_asset(CLIP["MESH_PATH"]).find_socket(PORTAL_SOCKET)
    body = TOOLKIT["reference_pose_table"](CLIP["SKELETON_PATH"])["Body"][1].translation
    return socket.get_editor_property("relative_location").x - body.x


def make_pose(hop):
    def pose(frame):
        grow = ramp(frame, WIGGLE_START, WIGGLE_END)
        swing = math.sin(2.0 * math.pi * frame / WIGGLE_PERIOD) if frame < WIGGLE_END else 0.0
        wiggle = WIGGLE_DEGREES * grow * swing
        stretch = curve(frame, BODY_STRETCH)
        dropped = ((frame - SINK_START) / float(SINK_END - SINK_START))
        dropped = max(0.0, min(1.0, dropped))
        travel = curve(frame, HOP_PROGRESS) + DIVE_DRIFT * dropped

        deltas = {"Body": ((hop * travel, 0, -SINK_DEPTH * dropped ** 2), (0, 0, wiggle),
                           (stretch, 1.0 / stretch, 1.0)),
                  "HandRight": ((curve(frame, PAW_X), curve(frame, PAW_Y), 0), (0, 0, 0), (curve(frame, PAW_SCALE),) * 3)}
        lead = -SINK_LEAD * ramp(frame, SINK_START - 4, SINK_END - 6)
        for part in LIGHT_PARTS:
            translation, rotation, part_scale = deltas.get(part, ((0, 0, 0), (0, 0, 0), (1, 1, 1)))
            deltas[part] = ((translation[0], translation[1], lead), rotation, part_scale)
        twitch_start, twitch_end, period, degrees = TAIL_TIP_TWITCH
        for index in range(9):
            late = -0.6 * WIGGLE_DEGREES * ramp(frame - 4 - 2 * index, WIGGLE_START, WIGGLE_END) * (
                math.sin(2.0 * math.pi * (frame - 4 - 2 * index) / WIGGLE_PERIOD) if frame - 4 - 2 * index < WIGGLE_END else 0.0)
            tip = 0.0
            if index >= 6 and twitch_start <= frame < twitch_end:
                tip = degrees * math.sin(2.0 * math.pi * frame / period)
            deltas["Tail{}".format(index)] = ((0, 0, 0), (0, 0, late + tip), (1, 1, 1))

        start, attack, hold, release, growth = EYES_WIDE
        for side, (eye, ear) in enumerate((("EyeLeft", "EarLeft"), ("EyeRight", "EarRight"))):
            wide = 1.0 + attack_release(frame - side, start, attack, hold, release, growth)
            deltas[eye] = ((0, 0, 0), (0, 0, 0), (1, wide, wide))
            deltas[ear] = ((0, 0, 0), (0, curve(frame - 2 * side, EAR_PITCH) * (1.0 - 0.15 * side), 0), (1, 1, 1))
        return deltas
    return pose


def add_montage_and_notify(sequence):
    montage = TOOLKIT["build_montage"](sequence, CLIP["ANIM_PACKAGE"], MONTAGE, ["Default"], [0.0], ["None"])
    fps = float(CLIP["FPS"])
    TOOLKIT["set_notify"](montage, PORTAL_TRACK, PORTAL_OPEN_FRAME / fps, unreal.AnimNotifyState_TimedNiagaraEffect,
                          {"template": unreal.load_asset(PORTAL_SYSTEM), "socket_name": PORTAL_SOCKET,
                           "destroy_at_end": True},
                          duration=(PORTAL_CLOSE_FRAME - PORTAL_OPEN_FRAME) / fps)
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(CLIP["ANIM_PACKAGE"], MONTAGE))
    return {"montage_sections": TOOLKIT["montage_sections"](montage),
            "notifies": [(round(unreal.AnimationLibrary.get_anim_notify_event_trigger_time(event), 3),
                          round(unreal.AnimationLibrary.get_anim_notify_event_duration(event), 3),
                          event.get_editor_property("notify_state_class").get_class().get_name())
                         for event in unreal.AnimationLibrary.get_animation_notify_events(montage)]}


run("Cat_PortalJump", FRAMES, make_pose(hop_distance()), after=add_montage_and_notify)
