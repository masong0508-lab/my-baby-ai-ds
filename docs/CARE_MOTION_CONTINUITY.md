# Interrupted care motion

A care tap previously reset `pose_age` to zero and immediately rebuilt the
baby at its idle pose. Interrupting held PLAY could therefore move the arms
roughly 50 degrees in one frame, erase the hop, and reopen NAP's eyes.

`WorldBabyPose` now samples the actual nine channels used by the renderer:
left/right shoulder reach, left/right forward reach, torso lean, neck tilt,
head yaw, eyelid aperture, and action bounce. Angles use integer degrees;
eyelids use 0..256; bounce stores Q8 hop units, each worth .0022 world units.
The pose occupies 18 bytes. `WorldBabyMotion` holds the captured pose and the
new action's zero-age baseline in 36 transient bytes.

## Handoff

Capture the current rig **before** changing needs. Sample it with the same
motion anchor used for rendering, so another tap captures an unfinished
handoff as well as an ordinary gesture. After care changes the needs, start
the new motion using the updated baby:

```c
WorldBabyPose from = world_baby_pose(&baby, frame, pose, pose_age,
                                    pose ? &care_motion : 0);
message = game_care(&baby, action);
pose = (int)action + 1;
pose_age = 0;
world_baby_motion_start(&care_motion, from, &baby, frame, pose);
```

Room quiet-rest applies additional energy/chaos changes after NAP. Rebase
the new motion after those changes, retaining `care_motion.start` as the
captured pose. This prevents an immediate change in sleepy arms or eyes.
Clear transient motion when resetting/loading the baby or entering Toybox.
Pass a null motion when no care pose is active.

Each channel follows its existing care curve plus the difference between
the captured pose and the new zero-age baseline. That difference decays
with integer smoothstep over 12 ticks. Age zero reproduces the captured rig
exactly; age 12 rejoins the original gesture; age 56 returns to idle.
Starting from unchanged idle leaves the complete original curve unchanged.
Eyelid results are bounded to 0..256 while the natural blink continues.

The main loop increments `frame` and `pose_age` after rendering. Capture at
the incoming tick: it advances the old gesture by its normal single tick
and then preserves that value when the action changes. It does not restart
the global blink, idle bob, or breath. Furniture attention retains its own
camera-linked anchor.

`world_draw_care_from` and the final `world_draw_nursery` argument accept the
motion. Existing `world_draw_care` callers retain the original sampler.
Toybox and staged egg reactions retain their simulation-derived poses.
There are no new meshes, textures, heap allocations, framebuffers, or save
fields. The hand-cap triangles and fixed-thickness eye geometry remain.

## Verification

The host world test checks all 16 care pairs plus starts from idle, every
interruption age from 0 through 56, seven energy cases spanning sleepy
thresholds, all subsequent ages through 56, and natural blink boundaries.
It checks exact handoff equality, bounded per-tick motion, recovery, and
512 rapidly repeated/alternating taps that interrupt previous handoffs.
It also checks quiet-rest baselines and unchanged saved Game data.

Rendering those transitions keeps the two hands' 32 valid cap triangles
and stays within the established 4,702 submitted vertices / 1,808 polygons.
The optimized host test and its ASan/UBSan build pass. These tests establish
rig continuity and command budgets; native screenshot review separately
checks DS rasterization and framing. They are not hardware FPS measurements.
