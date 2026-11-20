# Nintendo DS graphics pipeline: functional redesign

## Top: actual perspective 3D

The rendering code in `source/world.c` builds an original low-polygon nursery using hardware 3D quads and shaded ellipsoid meshes. It has distinct modeled objects in 3D space: flooring and floorboards, a window with frames and drapes, a crib with rails, dresser and three drawers, two wall paintings, a toy shelf, blocks, a colorful rug, a teddy bear, and a separate articulated baby with clothing, hands, feet, hair masses, a clip, eyes, and mood-dependent mouth expression. Perspective projection and a look-at camera reveal depth rather than flat screen-space decorations.

No texture allocations, imported commercial art or per-frame heap allocations are required. The visual direction keeps the awkward, brightly colored low-budget DS baby-game quality deliberately, but should read as a recognizable nursery with recognizable characters. Per-frame polygon budgets must still be measured on hardware.

## Bottom: touchscreen bitmap UI

The sub engine uses `MODE_5_2D`, `BgType_Bmp16`, `BgSize_B16_256x256` on `VRAM_C_SUB_BG`. This is ~128 KiB of dedicated graphics VRAM, **not another system-RAM framebuffer**. The 256×192 visible pixels are drawn directly to hardware BG memory. The native UI includes original 5×7 font data, status meters, four stylus actions, readable baby dialog, menu navigation, an arcade cabinet grid, reaction prompts and progress dots, ending ceremony panels, and a clearly gated reset screen.

The lower screen only redraws when state changes or around seven times per second during the timing game, avoiding unnecessary writes each VBlank.

## Hardware and testing gates

The base DS has about 4 MiB of main RAM and strict polygon/texture/VRAM budgets. Target ~60 Hz VBlank loop, avoid heap churn and keep visible geometric primitives well below 2,048 polygons per scene. These are **design targets, not yet measured results**.

Tests needed: devkitARM + libnds ROM compile; melonDS rendering/touch, minigames and saves; original DS performance, DLDI/storage and stress test at maximum corruption. `tests/test_ui.c` verifies host-side drawing and touch hitboxes; `tests/test_core.c` checks simulation, endings and recovery.

Still missing for full production: authored mesh/texture assets, unique minigame mechanics, sound/music, refined gameplay animation, tiny neural inference, renderer profiling, actual .nds build verification.

## Geometry-batch correction

Rounded meshes use DS-native `GL_QUAD_STRIP`: 8 angular segments × 3 bands, 24 quadrilateral polygons but only 54 submitted vertices per mesh rather than 96 with independent quads (44% reduction). Boxes use 6 quads/24 submitted vertices. `world_vertex_count()` and `world_polygon_count()` report submitted geometry each frame, making the 6,144-vertex and 2,048-polygon caps auditable. This is a source-level budget improvement, not an emulator FPS benchmark.

## More natural-looking baby and nursery — rendering pass (2026-11-20)

A visual review of the real DeSmuME gameplay capture showed that the original
baby had a caricatured bulging-eyed face, black hair clumps, detached-looking
extremities, highly saturated furniture and a floor-board pattern resembling
square tiles. This patch alters **the actual DS hardware-rendered gameplay
model**, not a stand-alone concept image.

- Baby: a smoother 12-sided, five-band head contour, toned-down facial
  proportions, proportionally smaller inset eyes with eyelids, irises and
  restrained highlights, subtle nose/mouth/cheeks, warm skin and chestnut hair,
  better connected limbs, a woven teal top, cream bib and soft booties.
- Room: daylight-inspired cream and sage wall colors, warm wood floors,
  staggered plank joints, muted curtains/furnishings, cotton pillow and framed
  wainscot detail, instead of candy-pink material blocks.
- Framing: camera closer and more squarely on the baby, retaining recognizable
  crib, dresser, window, rug and toys within the hardware perspective room.
- Geometry: inexpensive 8-sided ellipsoids remain for ordinary props, while
  only the three prominent character masses use 12-sided contours.
  Palette-based vertex shading produces material depth without a texture atlas.

There are **zero new textures, 3D VRAM allocations, heap allocations, or
framebuffers**. A few static surface quads replace full extruded blocks.
The hardware geometry regression test enforces **<=1,900 polygons and
<=5,600 submitted vertices** (below the theoretical 2,048/6,144 DS maxima).
Those counts are **software submission estimates**, not device FPS
measurements. Visual inspection of real DeSmuME screen captures is required
before merging the art pass, as is a passing native touchscreen integration
test to avoid regressing the known-good BlocksDS input behavior.

The aesthetic aims for a **soft, believable 2000s-era DS nursery**, not
modern high-polygon realism that would be impossible on the original DS.

## Uneasy early-DS baby-simulator reference (2026-11-20)

The target is not a modern realistic baby or a polished nursery. The user
provided a still of an early 3D baby-care game: **short jagged near-black
hair, pink clip, small dark eyes, round doll-like face, oddly fixed open
smile, pink shirt, dark trousers, orange teddy sitting between splayed
legs, lime-green flower rug, tulip decals and creamy nursery furniture**.
The strange combination of artificial cheer and low-detail realism is
intentional.

`source/world.c` is edited to implement that look on the actual DS GPU,
not with a generated image or a prerecorded sprite. The new doll-like
face, feathery hair points, altered pose and centered bear are all
real 3D geometry, while flower decals and clothing prints are cheap
flat quads. This patch does not copy any game assets or textures from
the supplied reference; it reconstructs its **visual direction** from
original procedural geometry and DS palette colors.

Stay within the stricter **1,900 polygons / 5,600 submitted vertices**
test headroom. No imported textures, heap allocation, additional
framebuffers or changes to the saved-game schema. Real emulator captures
and the touchscreen START/FEED regression tests are required prior to
merge. Cosmetic references should never be passed off as tested ROM screenshots.
