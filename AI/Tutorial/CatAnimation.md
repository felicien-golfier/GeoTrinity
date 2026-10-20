# The Cat — animation

How `SM_Cat` moves. Rig: `SKM_Cat` on `SK_Cat` in `/Game/Art/Meshes/Cat` (`AI/Python/Mesh/rig_cat.py`), materials `M_Cat` + `MI_Cat_<role>` (`AI/Python/Material/make_cat_materials.py`). Clips: `AI/Python/Anim/Cat/`, assets in `/Game/Characters/Anim/Cat`. Authoring rules: `AI/MCP/MCP_Animation.md`. Look and personality: `Cat.md`.

## Reading from the top camera
The camera is orthographic and straight above; the mesh faces +X, so its face plane is edge-on.
- **Top view only, no perspective, ever**: every animation is judged as a flat orthographic silhouette. Nothing scales to suggest nearness or height.
- Height is invisible: a hop, a bob or a sink never reads as motion. Motion reads as travel in X / Y, stretch and squash in the plane, yaw, or the plane clipping the cat (below).
- The face is only seen when the head pitches up toward the camera. Idle does; walk keeps the head level (the Cat is travelling, the face is not the point).
- Yaw, X / Y slides and in-plane stretch / squash are the animator's tools. Every pose is judged as a top-down silhouette.

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
| Tail tip leads every mood; the tail trails the body | Tail chain driven by one curve, each segment reading it a few frames later |
| Breathes slowly while sitting | Slight scale pulse on `Body` |
| Walks in a four-beat lateral sequence (left foot, left hand, right foot, right hand, a quarter stride apart), head held steady while the spine swings | Each paw on its own quarter-stride phase: planted = linear slide back, swing = quicker and eased; `Body` yaws twice a stride, `Head` counter-yaws; tail trails late |
| Pounce: crouch, hindquarter wiggle, stillness, spring | Jump: crouch + wiggle, dead-still hold, explosive cross (`MCP_Animation.md`, "Making motion read as dynamic") |
| Eyes lock on a target before the head turns, the head before the body | Eyes slide first, `Head` yaw 3-4 frames later, `Body` 4-8 later |
| Ears swivel on their own, one then the other, never together | Independent ear rolls and yaws at unrelated times |
| Stretch: front down, hindquarters up, long, then a shake | Cat_Reassemble's finish: reach in X, hold with a tremble, release with an overshoot, head shake |

## Living, not mechanical
General rules: `MCP_Animation.md`, "Making a creature read as alive". For the Cat:
- **Idle** is a 10 s loop of unrelated beats with stillness between: slow weight shift and breath underneath all the time; then, one at a time and at uneven moments, an eye glance (eyes lead, head follows, then settles back), an independent ear swivel, a tail-tip twitch, a slow blink (close 3 frames, open 8). Tail sway sums two cycles of unequal count, each segment late.
- **Walk**: lateral four-beat, planted paw linear / swing eased, head counter-yaw, body yaw twice a stride, tail late and wider at the tip. The clip is one steady stride; varying speed is the owner's job.
- **Jump**: the wiggle swings (4-6 frames a swing) rather than alternating each frame; eyes lock on the portal first (they widen), ears forward then flat, tail tip twitching through the hold; the cross keeps its two frames.
- Everything that moves in pairs (ears, hands, feet, eyes) has unequal amplitude and a 2-3 frame offset between the sides.

## Cat_Reassemble — splashed, then back together, then a stretch
The Cat's entrance and its comeback after a wipe. Not a loop; ends on `Cat_Idle`'s first pose so `ABP_Cat` takes over with no pop. The montage plays in `DefaultSlot` and does not blend in (it starts far from any pose).
| Beat | Motion |
|---|---|
| Splat (from frame 0) | Every part lying on the floor, flung out round the spot where the Cat will sit (parts at ground height, scaled flat: wider in XY, thin in Z, no two touching). Body flat in the middle. A short settle: the parts tremble once and go still |
| Stir | Eyes first: they blink on their own bones, parts twitch, a tail segment curls. Held still between each |
| Gather | Parts come home one at a time on an accelerating schedule (`anim_sequence_authoring.py` `accelerating_schedule`): the Body first, its tail chain dragged after it, then the Head, the ears and face features, the paws last, each arriving with a small overshoot and settle. Pairs offset 2-3 frames |
| Stretch | Hands reach far forward, feet back, the Body long (scale X up, Y down), head low (pitch down) then up, tail high and spread; held with a fine tremble |
| Release | Overshoot past rest, a head shake and an ear flick, settle into `Cat_Idle`'s first pose |
- While gathering, a waiting part clears everything already home: `MCP_Animation.md`, "Bringing a shape together" (checked as the frames a pair touches).
- Seen from above, height is invisible: the splat reads as spread, the gather as parts converging, the stretch as length in X.

## Clips
| Clip | Kind | Notes |
|---|---|---|
| `Cat_Idle` | loop | Sits, head pitched up so the face reads from above, slid forward so the folded-back ears clear the body. Breath, head sway and tail wave periodic over the loop; blink, ear flicks and tail kink start and end at rest, so it closes without a pop. Smug, bored: slow, never busy |
| `Cat_Walk` | loop | Head level. One stride = whole number of frames; hand / foot pairs diagonal; bob is a scale squash, not a lift |
| `Cat_PortalJump` | one shot | Montage `AM_Cat_PortalJump`, below |
| `Cat_Reassemble` | one shot | Montage `AM_Cat_Reassemble`, below |

`ABP_Cat`: `Cat_Idle` and `Cat_Walk` blended by a `Speed` float (0 idle, 1 walk; the owner, `AGeoCat`, writes it and smooths it), then `DefaultSlot` for the montage (editor shim `BuildBlendedAnimGraph`). Nothing outside the AnimBP chooses a clip.

## Portal jump
The Cat opens a portal in front of itself, hops into it and sinks out of sight through the ground plane.
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
