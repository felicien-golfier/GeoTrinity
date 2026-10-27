# The Cat — animation

How `SM_Cat` moves. Rig: `SKM_Cat` on `SK_Cat` in `/Game/Art/Meshes/Cat` (`AI/Python/Cat/Mesh/rig_cat.py`), materials `M_Cat` + `MI_Cat_<role>` (`AI/Python/Cat/make_cat_materials.py`). Clips: `AI/Python/Anim/Cat/`, assets in `/Game/Characters/Anim/Cat`. Authoring rules: `AI/MCP/MCP_Animation.md`. Look and personality: `Cat.md`.

## Reading from the top camera
The camera is orthographic and straight above; the mesh faces +X, so its face plane is edge-on.
- **Top view only, no perspective, ever**: every animation is judged as a flat orthographic silhouette. Nothing scales to suggest nearness or height.
- Height is invisible: a hop, a bob or a sink never reads as motion. Motion reads as travel in X / Y, stretch and squash in the plane, yaw, or the plane clipping the cat (below).
- Height decides what covers what: a part lifted in Z is drawn over the parts under it. A pitch reads as a length change (a part tipped toward the camera shortens).
- The face is only seen when the head pitches up toward the camera. **Every clip keeps the head slightly pitched up** (`FACE_PEEK_DEGREES` in `cat_posing.py`, ≈ 20°, the `Cat_Stretch_Flat` head at frame 61): eyes, nose and mouth always peek over the forehead edge. Never a level head (face edge-on, invisible: the old walk, portal jump). Idle sits higher (`FACE_PITCH_DEGREES`); no clip goes below the peek except a deliberate beat (stretch head-down, splat). Face pose checked in every clip's report.
- Yaw, X / Y slides and the Body's stretch along X are the animator's tools. Every pose is judged as a top-down silhouette.
- **One scaled part**: only `Body` scales, along X; the other parts keep their size, placed in the world (`MCP_Animation.md`, "Scale as motion"). No bounce. Face features are the exception: eyes scale for blink / squint, fangs scale 0 ↔ 1 to appear.
- **Ears float**: `EAR_DETACH` in every clip.

## Rig
Bones rest on their part's centre, unturned and unscaled (`MCP_Animation.md`, "Editing the rig").
| Bone | Parent | Carries |
|---|---|---|
| `Root` | — | nothing; on the actor origin (the plane); holds the `PortalPoint` socket (no clip moves it) |
| `Body` | `Root` | pyramid |
| `Head` | `Body` | `HeadRound`, `HeadFur` |
| `EarLeft` `EarRight` | `Head` | ear + inner ear |
| `EyeLeft` `EyeRight` | `Head` | eye (blink = scale Y, look = slide) |
| `Nose` | `Head` | nose |
| `Mouth0` `Mouth1` | `Head` | the two mouth curves |
| `FangLeft` `FangRight` | `Head` | small `Light` fang under each side of the grin; hidden in every clip but the evil face (below) |
| `HandLeft` `HandRight` `FootLeft` `FootRight` | `Body` | paw discs |
| `Tail0` … `Tail8` | `Body`, then each under the previous | tail squares |

One part per bone, rigid weights. Material slots stay the colour roles `Base` / `Shade` / `Ink` / `Light`.

## Cat motion, as the game uses it
What a cat does, and what we keep of it.
| Real cat | In the clips |
|---|---|
| Sits with front legs straight, hind legs folded, tail wrapped round the feet | Feet pulled in beside the body, hands at the sides of the head, tail curled hard to one side |
| Slow blink = trust / disdain; eyes narrow before they close | Squint → hold → blink on the eye bones' Z scale (the face's up axis); the Board's face timings (`Pending/CatPortrait.md`, "Face geometry") are the model, the 3D face blinks like the HUD face |
| Ears turn to sounds, flick when bored | Rare small ear roll, one side then the other |
| Tail tip leads every mood; the tail trails the body | Tail chain driven by its wave curves, each segment reading them a few frames later (`Tail`, below) |
| Breathes slowly while sitting | No pulse; stillness between beats |
| Walks in a four-beat lateral sequence (left foot, left hand, right foot, right hand, a quarter stride apart), head held steady while the spine swings | Each paw on its own quarter-stride phase: planted = linear slide back, swing = quicker and eased; `Body` yaws twice a stride, `Head` counter-yaws; tail raised, centred, waving late (`Tail`, below) |
| Looking up at someone: the head tips back over the shoulders, the neck stays put | Idle head lifted in Z and pitched up, sitting over the front of the body, never slid forward |
| Pounce: crouch, hindquarter wiggle, stillness, spring | Jump: crouch + wiggle, dead-still hold, explosive cross (`MCP_Animation.md`, "Making motion read as dynamic") |
| Eyes lock on a target before the head turns, the head before the body | Eyes slide first, `Head` yaw 3-4 frames later, `Body` 4-8 later |
| Ears swivel on their own, one then the other, never together | Independent ear rolls and yaws at unrelated times |
| Stretch: front down, hindquarters up, long, then a shake | Cat_Reassemble's finish: reach in X, hold with a tremble, release with an overshoot, head shake |

