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