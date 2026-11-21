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

## Camera art-direction pass

ROOM now uses closer-to-frontal two-subject compositions rather than turning the camera so far left that empty sage wall consumes most of the top DS screen. The window, lamp, Teddy and crib remain real fixed landmarks in the same geometry, and the usual baby is kept recognizable in shot. A tiny five-quad framed moon painting adds real visual storytelling to the left interior wall; it uses no imported image, sprite or texture memory and avoids thin horizontal lines that caused previous rendering glitches.

When WINDOW switches to day, the wall, floor and furniture also receive one or two 5-bit brightness steps, so the lighting actually tells a daytime story rather than changing only the glass color. Night/lamp-off remains the dimmest preset, preserving the quiet crib care bonus. This is a temporary renderer palette adjustment; the baby's material palette, Toybox and secret egg cinematic are restored immediately after each room draw. Native tests check a right-wall region outside the window and require a visible daylight difference.

## Crib mobile: attached movement instead of floating ornament

The real crib now has four hanging geometric charms tied to the existing crossbar. Each visible string and colored ornament rotates as one parented 3D element around its own attachment point, so it never floats away from the support. Tapping CRIB triggers one measured 40-tick swing (a gentle left lean, right follow-through, then natural rest). The normal CARE view, Toybox and the alien-egg cinematic retain the **same** mobile geometry at rest, ensuring spatial continuity and preventing props appearing or vanishing between scenes. Repeated CRIB taps can restart the motion; leaving it alone does not produce endless looping.

Rendering cost: just **12 hardware quads** and 48 submitted vertices on the existing DS polygon display list. The mobile uses flat two-dimensional-looking colored polygon facets in a real 3D location; no new textures, VRAM allocations, runtime heap memory, or transparency buffers. Host geometry tests enforce limits throughout the 40-tick animation, and the native DeSmuME walkthrough captures the mobile both in motion and at rest for visual verification.

## Audible nursery reactions and sound accessibility

The live care actions now play distinct short original melodies on the **existing single Nintendo DS PSG voice**: a welcoming ascending FEED jingle, three CHAT syllable pitches, a PLAY arpeggio, and a soft descending NAP phrase. WINDOW and LAMP switches make a brief confirmation click. Touching CRIB starts one finite mobile chime alongside its animated 40-tick hanging ornaments and normal NAP/quiet-night gameplay.

These are synthesized directly from the game tick without audio samples, an external music bank, a second sound channel, DMA or extra heap allocations. All melodies end automatically. The alien egg's shell-opening and jump-scare cues keep precedence over nursery sounds, so a low-priority chime cannot spoil the cinematic.

**Sound control:** while in ROOM, press **Y** or **tap the Y SND ON/OFF footer label** to mute/unmute the entire game's PSG output; the change is immediately displayed. The visible **A USE footer label** also accepts stylus taps to repeat the selected prop action. B/Start or the BACK footer still returns to CARE. The sound setting is shared with Toybox's existing Y mute and does not modify saved care stats or save serialization. Both physical keys and direct native DeSmuME stylus taps have dedicated regression checks. Silent play retains every animation and gameplay interaction.
