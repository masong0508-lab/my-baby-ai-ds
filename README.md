# My Baby A.I.! (Nintendo DS homebrew)

An ugly-cute DS-era baby simulator parody with an overeducated baby, 16 arcade games (8 initially unlocked), increasingly invasive comedy glitches, and unlockable **good and bad endings**, all with a continue-playing option.

## First playable prototype

The ARM9 program renders a furnished perspective-3D nursery and a modeled, animated, intentionally ugly-cute baby, with crib rails, dresser drawers, window, play mat, toys and teddy bear. The lower screen now uses a colored bitmap touchscreen interface, not the old text console. Buttons, arcade cabinets, care meters, prompts and ending cards are drawn directly into the DS sub-engine VRAM. There are 16 catalog entries; the minigames currently share a *temporary* directional/timing challenge. The baby uses a tiny, deterministic dialogue/state engine rather than an unimplementable large model. A quantized classifier is planned.

### Controls
* A feed, B chat, X play, Y nap; the four large touchscreen buttons provide the same actions.
* START or tap ARCADE to enter arcade. Use D-pad or touch cabinets to select; A/tap PLAY starts, X/tap BUY unlocks, B/tap BACK exits.
* In an arcade challenge press the prompted direction. At higher corruption tiers, REVERSE means the opposite.
* SELECT or tap END DAY to check for good or bad endings. After a finale, press A or B to **continue the same save**. Four good endings give one-time Diaper Dollar and Favor rewards.
* After three distinct or five total **bad** endings (good endings never count), hold L+R to open an **optional** reset ending. A continues, B cancels, then holding L+R and pressing X confirms. Only files belonging to this game are erased.

### Build
Install the official devkitPro **nds-dev** group (devkitARM, libnds and related libraries). Export DEVKITPRO and DEVKITARM. Run `make`. The output is `my-baby-ai.nds`. GitHub Actions builds with the official devkitPro Docker image and uploads the ROM. Use `sh tests/run.sh` for fast host-side regression tests.

The current design targets original DS hardware (~4 MiB RAM). 60 Hz is a design target, not a measured claim. A passing CI build does not substitute for melonDS or real-hardware FPS/VRAM testing. Saving requires DLDI-compatible FAT storage; the game runs in RAM-only mode without it.

See [docs/glitches.md](docs/glitches.md). Everything is original parody; no commercial assets.

## Good endings and continuation

Discover **Tiny Diploma Ceremony**, **Rattle Arcade Champion**, **Bugs Become Features**, and **Second-Chance Daycare** by caring well, winning minigames, managing corruptions, or recovering from a bad ending. Every discovered good ending awards $8 and 1 Favor once; the optional ending screen returns to nursery on A/B without clearing the current game. Bad endings also let the player recover and continue. Existing version-1 saves remain compatible because ending progress uses previously unused bits of `ending_mask`.

## 3D and UI implementation

The top screen now runs a multi-object, shaded, original low-poly scene via the DS hardware 3D engine. The lower screen uses 256×192 native pixel art and interactive status displays with a 16-bit BG bitmap in **128 KiB of dedicated VRAM** (not an extra 128 KiB main-RAM framebuffer), plus a small embedded bitmap font. The UI refreshes on changes, with lower-frequency countdown updates. See `docs/graphics-pipeline.md`.

**Verification status:** portable C simulation/save tests passed previously; host UI raster checks are present in CI. A native `.nds` build, original-hardware frame-rate validation, and emulator screenshots have **not yet been confirmed**. The sixteen cabinets currently share one arcade microgame; they are not sixteen finished unique minigames.

### Title screen, album, settings

A proper startup screen offers touchscreen/START entry to nursery. From the nursery press **L** to view the ending collection (records good and bad endings without losing progress). Press **R** to toggle visual corruption effects, while retaining gameplay interference. Holding **L+R** after enough bad endings enters the separately-confirmed secret reset ending. The new `source/ui.c` renders all of these screens in native DS VRAM.
