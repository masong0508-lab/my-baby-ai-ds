# My Baby A.I.! (Nintendo DS homebrew)

**The Last Nursery:** after the Continuity Bureau outlaws infancy, you shelter an unregistered baby found outside a shuttered clinic. A salvaged computer named **WARDEN** watches the nursery. The baby learns your phrases, becomes increasingly uncanny, and eventually attacks your caretaker bracer. Care, conversation, defensive slapstick, 16 arcade games and the hidden alien-shell reveal share one continuing save.

## Plain-homebrew ROM for the "encrypted ROM" warning (2026-11-22)

Some older DS emulators interpret the toolchain's 0x4000 ARM9 cartridge
offset and blank secure-area prefix as an encrypted retail ROM. This is
original homebrew, not encrypted commercial content. The normal builds
remain available, but packaging now also creates
**my-baby-ai-blocksds-plain.nds**, recommended first for DraStic users
who encounter that warning. The alternative devkitPro variant is
**my-baby-ai-plain.nds**.

The plain build relocates the exact compiled ARM9 bytes to the 0x1000
homebrew ROM region, sets the source offset/header size accordingly,
clears the nonexistent secure-area checksum and recalculates the header
checksum. ARM7, filesystem, graphics, gameplay, language model and saves
remain unchanged. Use a fresh emulator session, not a save state from an
older ROM. See docs/QA_RELEASE.md for the validation boundaries.

The automated DeSmuME native test boots this layout and exercises the
game. This does not prove that a particular DraStic installation accepts
it. If its warning persists, note the emulator version and when it occurs.

## DraStic encrypted-ROM alert: legacy metadata correction (2026-11-22)

A real Android DraStic screenshot showed that the earlier plain-ROM patch
**still fails** with the “This version of the ROM is encrypted” dialog.
It also displayed scrambled Asian-looking characters and a dark icon.
We inspected the *actual distributed ROM* and found the BlocksDS header
has banner offset 0, so it has no valid title/icon; the first repair had
also set ROM header size to an unusual 0x1000. The compiled ARM9 code is
not encrypted. The fix is original-DS **packaging**, not installing
Nintendo BIOS or decrypting a commercial ROM.

Use **my-baby-ai-blocksds-drastic-classic.nds** first. It has an ARM9
ROM source at 0x200, the canonical old-homebrew 0x200-byte header, a
proper 32x32 colored baby icon, six UTF-16 title entries, and two valid
CRCs. **my-baby-ai-blocksds-drastic-plain.nds** retains ARM9 at 0x1000
but fixes that header size and banner too. A standard 0x4000 ARM9
variant, and the existing older ROMs, are retained as fallbacks.
The binaries all preserve exact original ARM9 and ARM7 executable bytes,
filesystem and gameplay; no key material or BIOS is required to package
them. Start a fresh emulator session and select the new filename, not
a saved state from an older ROM.

Two layouts go through native DeSmuME gameplay CI as an independent
regression guard. A green DeSmuME test **does not confirm the alert is
gone in DraStic**; report the exact version and outcome of the fresh
v3 ROM if the warning persists. See docs/QA_RELEASE.md for details.

## First playable prototype

The ARM9 program renders a furnished perspective-3D nursery and a modeled, animated, intentionally ugly-cute baby, with crib rails, dresser drawers, window, play mat, toys and teddy bear. The lower screen uses a colored bitmap touchscreen interface. Buttons, arcade cabinets, care meters, prompts and ending cards are drawn directly into DS sub-engine VRAM. There are **16 distinct arcade minigames** with individual win conditions and touch/button controls, including the full eight-cabinet unlock progression. The baby has offline integer neural inference, an adaptive conversation layer, and authored responses grounded in its actual needs and recent care. The original **48 mood-responsive lines plus 12 recent-event CHAT replies** remain available. Nursery care and prop interactions include short single-channel PSG melodies and a Room Tour mute control.

**Cartridge identity:** the game ROMs are packaged with the twelve-byte title `MY BABY A.I.` and game code `MBAI`; diagnostic boot and touch-probe ROMs keep their own identifiers. The header checksum is validated without changing ARM9/ARM7 executables. The game remains native DS software and has no DSi-exclusive memory requirement.

### Microphone lessons and offline spoken prompts

