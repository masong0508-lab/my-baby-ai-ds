# Room Tour: continuous baby attention

Switching furniture previously eased the camera from its current position but
restarted the baby's head turn from straight ahead. A quick WINDOW → CRIB turn
therefore snapped the face toward the player before turning to the new object.
Leaving Room Tour also cleared the head angle immediately.

The head now uses the same 16-tick transition as the camera, beginning at the
current sampled attention angle. Each new target captures that angle before
restarting the transition; returning to ordinary care eases back to neutral.
Reusing the selected furniture keeps the existing turn in progress. Eyes, hair,
cheeks and mouth remain together under the existing head transform.

The change uses one transient integer anchor. It adds no polygons, textures,
framebuffers, heap allocations, save fields or gameplay effects. All four
furniture targets keep their original final angles.

Host renderer tests cover all 25 target pairs (four props plus neutral), each
possible interruption tick, immediate reversals, exact starting/ending angles,
bounded two-degree frame steps and unchanged mesh submissions/saved game data.
With the accompanying hand-cap correction, the complete renderer test reports
a maximum of 4,702 submitted vertices and
1,808 polygons, within the existing 5,000/1,850 limits. These counters are not
measured FPS. DS builds, native emulator screenshots and the Room Tour walkthrough
remain the integration gates; no physical DS or DraStic performance result is
implied by the host checks.
