# Roadmap: from compiled DS prototype to complete game

**Platform:** original Nintendo DS / DS Lite, with DSi supported only through the standard DS-compatible build. **Presentation:** intentionally awkward, colorful 2000s baby-game aesthetics enhanced by original environments, better character acting, a fluid touchscreen UI and playful (not horror-themed) corruptions.

This document is a prioritized plan, not a claim that the listed work already exists. The initial milestone is a reliably playable ROM; a final complete title is a later milestone.

## Priority levels

- **P0 — correctness and release blockers:** crashes, unverified runtime, save integrity, unusable input, memory/geometry budget violations, broken build automation.
- **P1 — core product:** eight truly different starting minigames, expressive baby interactions, audio feedback, coherent progression, readable menus.
- **P2 — full game:** eight more unlockable minigames, added environments/cosmetics/quests, rich glitch effects, multiple ending presentations and personality memory.
- **P3 — optional polish:** advanced unlock modes, additional characters, local multiplayer/DS wireless experiments, extra language packs.

## M0: establish trustworthy runtime evidence (P0)

**Work:** load the latest successful `.nds` in melonDS, record a real screenshot and boot log; navigate all seven current UI views; play arcade both normal and reversed; collect at least one good and one bad ending; leave/restart and confirm a retained save on an appropriate DLDI setup; test deliberate reset with disposable save copies. Add a visible developer diagnostics page with frame count, measured frame pacing, polygon and vertex counts, and memory watermark. Capture an FPS/RAM baseline before optimization.

**Done when:** emulator boot and 30-minute smoke test are reproducible; no black-screen regression; ROM is produced by CI; results recorded in [QA_RELEASE.md](QA_RELEASE.md). Failure means fix this before new content.

## M1: strengthen the real nursery and baby (P1)

**Work:** convert the procedural baby into an authored low-poly mesh with distinguishable head, cheek, eyes, mouth, hands, clothing and hairstyle options; keep expressive odd proportions from the references. Add a small rig/pose table for idle, gaze, reach, eat, nap, laugh and tantrum; change expressions and actions using state. Turn nursery props into meaningful interaction points: bottle, changing table, toy chest, mobile, crib and dresser. Introduce 2–3 camera presets without expensive scene duplication.

**Performance approach:** share static meshes, cap joint counts, precompute transforms where practical, avoid per-frame allocations, prefer palette/UV swaps to full texture copies. Do not expand graphics until the current renderer is profiled.

**Done when:** the baby is visibly recognizable at native 256×192, at least six animations can be triggered in-game, four nursery props offer actual touch interaction, and busy scenes meet the agreed performance target on baseline hardware.

## M2: make the first eight arcades genuinely different (P1)

**Work:** replace the shared `Arcade` direction test with a dispatcher and per-game state machines. Build each of the eight **starter** games with its own controls, win condition, fail condition, score, tutorial and balanced reward.

| Starter cabinet | Proposed distinct interaction |
| --- | --- |
| 01 Bottle Balance | Tilt or steer a wobbling bottle along a short path without spilling |
| 02 Diaper Dash | Stylus drag-and-drop sequence with readable hygiene/timing cues |
| 03 Tax Return Teether | Quick number/matching puzzle with absurd tax-themed baby dialogue |
| 04 Crib Kart | Fixed-camera, low-poly obstacle racing with turn/brake controls |
| 05 Baby Beatbox | Original four-lane rhythm pattern using buttons/stylus |
| 06 Sock Market | Timed memory and matching board with changing sock prices |
| 07 Naptime Negotiator | Choose dialogue responses to calm or annoy the baby |
| 08 Rattle Rescue | Top-down scrolling maze with pickups and hazards |

**Done when:** all eight games use different core mechanics, are finishable by touch/buttons, award currency consistently, and are separately checked by tests and a hands-on playthrough.

## M3: baby's personality and meaningful care (P1–P2)

