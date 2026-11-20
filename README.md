# My Baby A.I.! (Nintendo DS homebrew)

An ugly-cute DS-era baby simulator parody with an overeducated baby, 16 arcade games (8 initially unlocked), increasingly invasive comedy glitches, and unlockable **good and bad endings**, all with a continue-playing option.

## First playable prototype

The ARM9 program renders a furnished perspective-3D nursery and a modeled, animated, intentionally ugly-cute baby, with crib rails, dresser drawers, window, play mat, toys and teddy bear. The lower screen now uses a colored bitmap touchscreen interface, not the old text console. Buttons, arcade cabinets, care meters, prompts and ending cards are drawn directly into the DS sub-engine VRAM. There are 16 arcade entries: **Bottle Balance** and **Diaper Dash** now have distinct mechanics and touchscreen scenes; the other 14 retain a temporary directional challenge. The baby uses a compact, fixed-weight, integer-only neural intent classifier with 24 authored responses—not a modern generative language model.

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

**Verification status:** GitHub Actions has successfully cross-compiled a real `.nds` ROM and run host tests on a CI pull request; emulator boot/screenshots, original-hardware performance, RAM usage and persistent save behavior have **not** yet been independently verified. Two cabinets have distinct minigames, while the other fourteen still share the prototype direction challenge; all sixteen are not yet finished. See [development status](docs/DEVELOPMENT_STATUS.md) and [release QA](docs/QA_RELEASE.md).

### Title screen, album, settings

A proper startup screen offers touchscreen/START entry to nursery. From the nursery press **L** to view the ending collection (records good and bad endings without losing progress). Press **R** to toggle visual corruption effects, while retaining gameplay interference. Holding **L+R** after enough bad endings enters the separately-confirmed secret reset ending. The new `source/ui.c` renders all of these screens in native DS VRAM.

## Downloading the compiled Nintendo DS ROM