In WARDEN's **CPU** terminal, type a short word or phrase (up to 24
characters), then tap the new **MIC** badge above the keyboard. In the MIC
LAB press **A/TRAIN**, speak after the quiet calibration, then repeat TRAIN
to store a second example. Press **X/LISTEN** to attempt recognition.
If a label matches confidently, press **Y/USE** to send that text to the
currently selected BABY or WARDEN terminal. **B/BACK** cancels.
Tap **MIC in CARE** (or hold L and press X) to speak to BABY directly;
the MIC LAB shows the recipient, recording phase, and live level.
In the terminal, the four suggestion buttons fill the message box
without sending. L+R opens MIC by controller. While capturing, A or X
cancels instead of restarting. Captures accept up to 56 feature frames
(1.75 seconds including the silence release); longer input is rejected.
MIC LAB also has **PREV/NEXT** (L/R or D-pad left/right) for browsing
previously taught words, showing their EX 1/2 or 2/2 training status.
**DEL or SELECT twice** forgets only the selected phrase. The learner
does not discard an old lesson when all 24 slots are full: free a slot
explicitly before teaching a new one.

The DS microphone uses a finite ARM7 8,192 Hz capture session, with a
1 KiB aligned double buffer and cache-safe PCM16 processing on ARM9.
Voice detection first calibrates ambient sound, tracks slow background
noise changes, and requires a sustained two-block speech onset. The
displayed level is relative to that ambient baseline, rather than
treating a constant fan or microphone pop as a spoken command.
The user-trainable matcher supports **24 distinct labeled words or short
phrases**, two compact feature examples each, under 3.5 KiB of profile
RAM; raw audio is not stored. This is a speaker-dependent template
classifier with bounded time-warp alignment for different syllable
lengths, **not full speech-to-text, arbitrary conversation recognition,
or an always-listening neural language model**. It still rejects
ambiguous labels or unfamiliar sounds. Each recognized command
requires an explicit USE confirmation; no automatic care action is taken.

Optional lessons persist in a separate checksummed Game-bound voice
sidecar when save storage is available. In RAM-only mode the lessons
last for the current session. Unsupported emulator microphones show
an error and preserve the full keyboard and button fallbacks.
See [microphone architecture, controls and test limits](docs/VOICE_MIC_MILESTONE.md).

### WARDEN, typed conversation, and the hidden shelter

Tap **CPU** or press **UP** in CARE to open the terminal. **TARGET / Y** switches between WARDEN and BABY. Type on the touchscreen or use the D-pad and A; **SEND / X / START** sends. **123 / SELECT** switches to the number/symbol keyboard, L deletes, R inserts a space, and B returns.

- WARDEN answers **STATUS, NEEDS, MEMORY, WORLD, ATTACKS, BOND / TRUST, HELP**, and bounded calculations such as **12+7** from actual state or its original shelter archive.
- **GOALS** recommends urgent care first or the next locked arcade cabinet, reporting exact remaining wins, dollars and favors. **ARCADE** shows live cabinet progress; **BUDGET** shows currency; **ENDINGS** tracks collected finales; **TOMORROW** previews END DAY using a temporary copy of the current game. **MORE** gives related details. These questions never buy items, advance days or alter saves. BOND derives a care-pattern report from observed actions; it does not invent a numeric trust meter.
- BABY can learn eight short phrases: select BABY, type up to 24 characters and tap **TEACH**. Repeat teaching to reinforce a phrase. **SAY** recalls it; a familiar phrase can also appear after actual healthy PLAY. A full lesson library preserves all eight phrases. **PHRASES 1** and **PHRASES 2** list four slots at a time, **LESSON 1** shows practice confidence, and **FORGET 1** previews removal before **FORGET 1 YES** confirms it (use the desired slot number).
- Typed greeting, dream and personality replies use an original **412-token, 28,700-parameter neural word model**, generating one token at a time. Its complete immutable model/vocabulary/masks occupy **57,487 bytes**. The model is pretrained; player learning lives in the separate style and phrase profiles. Topic recognition is deliberately limited, and this is not a general-purpose assistant.
- Real care, completed activities and Toybox actions advance **FOUND → WATCHING → HOSTILE → REVEALED**. Questions and idle menus do not advance exposure. WARDEN records actual attacks and counters.
- Hostile attacks have a visible wind-up. Press **RIGHT** or tap **GUARD**; a successful Toybox slap/kick also counters a wind-up. A landed hit reduces a separate bracer integrity value. Type **REPAIR** to WARDEN to restore it. Reading, typing, arcades, films and a tumbling shell pause the attack clock.