## Living, not mechanical
General rules: `MCP_Animation.md`, "Making a creature read as alive". For the Cat:
- **Idle** is a 10 s loop of unrelated beats with stillness between: slow weight shift and breath underneath all the time; then, one at a time and at uneven moments, an eye glance (eyes lead, head follows, then settles back), an independent ear swivel, a tail-tip twitch, a slow blink (close 3 frames, open 8). Tail: wrapped to one side, its slow wave on yaw and pitch (`Tail`, below).
- **Walk**: two takes of one pattern blended by the Cat's speed, `Cat_Walk_Prowl` (slow, tail trailing) and `Cat_Walk_Fast` (long steps, tail hooked). Same frame count (56), same paw phases, same wave cycle counts and lags (as fractions of the stride): they differ only in amplitude (reach, yaw, side slide, tail base / hook), so any blend keeps every beat aligned. Each: lateral four-beat, head counter-yaw, tail raised and centred, its wave late and wider at the tip (`Tail`, below), sine paws, one yaw a stride, no scale. Natural speed = the blend space's sample position (`ABP_Cat`, below), so the paws never skate.
- **Run**: `Cat_Run`, a rotary gallop, not a faster walk, so it is its own clip (≈ 24 frames, own cycle). Two-beat gather / extend: hands reach together, feet drive together; `Body` stretches along X at full extension and shortens when gathered (the spine flexing, the one scaled part). Head peeks up (`FACE_PEEK_DEGREES`) with a small counter-yaw, ears pinned back, tail streaming low and level with a small flutter, one evil-face flash per stride.
- **Idle variants**: one-shots from and back to `Cat_Idle` frame 0; one beat, stillness around. All five (`Toilet`, `Groom`, `Loaf`, `LookAround`, `Knead`) are played at random by the owner while the Cat stands (`ABP_Cat`, below).

## Evil face
The Cat's true nature leaking (`Cat.md`, "Two readings"): a brief flash of the mask slipping, never a mood change. Smug stays the baseline.
| Feature | Evil pose (weight 1) |
|---|---|
| Eyes | Squinted to arches (Z scale .35), rolled 22° past the rest tilt so the inner corners sit low; the right eye reads 1 frame late |
| Mouth | `Mouth0` / `Mouth1` widen (Y scale 1.3), deepen (Z scale 1.6) and sink 1 unit: a wide grin |
| Fangs | Scale hidden → 1 under the grin's corners, sinking with it, 2 frames after the grin opens, gone 1 frame before it closes |
| Nose | Unchanged |
| Ears | Pinned back and outward (roll and pitch, unequal per side) |
- One weight curve drives it all (`cat_face.py` `evil_weight(frame, start, hold)`: attack 4 frames, hold, release 8, eased); features read it with their own small lags, so the face never snaps as one piece. A clip adds it on its finished pose (`apply_evil`).
- Only `Cat_Idle`, the two walks and `Cat_Run` carry it, one beat a loop (the least a loop can hold): **Idle** at frame 12, held 36 frames (1.2 s), between the loop start and the first blink, clear of every glance; **walks** frame 20, held 4 (16 frames in all), same pose in both; **run** frame 4, no hold.
- Fangs hide in every other clip: `cat_clip.py` writes the hidden track by default (`FANG_HIDDEN_SCALE`; a clip without it would show them at rest), `cat_reassemble.py` keeps them hidden through splat and gather.
- Face visibility rule applies: the head is already up, so the grin and fangs peek over the forehead edge.
- 3D only; the HUD portrait (`SGeoCatFace`) keeps its face.

