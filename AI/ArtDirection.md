# Art Direction — Geometry First

The player *is* a shape, and everything the game draws follows from that: **flat neon geometry on black**.
Hard edges, thin bright lines, whole polygons nameable in one frame. Nothing hazy, nothing organic, nothing
that would look the same in another game. The classes carry the vocabulary — Square, Circle, Triangle, a
hexagonal boss — and an effect is not decoration beside that vocabulary, it is the vocabulary in motion.

Read this before authoring any visual asset. The mechanisms are in `AI/VFX.md`.

**References**: *Sektori* is the target — vector-clean neon on black, every enemy, bullet and effect built from
the same handful of solids and outlines. *Geometry Wars* for grid and line as the entire visual world, where
brightness carries energy and texture carries nothing. *Tron / Rez* for light as an **edge**, not a volume: a
thin line at high intensity beats a wide glow every time.

## The grammar

Shape says **who**, colour says **what**, motion says **how much**.

| Channel | Rule |
|---|---|
| Shape — identity | An effect's shape is its source's shape: a Triangle's shot throws triangles, a Square's wall quads, a Circle's heal rings and arcs, the hex boss hexagons. A buff has no source shape — any class wears it — so it takes the shape of what it *means*: triangle for damage, ring for healing, hexagon for protection. |
| Colour — gameplay meaning | Straight from `EGeoColor` through the palette. Never picked because it looks nice: two effects meaning the same thing are the same colour, two meaning different things never are. |
| Motion — magnitude | Bigger buff, faster spin, more elements — never a different mechanism. A stronger version stays recognisably the same effect. |

Hollow and solid of the same polygon are one identity at two energies — hollow reads as light, solid as mass.
Mixing them gives a field of shapes depth without spending a second colour or shape.

## Rules

1. **Every element has a silhouette.** If a single particle frozen at gameplay zoom cannot be named — triangle,
   ring, quad, line — it does not belong on screen. The test is the frozen frame: motion hides everything,
   which is how soft effects survive authoring and die in a screenshot.
2. **Hollow and solid, mixed and exact.** Draw the same polygon both ways and split them by a rule, not by
   chance — alternate on the particle's index, so the ratio is what was authored. A random 50/50 clumps, and a
   clump of solids is a blob again.
3. **Core and halo, never a blob.** A thin, near-white, over-bright core with a wide dim halo of the same hue
   behind it, dim enough to disappear against a bright background. A soft radial gradient may sit *behind* a
   hard shape and may never be the subject.
4. **Place on a lattice, never scatter.** Elements sit on exact angular divisions (360/N from the execution
   index), on concentric shells at stepped radii, or along a path. Random placement is what produces haze;
   spacing is what makes a machine. Randomness is a discrete pick from a small set — one of three sizes, one of
   six angles — never continuous jitter on position.
5. **Motion is rotation and travel, not noise.** Constant angular velocity, constant orbit, constant drift:
   mechanical motion reads as manufactured, which is the point. Noise may bend a strand that already has a
   shape; noise may never be the shape.
6. **Black ground, HDR core.** Additive, unlit, HDR: core 6–15, body 1.5–4, halo 0.2–0.8. No white fill larger
   than a few pixels except on an impact frame, and no mid-grey anything — value separation is what makes bloom
   read as light instead of fog.
7. **Snap, hold, shrink.** Full intensity inside 0.08 s, hold while the state holds, then leave by **shrinking
   and fading together** over 0.25–0.6 s. A shape keeps its edges all the way out: fading on alpha alone turns
   a polygon into a smudge on its last frames, which is the frame the eye lingers on.
8. **Few, big, readable.** Six to twenty-four elements in an aura, not hundreds. Every element costs
   readability in a bullet-hell where the player is reading bullets. An effect needing more elements to read
   has elements that are too small.
9. **Every aura sheds.** An aura that only rides its owner is a decal. One layer is small, fast and **left in
   world space** — spawned around the body and abandoned there, so the body walks out of its own effect:
   standing still it is a pulse, running it is a trail. Speed gets drawn by the ground covered, which nothing
   riding along can show. What is shed is the shapes the aura is already made of, never a second vocabulary,
   and it keeps a slow drift of its own so it is alive when its owner is not.

## Excluded

- Smoke, plumes, fire, wisps, embers, fizz — no soft organic layer of any kind.
- The engine's default sprite and ribbon materials as a *final* look: both are round and soft, placeholders for
  everything except a strand whose shape is earned by real motion.
- Continuous noise as the source of a form. Bending, yes; forming, no.
- Anything both rotationally symmetric and soft — no edge and no orientation leaves nothing to read, which is
  what a scatter of glow sprites around a character amounts to.
- Colour chosen for looks, gradients between unrelated hues, any hue outside the palette.

`NS_ChargedTrail` and `NS_VitalTrail` pass for a generalisable reason: their shape is not authored, it is the
flight path of the shot, and a path is geometry. A body standing still lends no path, so an aura's shape has to
live in its elements and their placement — which is what rules 1, 2 and 4 are for.

## Recipes

Colour comes from the palette slot the effect means; shape, placement and rotation are the authored part.

| Reads as | Built from |
|---|---|
| **Faceted shell** — a transparent solid turning around the body | Concentric polygon outlines at stepped radii, counter-rotating on unrelated periods, rim brighter than interior. From a fixed top-down camera a shell *is* its outlines; a third outline at a half-step radius stops two of them reading as flat rings. |
| **Shape field** — hollow and solid polygons turning and fading | Sprites of one polygon on a lattice or along a path, born at one of a few discrete angles, each spinning at a constant rate, alternating hollow/solid by index, shrinking as they fade. |
| **Rail** — a bright core line with a little haze | Three layers on the same two endpoints: a thin near-white core with the ribbon's corners sharp, a wider dim ribbon of the hue behind it, a sparse drift of small shapes around it. The jitter belongs to the haze, never the core. |
| **Single line** — one straight stroke | A beam between two points with nothing bending it: no curl force, no jitter, tension at maximum. A line is a shape; a wobbling line is a noodle. |
| **Colour walk** — a ribbon whose hue travels along its length | One strand, colour driven by particle age or ribbon link order across two palette hues. The path stays simple — the colour is the event. |

## Judging an effect

1. **One frame.** Screenshot at gameplay zoom and name every shape. A layer that cannot be named is haze, not
   form — delete it.
2. **Eight at once.** Eight characters buffed, mid-fight. It has to still read, and bullets have to read
   through it. An aura that hides a bullet is a bug, not a look.
3. **Against the ground.** On the lattice background and the brightest arena floor, not only on black.
4. **The last frame.** If it dissolves into a smudge, the fade is wrong.
5. **From a stranger's chair.** If the answer to "what does this mean" is "something is glowing", the colour or
   the shape is not doing its job.
