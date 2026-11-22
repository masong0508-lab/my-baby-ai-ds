# Roadmap: from compiled DS prototype to complete game

**Platform:** original Nintendo DS / DS Lite, with DSi supported only through the standard DS-compatible build. **Presentation:** intentionally awkward, colorful 2000s baby-game aesthetics enhanced by original environments, better character acting, a fluid touchscreen UI and playful ordinary corruptions, plus the optional Toybox/alien-egg horror cinematic.

This document separates implemented checkpoints from remaining acceptance work. Updated against `main` at `47021429eb958bd56b62872866f22d3bae2008ae` after [PR #63](https://github.com/masong0508-lab/my-baby-ai-ds/pull/63): all sixteen cabinets, 48 dialogue lines, interactive nursery props, PSG cues, submitted-geometry diagnostics and touch-accessible saves exist. These do not establish a complete title or physical-DS release readiness.

## Priority levels

- **P0 — correctness and release blockers:** crashes, unverified runtime, save integrity, unusable input, memory/geometry budget violations, broken build automation.
- **P1 — core product:** validate and polish the eight implemented starter minigames, expressive baby interactions, audio feedback, coherent progression and readable menus.
- **P2 — full game:** balance and deepen the eight implemented unlockable minigames; add environments/cosmetics/quests, rich glitch effects, more ending presentations and personality memory.
- **P3 — optional polish:** advanced unlock modes, additional characters, local multiplayer/DS wireless experiments, extra language packs.

## M0: establish trustworthy runtime evidence (P0)

**Implemented checkpoint:** CI builds devkitPro and BlocksDS ROMs and DS-only variants, runs the strict host suite with ASan/UBSan, and performs a native DeSmuME stylus/game walkthrough with captured screens. The SELECT-on-title diagnostic displays live **software-submitted** vertex/polygon counts for its rendered scene; it does not measure hardware throughput, FPS, frame pacing or RAM. Title SAVE/LOAD and Room SAVE are explicit stylus actions; host regressions verify current-baby save ordering, counter rollover and two-slot recovery. See [DS_RENDER_TELEMETRY.md](DS_RENDER_TELEMETRY.md) and [TITLE_TOUCH_SAVE_MILESTONE.md](TITLE_TOUCH_SAVE_MILESTONE.md).

**Remaining work:** run the exact latest artifact in melonDS and on original DS/DS Lite; cover all current views, normal/reversed controls, good/bad endings and deliberate reset with disposable saves. Record a 30-minute session, measured frame times and RAM watermark; verify FAT/DLDI save, reboot, restoration and interrupted-write recovery. Native DeSmuME checks do not resolve the reported DraStic stylus issue or certify physical flashcart storage.

**Done when:** emulator boot and 30-minute smoke test are reproducible; no black-screen regression; ROM is produced by CI; results recorded in [QA_RELEASE.md](QA_RELEASE.md). Failure means fix this before new content.

## M1: strengthen the real nursery and baby (P1)

**Implemented checkpoint:** the procedural baby has connected care gestures, gaze/blink and need-dependent fatigue/restlessness; Room Tour offers window, lamp, teddy and crib touch interactions, quiet-night rest, an animated mobile and smoothly interpolated prop camera views. Native screenshots and geometry tests cover these paths. See [BABY_BEHAVIOR_MILESTONE.md](BABY_BEHAVIOR_MILESTONE.md) and [ROOM_TOUR.md](ROOM_TOUR.md).

**Remaining work:** refine anatomy, pose transitions and scene composition from native-resolution captures; extend authored acting and useful prop interactions where it improves play. An authored mesh/texture pipeline, hairstyle options, more rooms and a measured baseline-hardware performance target remain unfinished.

**Performance approach:** share static meshes, cap joint counts, precompute transforms where practical, avoid per-frame allocations, prefer palette/UV swaps to full texture copies. Do not expand graphics until the current renderer is profiled.

**Done when:** the baby is visibly recognizable at native 256×192, at least six animations can be triggered in-game, four nursery props offer actual touch interaction, and busy scenes meet the agreed performance target on baseline hardware.

## M2: make the first eight arcades genuinely different (P1)

**Implementation checkpoint:** all eight starter cabinets have individual mechanics, artwork and input paths on `main`, with host gameplay/hitbox coverage. The native DeSmuME walkthrough checks cabinet navigation, Bottle Balance entry and a complete Diaper Dash session; the other cabinets still need individual native playthroughs. Original-hardware testing and extended human balance/playability testing remain outstanding.

**Remaining work:** improve instruction clarity, interaction feedback and fairness under normal and high-corruption timing. Verify each win/loss path, rewards and touch/button alternatives in hands-on sessions; expand mechanics only after the existing loop is readable and responsive.

| Starter cabinet | Current implemented interaction |
| --- | --- |
| 01 Bottle Balance | Steer a wobbling bottle and hold it steady |
| 02 Diaper Dash | Wipe, drag a fresh diaper, fold and fasten two tabs |
| 03 Tax Return Teether | Drag receipts into KEEP or SHRED trays |
| 04 Crib Kart | Switch between three lanes to avoid incoming cones |
| 05 Baby Beatbox | Watch three notes and repeat them on four pads |
| 06 Sock Market | Match the sample sock to a shuffled stall |
| 07 Naptime Negotiator | Time taps inside the lullaby's marked calm window |
| 08 Rattle Rescue | Catch falling rattles in the correct lane and timing window |

**Done when:** all eight games use different core mechanics, are finishable by touch/buttons, award currency consistently, and are separately checked by tests and a hands-on playthrough.

## M3: baby's personality and meaningful care (P1–P2)

**Implemented checkpoint:** the fixed-weight 6-input/8-hidden/6-output integer classifier selects from **48 authored lines** in six eight-line pools. Bounded need-based score adjustments make all six intents reachable, checked by a 120-case CHAT census and care-action regressions. Daily need changes, contextual care guidance and quiet-night recovery are implemented. See [DIALOGUE_REACTIVITY_MILESTONE.md](DIALOGUE_REACTIVITY_MILESTONE.md) and [DAY_CYCLE_MILESTONE.md](DAY_CYCLE_MILESTONE.md).

**Remaining work:** add a small recent-event memory, relationship/mood flags, topic tags and event-dependent authored responses. Expand beyond the existing 48 lines with context and repetition control. The current system has no durable conversation memory and is not a contemporary text-generation model.

Make feeding/playing/sleeping more than one-button stats: short care interactions, discoverable preferences, delayed consequences, surprise jokes and perks from building trust.

**Done when:** the baby refers to at least three recent actions in context, behaves differently across several sustained moods, repeats fewer lines, and the response system never causes frame spikes or exceeds measured memory budget.

## M4: complete all sixteen arcades and unlock loop (P2)

**Implementation checkpoint:** cabinets 9–16 also have distinct playable state machines, illustrated screens and host coverage. Native DeSmuME checks their locked-cabinet menu and requirements; individual native gameplay sessions remain outstanding. Implementation exists for the sixteen advertised cabinet identities; depth, balance and physical-DS acceptance are unfinished.

| Unlockable cabinet | Current implemented interaction |
| --- | --- |
| 09 Corporate Crawl | Choose jump or duck before an office obstacle arrives |
| 10 Stroller Drift | Steer continuously to control stroller balance |
| 11 Cereal Cipher | Select the correct cereal arithmetic total |
| 12 Tantrum Blocks | Time a block drop into the target column |
| 13 Baby's First Audit | Accept matching numbers or flag discrepancies |
| 14 Glitch Gardening | Hold or tap to water the indicated flower |
| 15 Courtroom Crayons | Trace a three-clue evidence sequence |
| 16 CEO of Daycare | Balance budget and morale through six decisions |

**Current progression:** eight starters are free; eight more require the displayed wins, Diaper Dollars and Favors. A host recovery scenario earns every paid cabinet through actual free-game rounds from a broke, heavily corrupted save. Further price/reward balancing, human full-run testing and any optional unlock routes via care achievements remain future work.

**Done when:** all 16 are mechanically distinct and replayable, requirements are attainable, prices/rewards are balanced across a full run, and save files retain purchases.

## M5: atmosphere, corruption and story polish (P2)

**Implemented checkpoint:** single-channel DS PSG cues accompany care, props, Toybox actions and the optional egg film; Room Tour/Toybox expose mute controls. Day/night and lamp states affect the nursery, and the alien-egg sequence has staged camera/creature animation, pause/advance/exit controls and scrapbook replay. These use existing rendering/audio resources.

**Remaining work:** add a richer original soundtrack and broader ending vignettes, collectable stickers and wardrobe items; keep ordinary endings playable afterward. Extend clear, recoverable corruption events and review all audio/effects controls on actual devices. Sampled voices, a multitrack soundtrack and physical-speaker balance are not yet verified features.

**Safety/accessibility requirement:** a visible reduced-effects option, no rapid strobing, reliable clues for reversed inputs, no accidental reset or surprise deletion. The secret reset always requires multiple explicit confirmations and only targets this game's own saves.

**Done when:** the player can understand what a glitch changed, sound/UI feedback conveys success and failure, all normal endings return to gameplay, and accessibility controls function across modes.

## M6: polish, QA and first public release (P0/P1)

Add controller/key remapping where practical, stylus calibration guidance, battery/suspend-resume smoke tests, input debouncing, crash/soft-lock stress tests, audio and VRAM usage measures, and actual original-DS playtesting. Make the CI ROM artifact reproducible, consider build metadata and a downloadable release with player instructions, credits and licenses.

**Done when:** QA checklists pass, no known save-corrupting problems remain, core gameplay runs acceptably on original DS, all sixteen games work, and a tagged build has release notes plus screenshots captured from an emulator or device.

## Suggested work order for the next patches

1. Keep both ROM builds, host/sanitizer suites and the exact-head native DeSmuME walkthrough green; fix any regression before merging.
2. Measure sustained frame pacing and RAM on original DS/DS Lite; verify 30-minute sessions, lid resume and FAT save/reboot/recovery. Extend the current submitted-geometry diagnostics only where useful (M0/M6).
3. Continue native-resolution nursery/egg visual review and coherent character animation, preserving rendering budgets and existing prop controls (M1/M5).
4. Improve all sixteen cabinets' feedback, instructions and full-run balance; verify touch/button play and unlock recovery on hardware (M2/M4).
5. Add bounded recent-event personality memory, contextual dialogue and richer original audio without changing saved progress accidentally (M3/M5).
6. Add longer-term activities, ending presentations and cosmetics after the core loops are reliable.
7. Finish release documentation, compatibility checks, attribution and tagged ROM provenance; publish with explicit hardware-validation scope (M6).

Do **not** claim 60 FPS from a successful cross-compile. Performance goals must be measured in actual emulator/hardware runs and logged against the exact commit.
