# Touch-accessible Save / Load on the Nintendo DS title

## Player-facing change

The game originally displayed **X SAVE** at the bottom of the title screen but started the game for every stylus tap. Touch-only players could not initialize FAT storage or resume a prior baby, even though the X controller shortcut worked.

The original wide yellow START bar is now two distinct native 256×192 cards: a **yellow START** area and a **mint SAVE/LOAD** area. Tapping SAVE/LOAD (x=159..236, y=140..167) invokes **exactly the existing X-key storage path**: briefly show CHECKING SAVE STORAGE, initialize the removable save device on explicit user request only, then show SAVED BABY RESTORED / SAVING ENABLED / NO SAVE DEVICE - RAM ONLY. It stays on the title so the player can confirm status and then start.

## Boot integrity

- No automatic DLDI/FAT access during the initial title drawing or first START action. This protects older DS emulator boot paths; the storage check remains opt-in.
- No change to binary save records, two-slot recovery logic, original X key behavior or A/START controls. The existing center-portrait touch at (128,115) still launches the nursery.
- No new font assets, heap allocations, ROM image textures, VRAM buffers, per-frame 3D geometry, or rendering costs outside the title's one-time bitmap update.
- Screen touch bounds come from a pure integer `ui_title_save_touch(x,y)` function. The host test scans every one of the 49,152 visible touch coordinates and confirms only the intended save card activates it.
- The real DeSmuME walkthrough touches SAVE/LOAD first, checks the title's displayed storage result changed, then uses SELECT to verify the game was not accidentally launched. It then proceeds through the existing stylus, Room Tour, all-arcade and secret-egg tests.

## Honest release limitations

The tap proves the same storage path can be triggered by a stylus. Actual persistence across physical DS power cycles, DLDI/flashcart filesystem support, and specific DraStic storage backends still need explicit hardware/runtime validation.
## Additional in-game save access

A player who already entered the nursery in RAM-only mode must not lose their baby simply because they missed title-screen X or SAVE/LOAD. **Room Tour** now shows an explicit **SAVE** badge in its top status panel (x=198..243, y=49..62), also reachable with physical X while in Room Tour. On that action only, the game tries initializing the storage device if necessary, then calls the existing two-slot `save_write` on the **current in-memory baby**. It never invokes `save_load` while gameplay is in progress, so a previously saved baby cannot silently replace unsaved nursery progress. It leaves the player in the same Room Tour and shows either BABY SAVED, SAVE WRITE FAILED or NO SAVE DEVICE.

The Room Tour's four actual furniture cards, footer BACK/SOUND/USE hitboxes, global audio mute and camera focus are unchanged. Host tests assert precise edge coordinates; the native DeSmuME run touches the badge, photographs the status, and continues the original room-to-arcade-to-egg walkthrough. Saving remains **explicit**, so simply visiting Room Tour is guaranteed not to probe FAT.
