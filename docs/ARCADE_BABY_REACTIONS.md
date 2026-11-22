# Baby reactions to completed arcade results

The nursery baby now has two distinct result poses on its existing rig:

| Pose | Acting | Timing |
| --- | --- | --- |
| `WORLD_BABY_CHEER` (7) | Both arms lift to 76 degrees, the body makes one shallow lift, and the head tilts with a small open smile. | Full arm pose at tick 12; lift peaks at 18, lands by 40, and all channels settle by 56. |
| `WORLD_BABY_REGROUP` (8) | Low hands reach forward, the head gives a small acknowledging tilt and returns toward the player, and the face offers a closed smile. | Hands ease into place by 12; the head returns toward the player during 18–34; all channels settle by 56. |

REGROUP softens rested eyelids without making an already tired baby look
sleepier. Both outcomes keep their encouraging mouth expression at low
mood. The natural blink, need-driven idle expression, and breathing resume
normally as the reaction ends. The loss reaction has no shake, fall, or
repeating downcast pose.

## Starting the reaction

Use the named constants only for an actual completed arcade outcome.
They are renderer actions: do not pass them to `game_care` or apply extra
stat changes. Existing care IDs 1–4 and Toybox IDs 5/6 remain unchanged.

At the beginning of an active arcade tick, capture the current pose before
input/timer processing can finish the round and change the baby's stats:

```c
WorldBabyPose before_event = world_baby_pose(&baby, frame, pose, pose_age,
                                            pose ? &care_motion : 0);
```

After the result observer reports a new win or loss, start exactly once:

```c
pose = won ? WORLD_BABY_CHEER : WORLD_BABY_REGROUP;
pose_age = 0;
world_baby_motion_start(&care_motion, before_event, &baby, frame, pose);
```

Existing drawing and the 56-tick lifetime need no new API. The 12-tick
handoff preserves all nine captured rig channels, including an unfinished
care/result blend. Care can immediately interrupt either reaction, and
another result can retarget it without resetting the baby to idle.
Furniture attention remains a separate anchored head turn.

## Cost and checks

The implementation reuses the existing shoulder hierarchy, attached head,
eyelid shutters, smile and mouth geometry. The pose remains 18 bytes and
its motion anchor remains 36 bytes. It adds no textures, meshes, heap
allocations, framebuffers, or save fields, and does not consume game RNG.

Host tests verify the distinct peak poses, monotonic reach/recovery, one
cheer lift, gentle head tilt/eyelids, and idle equality from tick 56 through 180
and at maximum age. Every care/result transition is interrupted at every
old age from 0 through 56 across four energy cases, including blink
boundaries. Another 256 rapid care/result events interrupt ongoing blends.

Tests also complete a real Sock Market win and loss, capture before the
decisive input, and verify that result rendering leaves all Game data
unchanged. Submitted vertex signatures check that both result faces stay
positive at low mood and return to the ordinary idle expression afterward.
All hands retain their 32 valid cap triangles; submitted geometry remains
within 4,702 vertices / 1,808 polygons.

The optimized host test and ASan/UBSan build pass. These check rig values
and submitted geometry; native emulator captures separately verify the
screen composition and actual DS rasterization.
