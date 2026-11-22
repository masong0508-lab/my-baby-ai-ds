# My Baby A.I.! (Nintendo DS homebrew)

An ugly-cute DS-era baby simulator parody with an overeducated baby, 16 arcade games (8 initially unlocked), increasingly invasive comedy glitches, and unlockable **good and bad endings**, all with a continue-playing option.

## First playable prototype

The ARM9 program renders a furnished perspective-3D nursery and a modeled, animated, intentionally ugly-cute baby, with crib rails, dresser drawers, window, play mat, toys and teddy bear. The lower screen uses a colored bitmap touchscreen interface. Buttons, arcade cabinets, care meters, prompts and ending cards are drawn directly into DS sub-engine VRAM. There are **16 distinct arcade minigames** with individual win conditions and touch/button controls, including the full eight-cabinet unlock progression. The baby uses an offline integer-only intent classifier with **48 mood-responsive lines plus 12 recent-event replies**. Nursery care and prop interactions include short single-channel PSG melodies and a Room Tour mute control.

**Cartridge identity:** the game ROMs are packaged with the twelve-byte title `MY BABY A.I.` and game code `MBAI`; diagnostic boot and touch-probe ROMs keep their own identifiers. The header checksum is validated without changing ARM9/ARM7 executables. The game remains native DS software and has no DSi-exclusive memory requirement.

### Care flow and recent memories

Rapid care taps carry the baby's current arm, head, eyelid and bounce pose into the next gesture over 12 game ticks. This includes tiredness changes and the extra recovery from a quiet crib nap. Each gesture still settles back to idle after its original 56-tick cycle.

When the baby is comfortable, CHAT can mention a recent bottle, playtime, nap, quiet crib rest or completed arcade win/loss. Four distinct events fit in a 20-byte session memory; needs take priority, repeated replies are avoided, and events expire with further interactions or a new day. Loading or resetting a baby starts a fresh memory. The existing save format remains unchanged. See [care-motion details](docs/CARE_MOTION_CONTINUITY.md) and [recent-interaction memory](docs/RECENT_INTERACTION_MEMORY.md).

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

**Verification status (November 2026):** Standard devkitARM and independent BlocksDS ROMs have passed CI. Native DeSmuME tests cover stylus input, core views, care/Room Tour, the alien-egg film, a complete Diaper Dash session, and a separate walkthrough of Sock Market, Beatbox, Tax Return Teether and an earned Cereal Cipher unlock. The arcade walkthrough includes selected losses, wrong-answer and timeout retries, visible receipt dragging, and exact reward/purchase totals. **Physical original DS/DraStic operation, sustained FPS/RAM telemetry, long soak tests and real flashcart save persistence remain unverified.** All sixteen minigames have host mechanic coverage; native completion covers five cabinets and Bottle Balance entry. See [native arcade tests](docs/NATIVE_ARCADE_TESTS.md), [development status](docs/DEVELOPMENT_STATUS.md) and [release QA](docs/QA_RELEASE.md).

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

**Recommended next engineering patch:** profile the latest ROM in melonDS and on an original DS/DS Lite, record per-scene FPS and RAM headroom, and test saving across power cycles. Automated DeSmuME touchscreen and cinematic screenshots already exist; they are not substitutes for those additional platforms.

### Arcade gameplay: sixteen distinct cabinets

