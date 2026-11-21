# Development status — My Baby A.I.!

This is a source-based inventory of `main` as audited after the first successful GitHub Actions ROM build (2026-11-20 UTC). **Implemented** means code exists, not that a feature has passed emulator and physical-hardware validation. See [ROADMAP.md](ROADMAP.md) for completion work, [FEATURE_IDEAS.md](FEATURE_IDEAS.md) for design pitches, and [QA_RELEASE.md](QA_RELEASE.md) for the test gates.

## What exists in the repository

| Area | Current implementation | Limitations / missing verification |
| --- | --- | --- |
| Nintendo DS ROM | devkitPro `nds-dev` + `libnds` ARM9 `Makefile`, GitHub Actions `nds-build.yml`; PR #1 produced a real `.nds` ROM artifact | A compiled header is **not** proof of a playable, stable emulator/flashcart session. Re-test latest `main` on every release |
| Dual-screen rendering | `source/world.c`: perspective-3D nursery with crib, dresser, window, floorboards, toys, teddy and a procedural 3D baby; `source/ui.c`: sub-screen 16-bit bitmap interface | Most art is procedural geometry with fixed camera; limited pose variation; no authored texture/rig pipeline or multiple explorable rooms |
| Care loop | `source/game.c`: feed, chat, play and nap affect needs, mood, chaos and corruption | No rich touch-manipulation activities, time-of-day routine, meaningful room interaction, or evolving developmental skills |
| Baby dialogue | `source/dialogue.c`: fixed-weight 6-input → 8-hidden → 6-output integer classifier; six response sets of four lines (24 authored lines) | Not a generative LLM or trained neural model. No durable conversational/event memory, word recognition, typed chat, or long-running relationship |
| Arcade | 16 named cabinets; eight unlocked initially; distinct gameplay for Bottle Balance, Diaper Dash, Tax Return Teether, Crib Kart, Baby Beatbox, Sock Market, Naptime Negotiator and Rattle Rescue; the other eight use directional/timing challenge | **8 of 16 have distinct mechanics** on this branch. Eight cabinets still need unique gameplay, controls and tests; the new modes need ROM and emulator validation |
| Economy | Diaper Dollars, Favors and unlock costs in `game_unlock`; wins award currency | Limited sinks/long-term rewards; no purchasable décor, apparel, talent upgrades, sticker collection, or balanced price curve |
| Glitches | Five corruption tiers, occasional visual disruption, reverse directions, shorter arcade response windows | Effects are narrow; no per-mode corruption language, glitch frequency controls, or accessibility balancing beyond the visual effect toggle |
| Endings | Four good, four bad, ending album, continuing play; separately confirmed optional reset | Basic text/bitmap endings rather than bespoke cinematics; needs stress testing around reset and recovery |
| Saves | DLDI/libfat storage, two checksummed alternating save slots, validation and scoped erase | FAT write interruption, filesystem failures, version migration and real-flashcart persistence need testing |
| Audio | No active sound engine, SFX or music code identified in current `source/` modules | Full audio pipeline missing |
| Tests | Host-side logic, dialogue, UI and geometry budgets; GitHub Actions cross-compiles the ROM; DeSmuME 0.9.12 emulator CI now injects real touchscreen presses, verifies tap/move/release, START GAME and FEED, and captures screenshots | Still no verified original-DS hardware run, FPS/RAM telemetry, 30-minute soak, save-persistence proof, or working native touch on the reported DraStic installation |
| Distribution | CI artifact `my-baby-ai-nds` attached to successful workflow run | No tagged release, release notes, license/attribution review or player-facing manual yet |

## Current game loop (implemented)

1. Open the title screen and enter the nursery.
2. Use A/B/X/Y or the touchscreen to feed/chat/play/nap; state updates immediately.
3. Enter the arcade, select one of 16 named cabinets, and play one of eight distinct minigames (Bottle Balance, Diaper Dash, Tax Return Teether, Crib Kart, Baby Beatbox, Sock Market, Naptime Negotiator and Rattle Rescue), or one of eight shared directional cabinets (or unlock cabinets when eligible).
4. Progress days, gain currency/Favors, influence corruption and collect endings.
5. Continue playing after a good or bad ending. Optionally access the secret, explicitly confirmed game-save reset after sufficient **bad** endings.

## Gaps that matter most