Every push to `main` or `ci/*` and every pull request runs [the GitHub Actions DS builder](https://github.com/masong0508-lab/my-baby-ai-ds/actions/workflows/nds-build.yml). The workflow installs the official devkitPro `nds-dev` packages in the devkitARM container, runs portable tests, calls `make clean && make -j2` using this repository's `Makefile`, checks the ROM header, and uploads an artifact named `my-baby-ai-nds` containing `my-baby-ai.nds`. Open the latest **successful** run, scroll to **Artifacts**, and download it. A build failure means there is no verified ROM artifact yet. This is original homebrew, not a commercial DS game.

## Planning and developer documentation

These files describe the current codebase and distinguish confirmed implementation from proposed enhancements:

- **[Current development status](docs/DEVELOPMENT_STATUS.md)** — implemented systems, known gaps, and the meaning of a successful ROM build.
- **[Prioritized development roadmap](docs/ROADMAP.md)** — staged work, acceptance criteria and a complete set of 16 proposed distinct arcade mechanics.
- **[Feature suggestions](docs/FEATURE_IDEAS.md)** — baby personality, nursery interactions, cosmetics, comedy glitches and hardware-friendly polish.
- **[Testing and release checklist](docs/QA_RELEASE.md)** — emulator/original-DS verification, FPS/RAM telemetry, save safety, CI and release gates.
- **[Glitch endings](docs/glitches.md)** — ending conditions, continuing play and optional reset safety.
- **[Graphics pipeline](docs/graphics-pipeline.md)** — hardware 3D and touchscreen rendering approach.

**Recommended next engineering patch:** run the latest ROM in melonDS, record genuine screen captures and performance readings, then fix any boot, input or save issues *before* enlarging the game.

### Arcade gameplay patch: two distinct cabinets

- **Bottle Balance (#1):** keep a bottle centered despite deterministic rocking-table drift. Hold Left/Right on the D-pad or touch/hold the left/right steering panels. Stay within the central safe zone long enough to fill 150 stability frames; three spills or running out of time means defeat. At high corruption, steering is visibly reversed.
- **Diaper Dash (#2):** use four separate timed tasks in sequence: WIPE (left), NEW DIAPER (right), FOLD (down), TAPE (up). Use D-pad or the corresponding stylus zones. Three mistakes or expired tasks fail the challenge; high corruption may reverse one explicitly signposted task.
- **Cabinets #3–#16:** still use the original directional-response proof-of-concept until their individual mechanics are implemented.

Both games reuse the existing arcade win/loss economy and **do not change the on-disk save format**. Dedicated automated tests are in `tests/test_minigames.c`, and the existing GitHub Actions workflow continues to produce the `.nds` build. Actual emulator/physical-device behavior still requires validation.

## DraStic / legacy-emulator boot compatibility (test build)

**User-reported bug:** A previous `.nds` build opens to **two white screens** in DraStic. A successful cross-compile is not proof that a specific emulator or flashcart can boot it.

The runtime now paints the lower-screen title interface immediately after video setup, **before optional file storage initialization and before the complex nursery render**. The title's upper-screen image uses a lightweight scene. Normal gameplay remains accessible in RAM-only mode by pressing **A** or **START**.

**Important storage behavior:** Press **X on the title screen** to attempt to enable DLDI/FAT saves and load a saved game. This is intentionally opt-in until boot problems on older emulators have been traced. If storage is unavailable, simply restart and enter via START/A to play without persistent saving. Do not press X if the save probe itself causes a hang. The old two-slot save format has not changed.

GitHub Actions now uploads **two ROMs** in each artifact:

- `my-baby-ai.nds` — normal build from devkitARM.
- `my-baby-ai-ds-only.nds` — experimental legacy compatibility copy with the **DS-only unit-code byte** and recomputed header CRC. Program data is identical.

**Suggested test:** First try the DS-only ROM, starting a *new emulator session*, not a saved state. If it still gives two white screens, try the normal ROM and disable CPU JIT in the emulator if that option exists. If the lower title screen appears but selecting **X** freezes, the problem is probably in emulator storage support rather than initial video setup. If START leads to a freeze, investigate the heavy 3D renderer. Record which stage fails. Neither variant is currently certified by a successful DraStic or melonDS runtime test.

Hardware note: DraStic's emulation differs from current melonDS; devkitPro/libnds v2 changes have been reported to affect some older emulators, so this is a compatibility test, not a guaranteed final fix.

## DraStic white-screen compatibility investigation

A tester reported that the previous binary booted to **two entirely white screens** in DraStic before any game interface appeared. A successful GitHub Actions cross-build does not constitute an emulator boot test.

This compatibility branch introduces **two independent mitigations**:

1. The startup shows the title on the DS **2D display engines first**, without initializing the full 3D nursery or FAT storage. A purple top screen and a colorful bottom title confirm that the program reached its first graphics frame. Press **A/START/touch** to initialize the 3D engine and enter the nursery.
2. GitHub Actions also packages a validated **DS-only** compatibility ROM (`my-baby-ai-ds-only.nds`) alongside the standard `my-baby-ai.nds`. This adjusts cartridge header unit code 0x12 and recalculates header CRC16 without changing the game binaries.

**Important temporary save change:** at the title screen, press **X** to attempt loading/enabling DLDI/FAT saves. The game otherwise starts in RAM-only mode, deliberately avoiding potentially blocking emulator storage init. Do not assume progress persists unless the title shows that saving is enabled. Try the DS-only binary in DraStic using **Start Game**, not an emulator save state from the broken ROM.

For a diagnostic report, record whether (a) the purple/colorful title appears, (b) pressing START enters the nursery, and (c) pressing X at title freezes or reports no save device. A white screen *before* the title in both ROM variants suggests a deeper emulator/runtime incompatibility and needs an emulator-level reproduction; it is not proof of a 3D scene bug.

This is a compatibility **candidate**, not a runtime-verified fix.
