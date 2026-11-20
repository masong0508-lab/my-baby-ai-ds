# My Baby A.I.! (Nintendo DS homebrew)

An ugly-cute DS-era baby simulator parody with an overeducated baby, 16 arcade games (8 initially unlocked), increasingly invasive comedy glitches, and unlockable **good and bad endings**, all with a continue-playing option.

## First playable prototype

The ARM9 program renders a purposely awkward, low-poly nursery and baby with the Nintendo DS 3D engine (no textures or extra framebuffers), and the lower-screen console offers touch and button actions. There are 16 catalog entries; the minigames currently share a *temporary* directional/timing challenge. The baby uses a tiny, deterministic dialogue/state engine rather than an unimplementable large model. A quantized classifier is planned.

### Controls
* A feed, B chat, X play, Y nap; touchscreen quadrants do the same.
* START enter arcade; UP/DOWN choose, A play, X unlock, B exit.
* In an arcade challenge press the prompted direction. At higher corruption tiers, REVERSE means the opposite.
* SELECT end the day and check for good or bad endings. After a finale, press A or B to **continue the same save**. Four good endings give one-time Diaper Dollar and Favor rewards.
* After three distinct or five total **bad** endings (good endings never count), hold L+R to open an **optional** reset ending. A continues, B cancels, then holding L+R and pressing X confirms. Only files belonging to this game are erased.

### Build
Install the official devkitPro **nds-dev** group (devkitARM, libnds and related libraries). Export DEVKITPRO and DEVKITARM. Run `make`. The output is `my-baby-ai.nds`. GitHub Actions builds with the official devkitPro Docker image and uploads the ROM. Use `sh tests/run.sh` for fast host-side regression tests.

The current design targets original DS hardware (~4 MiB RAM). 60 Hz is a design target, not a measured claim. A passing CI build does not substitute for melonDS or real-hardware FPS/VRAM testing. Saving requires DLDI-compatible FAT storage; the game runs in RAM-only mode without it.

See [docs/glitches.md](docs/glitches.md). Everything is original parody; no commercial assets.

## Good endings and continuation

Discover **Tiny Diploma Ceremony**, **Rattle Arcade Champion**, **Bugs Become Features**, and **Second-Chance Daycare** by caring well, winning minigames, managing corruptions, or recovering from a bad ending. Every discovered good ending awards $8 and 1 Favor once; the optional ending screen returns to nursery on A/B without clearing the current game. Bad endings also let the player recover and continue. Existing version-1 saves remain compatible because ending progress uses previously unused bits of `ending_mask`.
