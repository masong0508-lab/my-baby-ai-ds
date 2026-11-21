# Room Tour: interactive nursery furniture

Use the **ROOM** button on the CARE speech panel (or D-Pad Down with virtual stylus off) to visit the nursery furniture. The same native 3D room remains visible on the top DS screen.

| Touch card | Game effect |
| --- | --- |
| WINDOW | Switch the top-screen sky, sun/moon and room window between day and night |
| LAMP | Turn the modeled dresser lamp on/off and change the room ambient colors |
| TEDDY | Focus and animate the actual teddy, triggering the existing PLAY needs and baby gestures |
| CRIB | Focus the real crib and trigger the existing NAP needs and eyelid animations |

Tap a card to use it. With physical controls and the virtual stylus disabled, use Up/Right/Left/Down for Window/Lamp/Teddy/Crib, or A to reuse the selected item. **B/Start** or the touchscreen BACK bar returns to CARE. With virtual stylus enabled, D-Pad moves the cursor and A taps the same hitboxes.

The camera blends five presets over 16 ticks with deterministic smoothstep, avoiding hard cuts or duplicate rooms. Session-only visual state (day/night, lamp and camera) is not added to the existing save schema. PLAY/NAP go through the normal game_care() function and existing save logic. No new textures, extra framebuffers, heap allocation or VRAM surface. Host tests check hitboxes, camera easing and DS geometry budgets, while DeSmuME automation performs actual native stylus taps and verifies top-screen changes. Physical DS / Android DraStic verification and measured FPS are still separate checks.