- **Bottle Balance (#1):** keep a bottle centered despite deterministic rocking-table drift. Hold Left/Right on the D-pad or touch/hold the left/right steering panels. Stay within the central safe zone long enough to fill 150 stability frames; three spills or running out of time means defeat. At high corruption, steering is visibly reversed.
- **Diaper Dash (#2):** use four separate timed tasks in sequence: WIPE (left), NEW DIAPER (right), FOLD (down), TAPE (up). Use D-pad or the corresponding stylus zones. Three mistakes or expired tasks fail the challenge; high corruption may reverse one explicitly signposted task.
- **Cabinets #3–#16:** implement their own timed, memory, movement, matching, resource-management, drawing or rhythm mechanics; each has a dedicated native DS screen and touch/button mapping. Full regression scenarios are in `tests/test_minigames.c`.

All sixteen cabinets reuse the established win/loss economy and **do not change the on-disk save format**. Dedicated automated tests are in `tests/test_minigames.c`, and the existing GitHub Actions workflow continues to produce the `.nds` build. Actual emulator/physical-device behavior still requires validation.

## DraStic / legacy-emulator boot compatibility (test build)

**User-reported bug:** A previous `.nds` build opens to **two white screens** in DraStic. A successful cross-compile is not proof that a specific emulator or flashcart can boot it.

The runtime now paints the lower-screen title interface immediately after video setup, **before optional file storage initialization and before the complex nursery render**. The title's upper-screen image uses a lightweight scene. Normal gameplay remains accessible in RAM-only mode by pressing **A** or **START**.

**Important storage behavior:** Press **X** or tap the mint **SAVE/LOAD** card on the title screen to explicitly check DLDI/FAT saves and restore an existing baby. If you already started in RAM-only mode, open **ROOM**, then press **X** or tap the new **SAVE** badge to save the *current* baby without loading over your unsaved work. No storage probe runs automatically on boot or when tapping START. If your emulator's storage probe hangs, choose START/A and remain in RAM-only mode; persistent saving depends on a compatible FAT/DLDI backend. The original two-slot save format is unchanged.

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

**Save accessibility:** at the title screen, press **X** or tap **SAVE/LOAD** to check or restore external storage. Once inside the nursery, you can open **ROOM** and use the visible **SAVE** badge (or X) to write current progress without returning to the title. These are both explicit actions, so the game can still boot directly into RAM-only play without blocking on emulator filesystem initialization. Do not assume progress persists unless saving has actually succeeded. Try the DS-only binary in DraStic using **Start Game**, not an emulator save state from the broken ROM.

For a diagnostic report, record whether (a) the purple/colorful title appears, (b) pressing START enters the nursery, and (c) pressing X at title freezes or reports no save device. A white screen *before* the title in both ROM variants suggests a deeper emulator/runtime incompatibility and needs an emulator-level reproduction; it is not proof of a 3D scene bug.

This is a compatibility **candidate**, not a runtime-verified fix.

## Alternate BlocksDS runtime for DraStic white-screen reports

A real-world DraStic test still showed two white screens with the original
devkitPro/libnds 2 build, even after switching the game to a simple 2D title
screen and patching the original-ROM header. The problem is **not resolved**
without further emulator testing.

An independent BlocksDS build is now available alongside the regular devkitPro
build. This changes both the ARM9 startup/runtime and the ARM7 firmware code,
not just the cartridge header. The standard `Makefile` and its CI are kept
unchanged; `Makefile.blocksds` builds the alternate ROM in the official
`skylyrac/blocksds:slim-latest` image.

A separate **diagnostic boot probe** (`diagnostics/main.c`) is also built
under BlocksDS. It initializes just two color-filled 2D screens, no gameplay,
no fatInitDefault(), no 3D and no baby model. This separates emulator/runtime
compatibility from the game's own graphics code.

The BlocksDS workflow produces original ROMs plus DS-only header copies with
recomputed CRC16, retaining identical ARM9/ARM7 executable bytes. On Android
DraStic, first try `my-baby-ai-blocksds-ds-only.nds` using **Start Game**;
do not load a saved emulator state from the old build. If it remains white,
try `blocksds-dual-screen-test-ds-only.nds`:

- **Probe shows colorful screens:** The basic BlocksDS runtime works in the emulator. Investigate the game's UI, RAM usage and startup code.
- **Probe stays white:** The problem is likely earlier (ROM loader, runtime, ARM7 or emulator compatibility); a 3D geometry change will not help.
- **Game title appears but START freezes:** Investigate 3D initialization and vertex/display-list traffic.
- **Title appears but X freezes:** Investigate FAT/DLDI initialization. The title's X button is optional; regular gameplay starts in RAM-only mode.

These are **diagnostic predictions**, not verified facts about the user's DraStic device. A green GitHub Actions build establishes only a compiled ROM, not an emulator/hardware runtime pass.

## Touchscreen and visual update — DraStic field-testing patch

The first DraStic screenshot confirms the **BlocksDS** ROM reaches both the native
bitmap bottom UI and perspective-3D nursery. This establishes a real emulator
boot on the tester's installation, not a tested FPS benchmark.

The new input code in `touch_input.c` recognizes a stylus gesture while
`KEY_TOUCH` is held, allowing a one-frame delayed valid position while
preventing repeat taps from one long press. The original DS `scanKeys()`
call remains before `touchRead()`, as BlocksDS requires.

**To diagnose touch if the lower screen does not respond:**
1. Restart the game with a fresh emulator session.
2. On the title screen, press **SELECT** on the DS controls for **STYLUS SIGNAL TEST**.
3. Touch several points on the **lower screen**. The crosshair, **PEN DOWN**,
   X/Y coordinates and tap counter should respond.
4. Use B or START to exit the diagnostic screen.
5. If the counter stays at zero, inspect DraStic's virtual-button touch
   regions and touchscreen mapping, not just the in-game button positions.
   A game cannot create stylus input that an emulator does not deliver.
6. If the counter moves but FEED/CHAT/PLAY/NAP still fail, preserve a
   screenshot of the diagnostic coordinates and the button being tapped.

Diaper Dash's actual WIPE/NEW/FOLD/TAPE tap zones now match its displayed
buttons rather than reusing the generic arcade prompt layout.

The 3D scene also has fewer floorboard/rail faces and a richer baby model:
colored irises and pupils, eyebrows, freckles, shirt stripes, and additional
crib mobile and shelf decorations. These improvements are hardware 3D
geometry and palette colors, **not AI-generated screenshots**. The extra
details must still pass vertex/polygon budgets and real-device framerate tests.

### Touch diagnostic flicker fix

The first stylus diagnostic repainted the entire 256x192 16bpp display every frame when touch was held. DraStic players reported noticeable flicker and no button response. The diagnostic now renders the background **once** and only redraws the KEY_TOUCH flag, X/Y, tap count and a small crosshair patch when their values change. This avoids heavy VRAM writes during active screen scanning and needs no second framebuffer.

At the **title screen press SELECT** and press/hold the **lower DS screen**. **KEY_TOUCH: DOWN** confirms the emulator and ARM7 runtime delivered the touch button signal. Changing X/Y means the touch position is arriving. A changing TAPS count means the game's touch gesture logic accepted it. If all remain unchanged, adjust DraStic touchscreen settings / layout or compare a known touch-based DS game; no game-side hitbox can process an event not delivered by the emulator. This instrumented build is still awaiting a user retest.

## Independent DraStic stylus signal test (November 2026)

The BlocksDS game has been confirmed to boot and render on the tester's
Android DraStic installation. The tester's physical screen touches still
appear to be ignored. The official BlocksDS documentation says `scanKeys()`
must be called before `touchRead()`; the game already does this in the
correct order. This makes a missing touch signal or emulator input mapping
more plausible than a mistaken gameplay button rectangle.

The CI now packages `blocksds-touch-probe-ds-only.nds`: a **standalone,
original-DS two-screen input test** linked against the same BlocksDS ARM7
and ARM9 runtime as the game, but without any game simulation, saving,
3D, custom fonts shared with the game, or touch-gesture classifier.

On this diagnostic ROM:
- The upper screen's **TOUCH** indicator is RED/OFF normally and GREEN/ON
  whenever `keysHeld() & KEY_TOUCH` reports an active stylus press.
- The adjacent **A KEY** indicator turns YELLOW when the emulator A
  button is held. This is a control case proving ordinary key input works.
- The lower screen shows a lilac touch canvas. Dragging a finger there
  should move the red crosshair, while upper X/Y values and TAPS update.
- **If A lights up but TOUCH never goes green:** inspect DraStic's quick-menu
  *Enable/Disable touch input* toggle, display/screen mapping, virtual-button
  overlap, and any Android fullscreen settings that alter touch coordinates.
  Confirm the stylus also works in a different DS game known to use touch.
- **If TOUCH lights up here but not in My Baby A.I.!:** file a game-specific
  issue with screenshots from both ROMs and the DraStic version.
- **If the control case (A) doesn't light:** the emulator/controller layout or
  ROM/runtime is failing more broadly; don't diagnose game-specific hitboxes.

No emulator runtime is considered fixed solely by this CI build. This
independent probe reduces ambiguity and does not erase game save data.

## Controller-driven virtual stylus (DraStic / no touchscreen required)

The user confirmed both the BlocksDS Maxmod and minimal ARM7 variants fail to
receive touch events in their DraStic configuration, even after trying all
emulator touch-input settings. Rather than asking the player to repeat those
steps, a **controller-driven software cursor is now available on every screen**:

- Hold **L** and press **SELECT** to toggle *VIRTUAL PEN* on/off.
- Use **D-PAD** to move the small on-screen crosshair to a button or menu item.
- Press **A** to tap it. This calls the **same interactive hitboxes** as actual
  stylus taps, so care, arcade selection, cabinet purchase, endings, and
  in-game menus can all be operated without a working touch digitizer.
- The existing **B, X, Y and START** shortcuts remain available.
- In Bottle Balance, hold A with the virtual cursor over either steering area
  to apply continuous left/right input.

This is a functional workaround, *not* a claim that DraStic's physical touch
path is fixed. The actual stylus path remains available when delivered by an
emulator or physical DS. Moving the cursor updates an 11x11 patch (242-byte
color backup), avoiding a 256x192 VRAM repaint or secondary framebuffer.
It does not affect save files or the core game's state.

## Why the other DS ports matter

The persistent DraStic touchscreen failure was compared against the actual implementations in **Mandex** and **Bore: GodGuy**, both of which use custom, direct-register ARM7 input and pass it through shared memory. The current My Baby A.I. BlocksDS ROM uses a filtered ARM7 touchscreen FIFO and has shown no native touch events on the user's DraStic build. See [cross-repository touchscreen audit](docs/DS_PORT_TOUCH_AUDIT.md).

A new **standalone native input architecture test** is built through GitHub Actions as `bare-touch-arm7.nds`. It uses Mandex-style ARM7 register reads plus SPI and a classic ARM9/ARM7 boot image, but **no BlocksDS** or My Baby A.I. game code. Its upper screen turns green on raw pen detection and yellow for the A-button control; the lower screen becomes pink if the pen is present but ADC sampling fails. If it responds in DraStic, the legacy ARM7 path is a promising route to a real native touch backend. This is a separate diagnostic, not a silently substituted engine in the playable game.

## Verified touchscreen gameplay in a second DS emulator (DeSmuME)

The **BlocksDS DS-only ROM**, built with the same source and standard ARM7 input
services as the DraStic binary, now has a **real automated emulator test**. The
`.github/workflows/blocksds-build.yml` `desmume-touch` job boots the
compiled ROMs with **DeSmuME 0.9.12** via `py-desmume`. The test injects a real
DeSmuME stylus event (not a game callback), advances ARM7/ARM9 emulated frames,
and verifies the rendered screenshots.

For the standalone probe, the test asserts: TOUCH OFF/red → TOUCH ON/green;
native A button → yellow; moving the stylus changes X/Y; release → red.

For **My Baby A.I. itself**, it asserts: SELECT enters the in-game diagnostic;
`KEY_TOUCH` changes to DOWN, coordinates and the tap counter update; a
touchscreen press on START GAME **enters the nursery**; and touching **FEED**
changes the care interface. The CI job fails on missing visual changes and
uploads actual emulator screenshot PNGs plus JSON evidence even on failure.

**Result:** native stylus handling works in DeSmuME on this exact game build.
The user's DraStic setup still does not deliver native stylus presses even
though virtual gamepad buttons work. This isolates a **likely emulator-specific
compatibility issue**, not a universally broken game touchscreen. It does
**not** establish that every DraStic version, melonDS build or original DS
hardware will behave identically. The **L+SELECT / D-pad / A virtual stylus**
remains available for DraStic users.

## Naturalistic graphics update

The original, intentionally awkward 3D baby art is being replaced by a
softer and more recognizable infant design rendered by the **actual
Nintendo DS 3D hardware**: a smoother face, reduced eye protrusion,
subtler expression, warm skin and hair, fabric-layer clothing and
small controlled animations. A closer camera and subdued real-world
nursery colors improve legibility at 256×192. The room uses warm wood,
cream walls, natural textiles and staggered floorboard joints.

This is not a pre-rendered image and requires no imported textures or
extra framebuffers. Geometry is tested against a stricter hardware
headroom budget, and the automatic DeSmuME workflow still verifies boot
and native stylus-driven START/FEED controls. Real console FPS remains
to be measured; the new visual design must not be described as
photorealistic given the DS's polygon limits.

## Character art direction: unsettlingly cheerful DS-baby nostalgia

The baby has been redesigned after the user-supplied 3D baby-game
reference: large almost-real facial proportions, small inset dark eyes,
asymmetrically messy dark hair with a pink clip, pink short-sleeved top,
dark baggy trousers, reaching hands and an oddly fixed smile. The
orange teddy now sits between the baby's legs, on a lime-green rug with
large printed flowers, in a nursery with pink bedding and tulip decals.
This deliberately preserves the awkward/uncanny visual character of
a low-budget Nintendo DS game, rather than turning it into a polished
modern cartoon.

These are **real, original 3D renderer changes** in `source/world.c`,
not concept imagery. Original DS polygon/vertex budgets and the
DeSmuME native stylus-to-START/FEED CI checks still apply.