The optional **256-byte observation profile** saves taught phrases, observed history and bracer state in a checksummed companion tied to the exact v1 Game. The loader accepts older 152-byte profiles, retaining the original four slots and initializing four new slots. Missing or damaged companions do not prevent normal progress loading. Saving remains explicit; RAM-only play does not persist after reboot.

The actual nursery renderer now has shallower rounded eyes, softer cheeks/bib, stronger clothing and room shading, grounded furniture/boots, a physical WARDEN wall terminal, and connected wind-up/lunge acting. Speaking mouths stay within their intended aperture instead of stretching into a dark line across the jaw. CARE/TALK cards have clearer hierarchy and selection. Sampled normal-scene submission maxima are **4,402 vertices / 1,631 polygons**, down from 4,702 / 1,808. These are geometry counts, not FPS measurements.

**FPS telemetry:** Hold **R** and press **SELECT** to show or hide a compact FPS strip. The game samples completed frame submissions against the DS bus clock, with separate current, weighted average, lowest and highest one-second samples for each view and each of the sixteen arcade cabinets. Measurements continue while the overlay is hidden; incomplete windows are discarded on view changes. The values measure submission throughput in the console/emulator's time domain, not physical LCD presentation or emulator host wall-clock speed. Native CI records toybox, nursery, room and arcade-menu FPS in `fps-report.json` once this build passes its tests.

See [story and progression](docs/THE_LAST_NURSERY.md), [observer rules and persistence](docs/SHELTER_OBSERVER.md), [word-model architecture and limits](docs/NARRATIVE_LANGUAGE_MODEL.md), [reference analysis](docs/SNES_REFERENCE_ANALYSIS.md), and [native observer tests](docs/NATIVE_OBSERVER_TESTS.md).

### Talk, follow up, and teach a conversational style

Touch **TOUCH TO TALK** in the nursery speech panel, or press **LEFT**, to open TALK. Six choices ask how the baby feels, what it needs, what it remembers, a follow-up, positive feedback, or a new subject. Touch a question directly, or select it with the D-pad and press A; B returns to care. Asking questions does not secretly spend energy, raise corruption, advance days, or consume arcade randomness.

The baby keeps track of its current subject and actual recent care, including whether a bottle or nap helped, repeated care, quiet crib rest and completed arcade results. **270 authored responses cover 30 topics in three styles, with three alternatives per style and two dedicated follow-up choices.** Urgent needs override unrelated topics. **I LIKE THAT** and **NEW SUBJECT** update a small integer neural style adapter: warm, playful, or corporate wording can become more likely from your feedback. The TALK panel uses authored response text and on-device style learning. The separate typed terminal adds the bounded neural word generator described above; neither system can invent factual observations or train urgent needs away.

Learned preferences fit a **40-byte profile**. When saving is enabled, checksummed companions bind that profile to the exact saved baby while leaving the existing version-1 progress files unchanged. Missing or damaged learning files start neutral preferences and do not block restoring progress. Short-lived event memories still reset on load or a new day. See [neural conversation](docs/NEURAL_CONVERSATION.md), [learning save compatibility](docs/LEARNING_SAVE_COMPATIBILITY.md), and [native conversation tests](docs/NATIVE_CONVERSATION_TESTS.md).

### Nursery lullaby

In ROOM, tap **LULLABY** or press SELECT to sing to the baby. A useful lullaby raises mood by up to 6 and lowers chaos by up to 8, costing 3 energy and adding 2 hunger. It plays a separate gentle melody and a connected regroup animation, and records one social care interaction. The baby refuses if hungry, exhausted, or already completely calm and happy. No extra currency, save fields or automatic actions are involved.

### Peekaboo

In ROOM, tap **PEEKABOO** or press R. Wait until the baby opens its eyes, then tap BOO or press A. Three successful reveals win; two early or missed reveals end the activity. A win raises mood by up to 12 and adds 4 chaos, costing 8 energy and adding 6 hunger. It records one play interaction, plays a result animation and saves if storage is enabled. No arcade currency or win/loss counts change. B cancels without a care cost. The baby needs sufficient energy and reasonable hunger/chaos before starting.

An exceptionally rare horror Easter egg may interrupt a successful BOO: the existing alien reveal lunges immediately, followed by a non-strobing fictional session-corruption screen and a halted game session. Restart the ROM or console to resume play. The nominal trigger is one of 2^20 uniformly distributed draw values, with the runtime draw mixed from input timing and the existing seed. No gameplay RNG is consumed, no save is written, and no cartridge, console, firmware or stored data is damaged. The event is visual and session-local only.

