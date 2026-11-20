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
