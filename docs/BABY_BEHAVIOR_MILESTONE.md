# Baby personality milestone: readable needs on the DS model

This change makes the already-persisted nursery needs visible on the **actual top-screen 3D baby** as well as in the lower-screen UI. It introduces no new save fields, textures, framebuffer buffers, particles, heap, dynamic meshes or per-frame arrays.

## Player-visible acting

| Saved need | Baby body and face | Intended response |
| --- | --- | --- |
| Energy ≥ 55 | Original wide-awake, natural blink | Normal care and chatting |
| Energy 0–54 | Upper eyelids gradually narrow, reaching a bounded 96/256 aperture at zero; both attached arms settle up to four degrees lower | Rest / NAP recovers energy, restoring open eyes |
| Hunger ≥ 70 | A subtle idle neck fidget with amplitude rising as hunger climbs | FEED lowers hunger; the fidget tapers away |
| Chaos ≥ 70 | Adds a small amount of restless idle motion, limited to 4 degrees total with hunger | Rest or other calming gameplay reduces chaos |
| Hunger ≥ 80 or Energy ≤ 15 | Gentle worried mouth even if an old Mood number remains high | The baby no longer appears delighted when immediately in need |
| CHAT/PLAY/NAP active | Existing 56-tick gestures retain priority; the idle fidget is proportionally suppressed | Care still feels intentional rather than continuously twitchy |
| Toybox | Original deterministic impact recoil and film acting only | No interference with collisions or special ending |

## Performance guarantees and tests

- The fatigue eyelid uses the already-present partial-blink geometry and stays broad enough to avoid subpixel collapsed-ellipse artifacts. It does not add a second eye rig.
- The idle neck pivot rotates **all** facial pieces together, never separately moving eyes or hair. All calculations are integer-only with fixed-duration triangle motion.
- Rig samples are reproducible from the current Game state and frame, independent of random calls and saved schema.
- Host tests sweep 100 energy levels and 250 frames of maximum restlessness, assert deterministic periodicity, ≤1-degree per-frame fidget, and verify the same Nintendo DS submission limits (≤5,000 vertices and ≤1,850 polygons) during exhausted idle and CHAT.
- A passing host test is not proof of physical DS frame rate. Before merging, run both CI ROM builds and the real DeSmuME walkthrough; inspect native screenshots for eyelid clipping or detached cheek geometry.
