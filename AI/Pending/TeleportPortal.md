# TeleportPortal — pending

Goal: every `AGeoTeleporter` wears the "6 · Round" portal of the Research artifact, identical, with one colour property to change its colour.
Priority: 2
Docs written: `Source/GeoTrinity/Public/Actor/Buttons/CLAUDE.md` ("Portal look", "Open / closed"), `AI/VFX.md` ("Teleport portal").

## Sources (the artifact is the only source; no image / texture / font to fetch)
Read with `Artifact action=read url=https://claude.ai/artifact/RZhzLd3DV8276eMvRo7bbF` (raw HTML); the portal is JS `round(col,hot)` + `sparks(col,hot)`, CSS `.portal/.sp/.breathe`. Canvas 400×400, centre 200,200; radii below are in that space (disc radius 200 = 1.0). Values copied (re-read the page if it was edited):
- Colours: blue `#4aa8ff` / hot `#e3f2ff`; orange `#ff9a2e` / hot `#fff1d2`; background black `#000` core.
- Rim rings (r, stroke, opacity): (168, 2, .12) (150, 3, .2) (136, 6, .36) (127, 10, .85) (120, 4, hot, 1); rim breathes opacity 1 ↔ .72, 3 s ease-in-out.
- Core: disc r 117 black; rings (100, 2, .3) (82, 2, .2).
- Shells (stroke 2.5, polygon outline, rotate about centre): hex r90 op .7 ccw 20 s; tri r62 op .8 cw 14 s; square r38 op .9 (rot 45°) ccw 10 s; solid tri r16 op .95 cw 8 s. "ccw" = the artifact's `rev`=1 (CSS reverse).
- Arcs (circle dashed on 100 units: length from offset start, stroke, colour, opacity, period): r123 w7 len16 start0 hot op1 cw 2.6 s; r123 w5 len9 start50 hot op1 ccw 3.8 s; r130 w9 len24 start25 col op.9 cw 5 s; r116 w3 len30 start70 hot op.9 ccw 3.2 s. Dash start is a share of circumference from the 3 o'clock point.
- Sparks (24, i = 0..23): angle = i·15° (from 3 o'clock, i.e. cos/sin); far = even i, big = i % 3 == 0; start radius 124; travel distance far 96 / near 60 along a path twisted by +0.2 rad at the end; duration far 2.6 s / near 1.8 s; phase offset −((i·7 mod 24)/24 × dur); size big 8 / small 5; colour = col if i % 4 == 1 else (hot if far else col); shape = i % 4: 0 triangle (tip 1.3s, back −s, half-height .9s) solid, 1 hexagon outline r s stroke 1.8, 2 diamond (±1.4s long, ±.8s wide) solid, 3 streak (3.6s × 2 px) solid. Motion eased with spline (.15 .6 .4 1); scale 1 until 45 % of the life then → .1; opacity 0 → 1 by 6 %, held to 50 %, → 0 at 100 %. Sparks rotate to face travel direction.
- Life (6 s loop in the artifact; game: open 0.5 s / hold / close 0.9 s): opacity 0 → 1 and scale .35 → 1.04 (at 8 % of the loop) → 1 (12 %), close back to .35 / 0 with ease `cubic-bezier(.2,.7,.2,1)`.
- Game pixel scale: artifact disc outer radius 200 ↔ the pad's chosen world radius (see Editor row "disc size").

## Done
- [x] `GeoTeleporter`: `PortalComponent` (`Activate` = intro, graceful `Deactivate` = outro), `PortalColor` → `User.Color`, label above, root mesh invisible; build + Encounter tests green
- [x] `M_TeleportPortal` (unlit translucent, one Custom HLSL node, colour and alpha from the particle colour)
- [x] `NS_TeleportPortal` (one system, beam life phases; `AI/Python/Niagara/teleport_portal_vfx.py`), defaults of the pad; seen in PIE on the lobby pad

## Left
- [ ] Look: compare with the artifact's held frame (spark ease approximated by `1-(1-t)^2.5`; spins assume UV y down the screen; scale intro goes 0 → 1, no 1.04 overshoot)
- [ ] Disc size vs the trigger shape of `TeleportDirectionMesh` (quad is 464 units wide); two pads, two colours
- [ ] Close / reopen seen in PIE (needs an encounter or `SetOpen` call)