## Tail
What a cat's tail does, and how the chain `Tail0` … `Tail8` reproduces it.
| Real cat | In the clips |
|---|---|
| The tail follows the body's path; the tip points back to where it just was | Every driver read late down the chain (a few frames a segment), amplitude growing toward the tip |
| Balance: quick sideways sweeps, opposite to the body's sway | Base yaw = the body's yaw, inverted, a few frames late |
| Moves in every direction, not a pendulum: down, up, other side | Two waves, yaw and pitch, at unequal whole cycle counts (e.g. 1 and 2 a stride): the tip traces loops. From above, pitch reads as the tail shortening and lengthening |
| Walking, relaxed or curious: carried raised, a hook at the tip | Base pitched up (~30-40°), hook on the last segments only; not upright, so the tail still reads from above |
| Slow sway = calm; fast lashing = agitation | Walk wave at stride rate at most; idle wave slower still. The Cat is smug, never agitated |
| Sitting: wrapped round the feet | Idle only: curled to one side |

- Idle tail crosses to the other side now and then; in `Cat_Idle_Knead` it counter-swings with the body.
- Neutral line: the mesh's tail curls toward +Y (`TAIL_TURN_DEGREES` per square, `generate_cat_mesh.py`). The walk cancels it per segment, so its wave swings across the midline, both sides alike. The idle keeps the curl (the wrap).
- Every term is a whole number of cycles over the clip, so it loops; the base reads the waves first, the tip last.
- **Jump**: the wiggle swings (4-6 frames a swing) rather than alternating each frame; eyes lock on the portal first (they widen), ears forward then flat, tail tip twitching through the hold; the cross keeps its two frames.
- Everything that moves in pairs (ears, hands, feet, eyes) has unequal amplitude and a 2-3 frame offset between the sides.

## Cat_Reassemble — splashed, then back together, then a stretch
The Cat's entrance and its comeback after a wipe. Not a loop; ends on `Cat_Idle`'s first pose so `ABP_Cat` takes over with no pop. The montage plays in `DefaultSlot` and does not blend in (it starts far from any pose).
| Beat | Motion |
|---|---|
| Splat (from frame 0) | Every part lying on the floor, flung out round the spot where the Cat will sit (parts at ground height, scaled flat: wider in XY, thin in Z, no two touching). Body flat in the middle. A short settle: the parts tremble once and go still |
| Stir | Eyes first: they blink on their own bones, parts twitch, a tail segment curls. Held still between each |
| Gather | Parts come home one at a time on an accelerating schedule (`anim_sequence_authoring.py` `accelerating_schedule`): the Body first, its tail chain dragged after it, then the Head, the ears and face features, the paws last, each arriving with a small overshoot and settle. Pairs offset 2-3 frames |
| Stretch | The `Cat_Stretch_Twist` pose (one spec, `cat_posing.py` `twist_spec`, shared with the clip): spine curved, front swung to one side, hands across, feet kicked the other way, head looking back, tail curled, Body long; held with a fine tremble |
| Release | Overshoot past rest, a head shake and an ear flick, settle into `Cat_Idle`'s first pose |
- While gathering, a waiting part clears everything already home: `MCP_Animation.md`, "Bringing a shape together" (checked as the frames a pair touches).
- Seen from above, height is invisible: the splat reads as spread, the gather as parts converging, the stretch as length in X.

