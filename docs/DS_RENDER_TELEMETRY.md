# Live Nintendo DS render-budget diagnostics

The existing SELECT-on-title stylus-test page now displays two **actual top-screen renderer submission counters**: VERTS (vertices) and POLYS (polygons). The same in-game 3D renderer already counts these on every frame. This change makes those numbers visible without additional geometry, image files, offscreen VRAM, save fields, network, or a profiling thread.

## How to use

1. Launch the DS ROM and on the title screen press SELECT to open STYLUS TEST.
2. Inspect the lower screen's `VERTS` and `POLYS` numbers beneath the touch canvas. These values update from the most recent 3D frame. Continue to test stylus taps and X/Y movement as before.
3. The numbers turn rose when submitted vertices exceed 5,000 or polygons exceed 1,850, matching the host-side per-frame guardrails.
4. Press B, START or SELECT to return to the title screen.

## Scope and interpretation

- The values are **submitted software geometry counts**, not the DS hardware's actual polygon throughput, measured FPS, per-frame frame time, VRAM free space, or ARM9/ARM7 RAM watermark.
- Updates are only active on the optional diagnostics screen, and redraw just 65×11 pixel strips when values change. All other views preserve the existing VRAM throttling.
- The title, nursery, room, toybox, arcade, save system and secret-egg cinematic do not gain new rendering work.
- Host tests check the actual diagnostic image changes when each count changes, remains identical when unchanged, and displays the over-budget state. The standard DS/BlocksDS builds and native DeSmuME stylus walkthrough remain mandatory merge gates.

Physical original DS testing is still needed before claiming sustained 60 FPS or any measured RAM headroom.