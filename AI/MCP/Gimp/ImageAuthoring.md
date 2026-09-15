# Image Authoring in GIMP

Making images from scratch or from sources through the `gimp-mcp` server — game content (icons, UI, sprites,
textures) and print or web pieces (flyers, posters, social images). Read `CLAUDE.md` in this folder first for
connection and the exec console.

## Working from bitmaps

- The only view of the work is a bitmap fetched back; judge it at full size, at the display size and as a zoomed
  region of the part just changed.
- State the intent before building — purpose, pixel or print size, palette, output format — so the user judges
  against a target.
- Validate after every few operations; a fault is fixed on the layer that holds it, never painted over.
- A pixel sampled on a named layer tells a wrong layer value from a compositing effect.
- The context (colours, feather, opacity, line width, interpolation) is shared with the GIMP window and the user
  may change it; set what an operation depends on instead of assuming it. Push and pop the context around helpers.
- Plan the layer stack before drawing: one element or effect per layer, named, grouped by role.

## Choosing the route

| Need | Route |
|---|---|
| One fill, shape, adjustment, text or export at default settings | The dedicated tool |
| Layer modes, groups, masks, paths, non-destructive filters, precise export options, repeated shapes | The exec console |

- Define helpers once per GIMP session — colour, polygon, fill, stroke, filter, export — and reuse them.
- The API reference is `developer.gimp.org/api/3.0/libgimp/`; listing an object's methods in the console is faster
  and matches the installed version.
- A PDB procedure is configured by argument name: look it up, read its argument list, set each by name, run it.
  Its result starts with a status, and success is `Gimp.PDBStatusType.SUCCESS`.

## Canvas setup

| Output | Canvas |
|---|---|
| Icon, UI element | Square or target aspect, sides a power of two (256, 512, 1024), RGBA. |
| World texture, sprite atlas | Power-of-two sides, each a multiple of 4 so the engine can compress and stream it. |
| Pixel art | The native small size; enlarged only at the end by an integer factor. |
| Print piece | Trim size plus 3 mm bleed on every side at 300 DPI — A5 is 1819 × 2551 px, A4 2551 × 3579 px. |
| Web or social image | The platform's pixel size at 72 DPI. |

- A new image needs a background layer inserted and a display opened so the user can watch it.
- Image resolution only matters for print: set it to 300 so exports carry the physical size.
- Guides mark trim, safe zone and grid cells; they never export.

## Colour

- Colours are hex strings (`#rrggbb`, `#rrggbbaa` for alpha) or CSS names; `rgb()` takes 0–1 floats, and 0–255
  values clamp to full intensity.
- Pull colours from the project's palette or brand sheet when one exists; pick by meaning, not by look.
- Fills and strokes use the context foreground or background, so set the colour immediately before each one.
- Value contrast carries readability at small sizes: check the result desaturated.

## Building blocks

| Goal | API |
|---|---|
| Filled polygon | `Image.select_polygon`, then `Drawable.edit_fill` |
| Rectangle, ellipse, rounded rectangle | `Image.select_rectangle` / `select_ellipse` / `select_round_rectangle` |
| Outline of a selection | `Drawable.edit_stroke_selection` — width from `context_set_line_width` |
| Smooth curve or filled curved shape | `Gimp.Path` bezier strokes; `Drawable.edit_stroke_item` to outline, `Image.select_item` to fill |
| Inset, outset, ring | `Gimp.Selection.shrink` / `grow` / `border` |
| Gradient | `context_set_gradient_fg_bg_rgb`, then `Drawable.edit_gradient_fill` |
| Erase an area | Select it, then `Drawable.edit_clear` |
| Layer | `Gimp.Layer.new`, then `Image.insert_layer` — position 0 is the top, the parent is a group or none |
| Group | `Gimp.GroupLayer.new`, inserted like a layer |
| Mask | `Layer.create_mask`, then `Layer.add_mask` |
| Blend | `Layer.set_mode` (addition, multiply, screen, overlay), `Layer.set_opacity` |
| Filter | `Gimp.DrawableFilter.new` with a GEGL operation, its config properties, `update`, then `append_filter` |
| Text | `Gimp.Font.get_by_name`, `Gimp.TextLayer.new`, inserted like a layer |
| Copy a layer into another image | `Gimp.Layer.new_from_drawable` |
| Shift content with wrap-around | `Drawable.offset` with the wrap-around offset type |
| Resample | `context_set_interpolation`, then `Image.scale` |
| One undo step for many edits | `Image.undo_group_start` / `undo_group_end` |

