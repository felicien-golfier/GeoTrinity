# BadgePartMarks — pending

Goal: every movable badge part shows an outline stroke plus an inner shape, top and bottom, all contours one width, and the lines stay editable (and hand-drawable) at any time.
Priority: 2
Docs written: `AI/Python/Badge/CLAUDE.md` ("Line masks").

Done: `generate_class_badge_masks.py` (masks in `GamePictures/BadgeMasks/`, `LINE_WIDTH` 17 measured from the old Triangle mask), `make_class_badge_materials.py` and `import_textures.py` point at `<Class>_Mask.png`. Body logo lines redrawn as polylines from the old art (not pixel-identical).

## Editor / Blueprint (by hand or script)
- [ ] Rerun `make_class_badge_materials.py` (headless, `-run=pythonscript`, `DebugGame-Cmd`, editor closed); instances keep tuned values (`LineColor` stays black).

## Verify
- [ ] Look at the masks in game / preview: ring + inner on each movable part (Circle disc, 2 Square mandibles, Triangle needle + plug); confirm the thin lines (Square contour + circuit, Circle middle line) are the right ones; tune `INNER_SCALE`, `RING_WIDTH`, insets at the top of the script.
- [ ] No `Source/` change: Tutorial gate not triggered.
