# Secret alien-egg film — native DS direction and acceptance

This milestone changes **the actual in-engine 256×192 Nintendo DS film**, not concept art. The same physical 3D nursery, shell crescents, recess, creature, and parental mesh transforms appear throughout; there is only one deliberately hard camera cut on the scare.

## Camera and acting beats

| Game tick | Beat | Actual code |
| --- | --- | --- |
| 0–71 | Fractured doll, readable opening portrait | `world_egg_staged_doll`; original Toybox physics is not rewritten |
| 72–157 | Reveal the recessed egg and identifiable nursery | `world_secret_camera`, track 1: easing toward the opening, 30% travel |
| 158–279 | Push inside the egg, awaken the membrane and eyes | `world_secret_camera`, track 2: additional 70% travel; `world_egg_membrane_pulse`, `world_egg_eye_glow` |
| 280–297 | Final contradiction: forward-moving camera, creature retreats while eyes fade | `world_egg_embryo_retreat`, max 0.08 world units |
| 298–374 | One hard cut and a single creature lunge, jointed jaw and fangs, then stillness | `world_egg_creature_depth`, `world_egg_head_tilt`, `world_egg_jaw_angle` |
| 375 | Existing secret-ending postcard and continue flow | Normal game state machine |

The camera stops smoothly at the PUSH boundary rather than restarting or teleporting. Embryo motion starts and stops smoothly. The final retreat acts on the same parent transform as the eyes, mouth and silhouette. The membrane changes the radius of an **existing** mesh; it does not add animated polygons. The original long, thin scare-brow glint was replaced by two short, taller marks to avoid a rasterized horizontal ruler.

## Hardware and game-design invariants

- **Original DS budgets:** 5,000 submitted vertices and 1,850 polygons are enforced in the host renderer test (below the device's theoretical processing limits). These are submission counters, **not measured FPS**.
- No texture sheets, external art assets, heap, framebuffers, save-schema changes, additional GPU layers, random per-frame particle allocations, or new sound streams.
- All live dialogue/care, Toybox, ending unlocks, stage-pause/skip and replay use the existing paths. The unrelated nursery-audio work remains in its own PR.
- Pure frame samplers mean pausing, replaying and skipping the film cannot change the animation timing or spawn an orphaned facial piece.
- Additional deterministic gaze acting is layered onto the existing eyes: tiny movements as the camera approaches, then synchronized lock-on of both irises and catchlights during the final one-shot lunge. No separate pupil textures or additional GPU polygons are introduced.
- In horror close-ups, eyes, cheeks, tongue and fangs are still connected through a single parent cranium transform; the lower jaw retains its own bounded hinge.

## Mandatory validation

1. `sh tests/run.sh` must cover **each anticipation frame**, segment-boundary continuity, finite membrane motion, one 18-tick retreat, creature lunge/jaw behavior, and GPU submission budgets.
2. Standard devkitPro build must produce an .nds ROM and pass header checks.
3. BlocksDS builds and the **actual DeSmuME native touch walkthrough** must pass, including the egg film.
4. Inspect generated `emulator-proof/game_egg_green_ichor.png`, `game_egg_slow_push.png`, `game_egg_anticipation_close.png`, **`game_egg_final_recoil_before_cut.png`**, `game_egg_jump_scare.png` and `game_egg_creature_lunge.png` at native resolution. Check that the camera genuinely closes in, the mouth stays below both eyes, the shell remains visible, no thin cyan/white streaks span the facial area, and the late recoil is discernible.
5. No claims of 60 FPS, measured latency, RAM headroom, Android DraStic compatibility or original DS/DS Lite performance until those platforms are actually measured.

If the native screenshots look worse despite tests passing, **do not merge on tests alone**. Capture a real comparison to the previous main ROM before changing further camera targets or pupil layout.