- Shapes keep hard edges with antialiasing on and feather off; feather only for a deliberately soft effect.
- Clear the selection after each fill or stroke — a leftover selection clips the next operation.
- `append_filter` keeps a filter editable in the saved XCF; `merge_filter` bakes it into the pixels.
- A filter is named by its GEGL operation (`gegl:gaussian-blur`, `gegl:dropshadow`); an unknown name fails in the
  constructor, and the console's has-operation check reports false for every name, so test by constructing.
- A filter's property names come from listing its config's properties.
- Font names are family plus style (`Arial Bold`, `Arial Regular`); a bare family fails, except the generic
  `Sans-serif`, `Serif` and `Monospace`. List the installed fonts before choosing one.
- A text layer sizes itself to its content: read its width and height after creation to centre or align it.

## Looks

| Look | Build |
|---|---|
| Flat vector | Hard polygon fills on separate layers, a limited palette, no filters. |
| Glow | The shape repeated on an addition-mode layer beneath it and blurred; bright thin core, dim wide halo of the same hue. |
| Drop shadow | `gegl:dropshadow` on the layer — offset, radius, grow, colour, opacity. |
| Outline | Stroke or border the shape's selection; text layers carry their own outline settings. |
| Shading, vignette | A gradient on a multiply or overlay layer above the art. |
| Pixel art | Antialiasing off, pencil strokes, no-interpolation scaling by integer factors, indexed palette. |
| Photo composite | Layer masks instead of erasing, so every cut can be revised. |

## Game content

- An icon is judged at its smallest display size: one silhouette, strong value contrast, the subject centred with
  roughly a tenth of the side as margin.
- A set of icons shares canvas size, margin, line weight, lighting direction and palette.
- UI art ships as PNG with alpha; UI textures import without mips and uncompressed.
- Alpha doubles a texture's memory, so world textures carry it only when it is used.
- A sprite sheet is equal frames on a grid: build each frame in its own image or layer, copy them into one sheet
  image at cell offsets, export once.
- A tiling texture is checked by offsetting it half its size with wrap-around, which moves the seams to the centre.
- DDS export exists with mipmaps and BC formats, but the engine compresses on import — deliver PNG unless a DDS is
  asked for.

## Print pieces

- Background colour and edge imagery run to the canvas edge through the bleed; text, logos and QR codes stay at
  least 3 mm inside the trim, 5 mm when possible.
- Point sizes convert at 300 DPI as one point to 300/72 px.
- GIMP works in RGB; JPEG, TIFF and PSD exports write CMYK when asked. The conversion follows the image's
  soft-proofing profile, so set the printer's CMYK profile as the simulation profile first — Windows ships
  `RSWOP.icm` in its colour folder.
- PDF export stays RGB; it can keep vectors and text layers, one page per layer, or merge them.
- GIMP draws no crop marks; the print shop or a layout tool adds them.
- A multi-page or text-heavy document belongs in a layout tool — build its images in GIMP.

## Export

| Target | Procedure — key arguments |
|---|---|
| PNG | `file-png-export` — alpha kept, compression |
| JPEG | `file-jpeg-export` — quality 0–1, `cmyk` |
| WebP | `file-webp-export` — `lossless`, quality |
| TIFF | `file-tiff-export` — compression, `cmyk` |
| PSD | `file-psd-export` — layers kept, `cmyk` |
| PDF | `file-pdf-export` — `vectorize`, `layers-as-pages`, `convert-text-layers`, `fill-background-color`; `file-pdf-export-multi` for several images |
| GIF | `file-gif-export` — `as-animation` turns layers into frames |
| DDS | `file-dds-export` — `compression-format`, `mipmaps`, `srgb` |
| Any, default options | `Gimp.file_save` picks the format from the extension |
| Editable source | `gimp-xcf-save` — layers, groups, masks, text and live filters kept |

- Every export runs non-interactive on the image and a file object, and composites the visible layers with
  alpha — the image itself is left unchanged.
- The dedicated export tool flattens the open image by default; turn that off or export through the procedure.
- Save the XCF next to each delivered file so it can be revised.