## Clips
| Clip | Kind | Notes |
|---|---|---|
| `Cat_Idle` | loop | Sits (the tail crosses to the other side now and then), head pitched up so the face reads from above, lifted in Z (no X slide, a small one back at most) so it sits over the front of the body and is drawn over it, ears folded back over the body. Walk → idle: the head tips up in place, never travels forward. The idle check allows the head family over the body (the one intended overlap). Breath, head sway and tail wave periodic over the loop; blink, ear flicks and tail kink start and end at rest, so it closes without a pop. Smug, bored: slow, never busy. One evil-face beat per loop (`Evil face`, above) |
| `Cat_Walk_Prowl` `_Fast` | loop | The only walks. Same length and pattern (56 frames), amplitude differs. Head peeks up (`FACE_PEEK_DEGREES`) and counter-yaws; the raised tail's squares and the pyramid cover one another from above (allowed pairs). One stride = whole number of frames; hand / foot pairs diagonal. Prowl = slow, tail trailing; Fast = long steps, tail hooked. Each stride carries one evil-face flash (`Evil face`, above) |
| `Cat_Run` | loop | Gallop (`Cat motion`, above): hands then feet land in pairs, `Body` stretches / gathers along X, tail streaming. Head peek, one evil-face flash per stride. Not in `BS_Cat_Walk` until the AnimBP integrates it |
| `Cat_Idle_Toilet` | one shot (idle variant, montage `AM_Cat_Idle_Toilet`; the five variants all have one) | Circles, squats (tail up, strains), digs, glances back |
| `Cat_Idle_Groom` | one shot | Licks the left hand, wipes it over the left ear; the ear folds as it passes |
| `Cat_Idle_Loaf` | one shot | Tucked in, nods off: sinks, snaps up once, sinks again |
| `Cat_Idle_LookAround` | one shot | Head turns each way; body and ears follow |
| `Cat_Idle_Knead` | one shot | Hands push in turn, eyes half shut, tail counter-swings |
| `Cat_Stretch_Reach` `_Sprawl` `_Flat` `_Twist` | one shot | Floor stretches, `Body` 25-45% longer, shaking through the hold: Reach = on its side; Sprawl = on its back; Flat = belly down; Twist = spine curved. Twist is the Tutorial's (`Cat_Reassemble`'s stretch beat); the others are kept for later content, no montage until a step needs one |
| `Cat_PortalJump` | one shot | Montage `AM_Cat_PortalJump`, below |
| `Cat_Reassemble` | one shot | Montage `AM_Cat_Reassemble`, below |

`ABP_Cat`: `Cat_Idle` and the locomotion blended by a `Speed` float in cm/s (the owner, `AGeoCat`, writes the real, smoothed speed), then `DefaultSlot` for the montage (editor shim `BuildBlendedAnimGraph`).
- Locomotion = 1D blend space `BS_Cat_Walk` on `Speed`: `Cat_Walk_Prowl` at `PROWL_SPEED`, `Cat_Walk_Fast` at `FAST_SPEED` (150 and 300 cm/s, constants in `cat_walks.py`, read by the blend space script; 300 is the most the Cat walks at). `Cat_Run` is sampled at `RUN_SPEED` 600 (`cat_run.py`) when the AnimBP adds it. Chosen, not matched to the strides, so the feet slide at speeds away from the clips'. Time is synced across the two (normalized time), so a speed change never restarts a step.
- Idle ↔ locomotion: alpha = `Speed / PROWL_SPEED` clamped 0-1, so the Cat eases into the Prowl and the blend space takes over above it.
- Idle variants and the entrance / jump are montages in `DefaultSlot`. The AnimBP chooses the locomotion clip; the owner only picks which idle variant montage to play (`Actor/CLAUDE.md`, "GeoCat").

## Portal jump
The Cat opens a portal in front of itself, hops into it and sinks out of sight through the ground plane. Crouch shortens `Body`, lunge lengthens it; other parts keep their size.
| Beat | Motion |
|---|---|
| Open | Raised paw, swipe down. The montage's `Timed Niagara Effect` notify state (stock engine class, no C++) begins on the swipe: it spawns `NS_TeleportPortal` attached at the `PortalPoint` socket (ground, ahead of the Cat), intro 0.5 s then held; colour = the system's default |
| Anticipation | Crouch, hindquarter wiggle growing, then dead-still hold |
| Lunge | Cross to the portal in two frames, stretched along X (no scale-up) |
| Dive | No stop at the portal: `Body` keeps travelling forward while it sinks in Z through the plane (`Root` stays, so the socket stays); head, hands and feet drop extra so they go first, the tail last; ends fully below the plane |
| Close | The notify state ends once the tail is gone and destroys the portal's component (`destroy_at_end`; a graceful `Deactivate` left the effect alive forever) |

- **The plane**: the world height of the actor origin. `M_Cat` masks every pixel below it (unlit, two-sided so the cut shows the inside, masked blend, world Z against the actor origin Z; not the object position, which is the bounds centre), so a part vanishes as it crosses. From above the silhouette shrinks as the plane cuts it, head and ears last; the portal disc on the plane hides the cut edge.
- The clip is fully below the plane before its blend-out starts; the encounter hides the cat right after the montage order (`GeoSetActorHiddenAction`), which lands at blend-out start.
- The sink depth clears the Cat's highest point above the origin (ear tips).
- The portal distance is `PORTAL_X` in `rig_cat.py` (the socket); the clip reads the socket and lands the Body on it.
- The portal is a notify-spawned system, not an `AGeoTeleporter`: the Cat's portal opens, carries no travel logic and is closed by the clip.
