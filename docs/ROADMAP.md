# Roadmap: from compiled DS prototype to complete game

**Platform:** original Nintendo DS / DS Lite, with DSi supported only through the standard DS-compatible build. **Presentation:** intentionally awkward, colorful 2000s baby-game aesthetics enhanced by original environments, better character acting, a fluid touchscreen UI and playful ordinary corruptions, plus the optional Toybox/alien-egg horror cinematic.

This document separates implemented checkpoints from remaining acceptance work. The current priority is the neural conversation and responsiveness milestone following [PR #67](https://github.com/masong0508-lab/my-baby-ai-ds/pull/67): typed WARDEN/BABY conversation, bounded local word generation, truthful observation, learned phrases and the Last Nursery threat/counter loop. See [the story direction](THE_LAST_NURSERY.md). All sixteen cabinets, nursery props, care/result acting, PSG feedback, submitted-geometry diagnostics and touch-accessible saves remain in place. These do not establish a complete title or physical-DS release readiness.

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

**Implemented checkpoint:** the procedural baby has connected care gestures, gaze/blink and need-dependent fatigue/restlessness. Interrupted care carries the actual in-flight rig into the next gesture over 12 ticks, including energy-dependent arms and eyelids. Actual arcade wins and losses trigger distinct finite cheer/regroup gestures using the same rig. Room Tour offers window, lamp, teddy and crib touch interactions, quiet-night rest, an animated mobile and smoothly interpolated prop camera views. Native screenshots and geometry tests cover these paths. See [ARCADE_BABY_REACTIONS.md](ARCADE_BABY_REACTIONS.md), [CARE_MOTION_CONTINUITY.md](CARE_MOTION_CONTINUITY.md) and [ROOM_TOUR.md](ROOM_TOUR.md).

**Remaining work:** refine anatomy, pose transitions and scene composition from native-resolution captures; extend authored acting and useful prop interactions where it improves play. An authored mesh/texture pipeline, hairstyle options, more rooms and a measured baseline-hardware performance target remain unfinished.

**Performance approach:** share static meshes, cap joint counts, precompute transforms where practical, avoid per-frame allocations, prefer palette/UV swaps to full texture copies. Do not expand graphics until the current renderer is profiled.

**Done when:** the baby is visibly recognizable at native 256×192, at least six animations can be triggered in-game, four nursery props offer actual touch interaction, and busy scenes meet the agreed performance target on baseline hardware.

## M2: make the first eight arcades genuinely different (P1)

**Implementation checkpoint:** all eight starter cabinets have individual mechanics, artwork and input paths, with host gameplay/hitbox coverage and native DeSmuME completion scenarios. New motion/timing playthroughs cover Bottle spills, steady-progress decay and recovery, Kart collision/avoidance, Naptime early/duplicate inputs, and Rattle early/late catches. The Bottle safe band matches the actual scoring window; the Teether held receipt stays visible over its destination tray. All sixteen cabinets receive bounded progress/mistake/result feedback, with silent ignored inputs and no duplicate cues. Original-hardware and extended human balance/playability testing remain outstanding. See [NATIVE_STARTER_ARCADE_TESTS.md](NATIVE_STARTER_ARCADE_TESTS.md), [NATIVE_ARCADE_TESTS.md](NATIVE_ARCADE_TESTS.md) and [ARCADE_FEEDBACK.md](ARCADE_FEEDBACK.md).

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

## M3: baby's personality and meaningful care (current P1 focus)

**Microphone starter (2026-11-22):** A new opt-in MIC LAB inside WARDEN
provides ARM7 microphone capture, a level meter, train/listen/confirm controls
and a compact user-adapted 24-label, two-example template recognizer.
It stores features, not raw recordings, in a separate optional checksummed
save companion. Unknown input is explicitly rejected, and spoken labels are
only submitted to existing typed conversations upon an explicit USE command.
This is limited, speaker-trained word/short-phrase matching, not unrestricted
voice transcription or a trained large language model. Physical DS mic
accuracy, multiple speakers, noise, DraStic mic forwarding and latency
remain separate validation priorities.


**Implemented checkpoint:** the fixed-weight 6-input/8-hidden/6-output integer classifier selects from **48 authored lines** in six eight-line pools, with **12 additional contextual replies** for six kinds of recent care/arcade events. A 20-byte session memory keeps four distinct events, recalls each once on healthy CHAT, prevents immediate repeated lines and expires old events. Urgent needs take precedence. Care and arcade RNG progression and the version-1 save remain unchanged. Host tests cover recall, priority, aging and RNG preservation; native DeSmuME reads the actual rendered bottle-memory reply after FEED then CHAT. See [RECENT_INTERACTION_MEMORY.md](RECENT_INTERACTION_MEMORY.md), [DIALOGUE_REACTIVITY_MILESTONE.md](DIALOGUE_REACTIVITY_MILESTONE.md) and [DAY_CYCLE_MILESTONE.md](DAY_CYCLE_MILESTONE.md).

**Neural conversation checkpoint:** a six-prompt TALK view adds questions, topical continuations and explicit feedback. A small learned output adapter uses the existing neural hidden activations plus conversation context to choose warm/playful/corporate wording. Actual care outcomes and recorded events constrain the eligible responses; urgent needs cannot be trained away. Learned preferences persist in a 40-byte, checksummed companion profile bound to the exact saved Game, without changing v1 progress bytes. FEED/PLAY/NAP replies distinguish useful, redundant and tiring care. See [NEURAL_CONVERSATION.md](NEURAL_CONVERSATION.md), [LEARNING_SAVE_COMPATIBILITY.md](LEARNING_SAVE_COMPATIBILITY.md), and [NATIVE_CONVERSATION_TESTS.md](NATIVE_CONVERSATION_TESTS.md).

**Shelter AI checkpoint:** a typed terminal, original 412-token neural word generator, factual observation/archive/calculator paths, four persistent taught phrases, durable event counts and an actual last-event record, and telegraphed attacks/counters now provide a first playable Last Nursery loop. The optional observer companion leaves v1 Game unchanged. See [observer implementation](SHELTER_OBSERVER.md) and [word-model scope](NARRATIVE_LANGUAGE_MODEL.md).

**Relationship checkpoint (2026-11-22):** WARDEN/BABY now accept TRUST, BOND, FRIEND and CARE PATTERN, responding to actual persistent care counts and current needs with stage-specific wording. WARDEN explicitly does not invent a trust meter. The reply focus supports MORE; the original v1 progress and observer companion format are unchanged. Host and native observer tests verify zero-care and hostility cases.

**Remaining work:** deepen sustained relationships, useful learned behavior beyond phrases/style, broader subject recognition, more diverse generated dialogue, several-day campaign pacing and a fuller durable event history. The older TALK session's short-term facts reset on load/day changes; the new observer retains counters and its last recorded event separately; learning a conversational style does not invent a transcript or train language understanding. Preserve old progress and independently test any future durable-event schema.

Make feeding/playing/sleeping more than one-button stats: short care interactions, discoverable preferences, delayed consequences, surprise jokes and perks from building trust.

**Done when:** the baby refers to at least three recent actions in context, behaves differently across several sustained moods, repeats fewer lines, and the response system never causes frame spikes or exceeds measured memory budget.

## M4: complete all sixteen arcades and unlock loop (P2)

**Implementation checkpoint:** cabinets 9–16 also have distinct playable state machines, illustrated screens and host coverage. Native DeSmuME checks their locked-cabinet menu and requirements, then earns six wins, purchases Cereal Cipher, waits through two question timeouts and completes six visible sums. The other seven paid cabinets still need individual native gameplay sessions. Implementation exists for all sixteen identities; depth, balance and physical-DS acceptance are unfinished.

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

**Implemented checkpoint:** single-channel DS PSG cues accompany care, props, arcade progress/mistakes/results, Toybox actions and the optional egg film; Room Tour/Toybox expose global mute controls. Result melodies retain priority over ordinary arcade chirps, and dramatic egg cues remain protected. Day/night and lamp states affect the nursery, and the alien-egg sequence has staged camera/creature animation, pause/advance/exit controls and scrapbook replay. These use existing rendering/audio resources.

**Remaining work:** add a richer original soundtrack and broader ending vignettes, collectable stickers and wardrobe items; keep ordinary endings playable afterward. Extend clear, recoverable corruption events and review all audio/effects controls on actual devices. Sampled voices, a multitrack soundtrack and physical-speaker balance are not yet verified features.

**Safety/accessibility requirement:** a visible reduced-effects option, no rapid strobing, reliable clues for reversed inputs, no accidental reset or surprise deletion. The secret reset always requires multiple explicit confirmations and only targets this game's own saves.

**Done when:** the player can understand what a glitch changed, sound/UI feedback conveys success and failure, all normal endings return to gameplay, and accessibility controls function across modes.

## M6: polish, QA and first public release (P0/P1)

Add controller/key remapping where practical, stylus calibration guidance, battery/suspend-resume smoke tests, input debouncing, crash/soft-lock stress tests, audio and VRAM usage measures, and actual original-DS playtesting. Make the CI ROM artifact reproducible, consider build metadata and a downloadable release with player instructions, credits and licenses.

**Done when:** QA checklists pass, no known save-corrupting problems remain, core gameplay runs acceptably on original DS, all sixteen games work, and a tagged build has release notes plus screenshots captured from an emulator or device.

## Suggested work order for the next patches

1. Keep both ROM builds, host/sanitizer suites and the exact-head native DeSmuME walkthrough green; fix any regression before merging.
2. Focus on the Last Nursery response and observation systems: coherent questions/follow-ups, learned preferences, useful care reactions and broader original response topics, with truthful memory and saved-progress compatibility (M3).
3. Measure sustained frame pacing and RAM on original DS/DS Lite; verify 30-minute sessions, lid resume and FAT save/reboot/recovery. Extend the current submitted-geometry diagnostics only where useful (M0/M6).
4. Continue native-resolution nursery/egg visual review and coherent character animation when required by an interaction, preserving rendering budgets and existing prop controls (M1/M5).
5. Improve cabinet balance and richer original audio after the response-system milestone; verify touch/button play and unlock recovery on hardware (M2/M4/M5).
6. Add longer-term activities, ending presentations and cosmetics after the core loops are reliable.
7. Finish release documentation, compatibility checks, attribution and tagged ROM provenance; publish with explicit hardware-validation scope (M6).

Do **not** claim 60 FPS from a successful cross-compile. Performance goals must be measured in actual emulator/hardware runs and logged against the exact commit.