Retain the existing fixed-weight integer classifier as a fast intent-selection layer. Add a tiny event ring recording recent actions and outcomes, relationship/mood flags, topic tags and event-dependent phrase pools. Grow original dialogue beyond the current 24 lines; combine a context classifier, lightweight grammar and authored responses. Avoid claiming the DS runs a contemporary text-generation model.

Make feeding/playing/sleeping more than one-button stats: short care interactions, discoverable preferences, delayed consequences, surprise jokes and perks from building trust.

**Done when:** the baby refers to at least three recent actions in context, behaves differently across several sustained moods, repeats fewer lines, and the response system never causes frame spikes or exceeds measured memory budget.

## M4: complete all sixteen arcades and unlock loop (P2)

| Unlockable cabinet | Proposed distinct interaction |
| --- | --- |
| 09 Corporate Crawl | Side-scrolling obstacle course with crawl, duck and climb |
| 10 Stroller Drift | Cornering/racing with momentum and controlled drifting |
| 11 Cereal Cipher | Stylus pattern / substitution decoding puzzle |
| 12 Tantrum Blocks | Falling-block puzzle with mood-driven rules and shapes |
| 13 Baby's First Audit | Spot visual inconsistencies across quick scenes |
| 14 Glitch Gardening | Repair and tend an intentionally corrupted garden simulation |
| 15 Courtroom Crayons | Timed evidence sorting and ridiculous verdict decisions |
| 16 CEO of Daycare | Lightweight strategic staffing/resource-management minigame |

**Progression proposal:** begin with 8, unlock the next 8 through arcade victories and Favors, with some optional alternative routes via care achievements or good endings. Clearly display unlock requirements before purchase and prevent dead ends or exploits.

**Done when:** all 16 are mechanically distinct and replayable, requirements are attainable, prices/rewards are balanced across a full run, and save files retain purchases.

## M5: atmosphere, corruption and story polish (P2)

Add original short music loops, sound feedback and comedic baby babbling with maxmod or compatible DS audio tooling. Enrich the nursery with prop textures, inexpensive light/shade tricks, subtle particles and seasonal/room palettes. Add ending vignettes, collectable stickers and wardrobe items; preserve continuing play after every ordinary ending. Create clear, recoverable corruption events that occasionally disrupt arcade rules, dialogue or props.

**Safety/accessibility requirement:** a visible reduced-effects option, no rapid strobing, reliable clues for reversed inputs, no accidental reset or surprise deletion. The secret reset always requires multiple explicit confirmations and only targets this game's own saves.

**Done when:** the player can understand what a glitch changed, sound/UI feedback conveys success and failure, all normal endings return to gameplay, and accessibility controls function across modes.

## M6: polish, QA and first public release (P0/P1)

Add controller/key remapping where practical, stylus calibration guidance, battery/suspend-resume smoke tests, input debouncing, crash/soft-lock stress tests, audio and VRAM usage measures, and actual original-DS playtesting. Make the CI ROM artifact reproducible, consider build metadata and a downloadable release with player instructions, credits and licenses.

**Done when:** QA checklists pass, no known save-corrupting problems remain, core gameplay runs acceptably on original DS, all sixteen games work, and a tagged build has release notes plus screenshots captured from an emulator or device.

## Suggested work order for the next patches

1. Emulator smoke test + telemetry (M0).
2. Baby animations and real prop touch interactions (M1).
3. Bottle Balance and Diaper Dash as complete distinct minigames (M2).
4. Arcade dispatcher and shared rewards/tests; expand to all 8 starters.
5. Sound and baby event-memory prototype (M3/M5).
6. Remaining 8 cabinets, unlock balancing, endings and cosmetics.
7. Bug bash, save migrations, original DS benchmark, public release.

Do **not** claim 60 FPS from a successful cross-compile. Performance goals must be measured in actual emulator/hardware runs and logged against the exact commit.