**Critical before claiming a verified playable build:** boot and frame-by-frame rendering in melonDS, input testing on both screens, 30-minute play session, save/reload, and original-hardware flashcart testing. The GitHub Actions compiler cannot certify those.

**Critical before claiming a complete game:** finish the other eight distinct minigames; authored baby animation and environment interactions; additional activities and progression; sounds/music; stronger personality memory; accessible settings; an achievable economy; and end-to-end game balance.

**Important documentation correction:** The neural classifier is already implemented, but it is a compact hand-tuned integer intent classifier, not a modern language model. The ROM has cross-compiled successfully through GitHub Actions, but its runtime behavior has not been independently confirmed here.

## Non-goals for the baseline DS release

- Requiring network access, cloud inference, or DSi-exclusive memory.
- Large framebuffer allocations or unbounded per-frame heap churn.
- Copying commercial assets, brand marks, code or audio from the games being parodied.
- Unexpected erasure of save files. The optional reset finale must remain explicit, scoped, and cancelable.

**Source of truth:** `source/`, `include/`, `tests/`, `Makefile`, and `.github/workflows/nds-build.yml`. Update this matrix when features land; never change an item's status on the strength of concept art or a plan alone.

## DeSmuME touchscreen verification (2026-11-20)

The BlocksDS native-touch path **has now been verified in a real emulator**, not just compiled: automated DeSmuME tests run both the independent input probe and the full game, inject stylus events, check screenshots for KEY_TOUCH and X/Y changes, and activate START GAME and FEED via the touchscreen. See [QA_RELEASE.md](QA_RELEASE.md). The corresponding DraStic user still reports **no native stylus events** despite working gamepad buttons; the root cause specific to DraStic remains unverified. This does not supersede original-hardware testing or frame-rate profiling.

## Illustrated arcade pass (feature branch, 2026-11-20)

This change replaces the old first-two-cabinet placeholder panels with original ROM-embedded indexed pixel art (baby face, bottle, rattle, diaper), scene dressing, touch feedback and larger progress indicators. It does **not** yet provide imported production sprite sheets or music, and it does not claim all 16 cabinets are finished.

- **Bottle Balance:** a baby/bottle scene, live stable-progress bar, clearer control regions and moving bottle indicator; physics preserved.
- **Diaper Dash:** four actual stylus interactions (rub to clean, drag fresh diaper from tray, swipe upward to fold, tap two tabs), with D-pad assist for broken native-touch environments. Each stage has visual instructions and a separate gesture state.
- **Rattle Rescue:** newly distinct falling-rattle game with three lanes, timed catch window, six catches to win, three misses to lose, touchscreen and D-pad controls.
- **Tax Return Teether:** drag a paper receipt from the baby executive's desk into the correct KEEP or SHRED tray; six correct sorts win, three mistakes lose; D-pad alternative.
- **Baby Beatbox:** watch a three-note light-up drum sequence then reproduce it on four touchscreen pads; three patterns win, with a three-strike limit and D-pad alternative. Its preview-duration clock is now initialized so all three notes illuminate in sequence.
- **Crib Kart:** survive six incoming traffic cones by switching among three lanes with either the stylus or D-pad; a collision costs a strike and the third collision ends the race.
- **Sock Market:** match the stripe on a sample sock to one of three randomly shuffled matching stalls; six pairs win, three mismatches lose. Includes graphic sock art and touchscreen stalls.
- **Naptime Negotiator:** calm the baby with four well-timed taps in a visibly marked 33%-width lullaby window; repeated taps in a single window cannot be exploited. Global session timer, three-strike rule and button/touch options.
- **Verification:** host tests now cover all eight distinct cabinet mechanics, touch hitboxes, timeouts and corruption-based difficulty. ROM and emulator checks are required before merge.
- **CI regressions repaired:** Rattle Rescue now has an active UI draw path and resets properly on timeout; emulator Diaper Dash script now performs real continuous gestures rather than obsolete one-tap tests.
- **Testing:** new host tests cover gesture sequence, premature catches, lane matching, timeouts, high-glitch speed and lane hitboxes. The ROM build and emulator still need independent verification for this new branch; source changes alone are not confirmed runtime screenshots.

The game save structure and on-disk save format remain unchanged. All new transient fields live in `Arcade` and are discarded when a cabinet ends.