### Care flow and recent memories

Rapid care taps carry the baby's current arm, head, eyelid and bounce pose into the next gesture over 12 game ticks. This includes tiredness changes and the extra recovery from a quiet crib nap. Each gesture still settles back to idle after its original 56-tick cycle.

When the baby is comfortable, CHAT can mention a recent bottle, playtime, nap, quiet crib rest or completed arcade win/loss. Four distinct events fit in a 20-byte session memory; needs take priority, repeated replies are avoided, and events expire with further interactions or a new day. Loading or resetting a baby starts a fresh memory. The existing save format remains unchanged. See [care-motion details](docs/CARE_MOTION_CONTINUITY.md) and [recent-interaction memory](docs/RECENT_INTERACTION_MEMORY.md).

### Arcade training, pause and timing

All sixteen cabinets have illustrated **GUIDE** pages (tap Y GUIDE or press Y in the arcade). The guide explains each actual objective, touch gestures and button controls. Select **X PRACTICE** to try even a locked paid cabinet. Practice uses an isolated, normal-difficulty copy of game state: no live rewards, wins/losses, corruption, RNG, learning events, unlocks or save writes. Real runs retain the original purchase requirements and reward economy. Tap the mode strip or press **Y** in the guide to cycle Standard, Relaxed (+50% time on every stage and slower moving blocks), or Expert (high-corruption rules with an additional 20% deadline reduction, quicker block movement, larger sums/subtraction in Cereal Cipher, and near-match accounting discrepancies in Baby's First Audit). All three are isolated practice modes and never change the live game. Normal runs continue to use the original difficulty and live corruption.

Press **START** or tap the small **II** badge in a cabinet to pause and read its guide. A/B/START or RESUME continues; X END RUN exits. Unfinished drag/trace gestures must be started again; all other puzzle state and timers are held. All cabinets now have a green/amber/red live time track between progress and mistake indicators. The track uses the actual remaining fraction of the current stage's timer, including each new question and Beatbox WATCH/REPEAT transition, and updates only a small bitmap region.

Baby Beatbox uses four original distinct PSG pad sounds for its visual demonstration and correct repeat inputs. Existing result cues and the global mute still take priority.

### Arcade reactions and clear feedback

Successful steps, mistakes, wins and losses now have distinct short PSG cues and brief text feedback in the existing footer. Ignored taps and ordinary steering stay silent. Completed games trigger a finite baby cheer or a gentle regrouping gesture; both can blend into the next care action. Normal footer controls return after the message expires, while a failed save takes priority and remains visible until saving succeeds. The Bottle Balance green band now matches its actual scoring window. See [arcade feedback](docs/ARCADE_FEEDBACK.md) and [baby result reactions](docs/ARCADE_BABY_REACTIONS.md).

### Controls

* A feed, B chat, X play, Y nap; the four large touchscreen buttons provide the same actions.
* UP or **CPU** opens the typed WARDEN/BABY terminal; RIGHT or **GUARD** counters an actual attack wind-up.
* LEFT or the speech-panel **TOUCH TO TALK** opens conversation; D-pad selects a prompt, A asks, and B returns. Touching any prompt asks it immediately.
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

**Verification status (November 2026):** Standard devkitARM and independent BlocksDS ROMs have passed CI. Native DeSmuME suites cover stylus input, core views, care/Room Tour, the alien-egg film, completion of all eight starter cabinets, and an earned Cereal Cipher unlock and victory. They include selected losses, wrong-answer and timeout retries, actual steering/timing/dragging, result feedback and reward/purchase totals. **Physical original DS/DraStic operation, sustained FPS/RAM telemetry, long soak tests and real flashcart save persistence remain unverified.** All sixteen minigames have host mechanic coverage; native completion covers nine cabinets, with the other seven paid games still outstanding. See [starter playthroughs](docs/NATIVE_STARTER_ARCADE_TESTS.md), [native arcade tests](docs/NATIVE_ARCADE_TESTS.md), [development status](docs/DEVELOPMENT_STATUS.md) and [release QA](docs/QA_RELEASE.md).

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

**Current development priority:** deepen the Last Nursery premise, WARDEN's useful observations, the baby's evolving conversation and meaningful interactions. Preserve exact-head build/native checks while extending that system. Original-DS profiling, measured RAM headroom and save testing across power cycles remain release requirements; automated DeSmuME screenshots do not replace those device checks.

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
