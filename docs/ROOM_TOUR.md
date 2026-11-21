# Room Tour: interactive nursery furniture

Use the **ROOM** button on the CARE speech panel (or D-Pad Down with virtual stylus off) to visit the nursery furniture. The same native 3D room remains visible on the top DS screen.

| Touch card | Game effect |
| --- | --- |
| WINDOW | Switch the top-screen sky, sun/moon and room window between day and night |
| LAMP | Turn the modeled dresser lamp on/off and change the room ambient colors |
| TEDDY | Focus and animate the actual teddy with a **finite 40-tick bow**, and trigger the existing PLAY needs and baby gestures; Teddy settles instead of waving forever |
| CRIB | Focus the real crib and trigger NAP needs and eyelids. With **moonlight selected and the lamp off**, receive **+8 additional energy and −4 chaos**, in addition to normal NAP |

Tap a card to use it. With physical controls and the virtual stylus disabled, use Up/Right/Left/Down for Window/Lamp/Teddy/Crib, or A to reuse the selected item. **B/Start** or the touchscreen BACK bar returns to CARE. With virtual stylus enabled, D-Pad moves the cursor and A taps the same hitboxes.

The camera blends five presets over 16 ticks with deterministic smoothstep, avoiding hard cuts or duplicate rooms. Session-only visual state (day/night, lamp and camera) is not added to the existing save schema. PLAY/NAP go through the normal game_care() function and existing save logic. No new textures, extra framebuffers, heap allocation or VRAM surface. Host tests check hitboxes, camera easing and DS geometry budgets, while DeSmuME automation performs actual native stylus taps and verifies top-screen changes. Physical DS / Android DraStic verification and measured FPS are still separate checks.

## Natural reactions and bedtime bonus

The baby turns its complete head—including eyes, cheeks, hair and mouth—slightly toward the prop currently in focus. The movement is eased into the camera's 16-tick travel, with a maximum yaw of seven degrees so the low-poly face stays recognizable. Teddy responds to an interaction with one short animated bow (8 ticks to begin, 10 held, and 22 to relax) instead of an infinite loop. Repeat the action to bow again.

Bedtime is now a small, playable room puzzle: tap WINDOW until the moonlight is showing, switch the dresser LAMP off, then touch CRIB. The normal NAP action still restores 25 energy, and this quieter arrangement provides another **8 energy** while reducing chaos by **4**. Values saturate at their normal limits. Crib's card advertises the bonus only when the player actually has that setup, and the resulting care statistics use the unchanged existing game save format. The bonus isn't tied to a timed limit or hidden random chance.

## Uninterrupted 3D camera journeys

Selecting a new prop before the previous 16-tick camera move ends no longer restarts the lens at the previous preset. The game captures its exact six-component camera position and gaze target at the interruption, then eases to the new landmark. Pressing BACK during a move uses the same continuity rule. This avoids a visible teleport or artificial scene cut when tapping quickly, with no added vertices, textures or framebuffers. The settled camera is returned directly from its preset so idle drawing requires no interpolation arithmetic.
