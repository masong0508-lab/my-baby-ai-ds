# Development status — My Baby A.I.!

This is a source-based inventory of `main` as audited after the first successful GitHub Actions ROM build (2026-11-20 UTC). **Implemented** means code exists, not that a feature has passed emulator and physical-hardware validation. See [ROADMAP.md](ROADMAP.md) for completion work, [FEATURE_IDEAS.md](FEATURE_IDEAS.md) for design pitches, and [QA_RELEASE.md](QA_RELEASE.md) for the test gates.

## What exists in the repository

| Area | Current implementation | Limitations / missing verification |
| --- | --- | --- |
| Nintendo DS ROM | devkitPro `nds-dev` + `libnds` ARM9 `Makefile`, GitHub Actions `nds-build.yml`; PR #1 produced a real `.nds` ROM artifact | A compiled header is **not** proof of a playable, stable emulator/flashcart session. Re-test latest `main` on every release |
| Dual-screen rendering | `source/world.c`: perspective-3D nursery with crib, dresser, window, floorboards, toys, teddy and a procedural 3D baby; `source/ui.c`: sub-screen 16-bit bitmap interface | Most art is procedural geometry with fixed camera; limited pose variation; no authored texture/rig pipeline or multiple explorable rooms |
| Care loop | `source/game.c`: feed, chat, play and nap affect needs, mood, chaos and corruption | No rich touch-manipulation activities, time-of-day routine, meaningful room interaction, or evolving developmental skills |
| Baby dialogue | `source/dialogue.c`: fixed-weight 6-input → 8-hidden → 6-output integer classifier; six response sets of four lines (24 authored lines) | Not a generative LLM or trained neural model. No durable conversational/event memory, word recognition, typed chat, or long-running relationship |
| Arcade | 16 named cabinets; eight unlocked initially; distinct gameplay and art for all 16 named cabinets; the formerly generic cabinets 8–15 now have jump/duck, steering, counting, timing, audit, watering, path-tracing and management gameplay | **16 of 16 have individual mechanics** on the feature branch. Fresh emulator and DS builds are still required; original hardware and DraStic touch compatibility remain unconfirmed |
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
3. Enter the arcade, select one of 16 named cabinets, and play any of 16 individually implemented arcade cabinets, eight available initially and eight unlockable through progression; each uses its own rules, illustrated screen and touch/button mapping.
4. Progress days, gain currency/Favors, influence corruption and collect endings.
5. Continue playing after a good or bad ending. Optionally access the secret, explicitly confirmed game-save reset after sufficient **bad** endings.

## Gaps that matter most

**Critical before claiming a verified playable build:** boot and frame-by-frame rendering in melonDS, input testing on both screens, 30-minute play session, save/reload, and original-hardware flashcart testing. The GitHub Actions compiler cannot certify those.

**Critical before claiming a complete game:** balance and validate all sixteen mini-games on real DS hardware; authored baby animation and environment interactions; additional activities and progression; sounds/music; stronger personality memory; accessible settings; an achievable economy; and end-to-end game balance.

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
- **Verification:** prior mainline CI passed for the first eight modes. This next feature branch adds independent host logic, screen-rendering and hitbox regression cases for cabinets 8–15; fresh ROM and emulator checks are required before merge.
- **CI regressions repaired:** Rattle Rescue now has an active UI draw path and resets properly on timeout; emulator Diaper Dash script now performs real continuous gestures rather than obsolete one-tap tests.
- **Testing:** new host tests cover gesture sequence, premature catches, lane matching, timeouts, high-glitch speed and lane hitboxes. The ROM build and emulator still need independent verification for this new branch; source changes alone are not confirmed runtime screenshots.

The game save structure and on-disk save format remain unchanged. All new transient fields live in `Arcade` and are discarded when a cabinet ends.

## Remaining eight arcade cabinets — bespoke mechanic pass

The following implementations are present in the feature branch and must pass host/DS/emulator CI before merging. Every cabinet keeps its existing name and unlock number; the serialized `Game` save format has not changed.

| Cabinet | Actual activity | Inputs |
| --- | --- | --- |
| 8 Corporate Crawl | See the approaching floor crate or HR scanner, choose jump or duck and survive six obstacles | Stylus choice or UP/DOWN |
| 9 Stroller Drift | Correct continuous steering against a drifting track to complete five stable sections before time expires | Hold left/right on touchscreen or D-pad |
| 10 Cereal Cipher | Solve an addition puzzle, choose one of three numbered cereal bowls, six rounds | Stylus or LEFT/DOWN/RIGHT |
| 11 Tantrum Blocks | Drop horizontally moving blocks only when they align with the highlighted lane, six correct placements | Tap or A |
| 12 Baby's First Audit | Compare receipt and invoice totals, then APPROVE or FLAG six cases | Stylus or LEFT/RIGHT |
| 13 Glitch Gardening | Hold water on the highlighted flower bed until four plants are grown; wrong D-pad watering incurs strikes | Held stylus or LEFT/DOWN/RIGHT |
| 14 Courtroom Crayons | Trace a shuffled three-clue evidence path in number order across four cases | Continuous stylus or LEFT/UP/RIGHT accessibility input |
| 15 CEO of Daycare | Balance budget and morale through six INVEST, PLAY or NAP decisions | Stylus or LEFT/DOWN/RIGHT |

No new runtime heap buffers, image decoders or serialized fields were introduced. Existing bitmap rendering is throttled for animated cabinets; validate frame rates on the target DS and emulator rather than assuming a compile proves performance.

## Unified bottom-screen interface overhaul (development branch, 2026-11-20)

All eight application views and all 16 unique cabinets share an updated Nintendo DS visual language. It is rendered in the existing 256×192 16-bit sub-screen bitmap, with no image loader, heap allocation, framebuffer duplication, font assets or save-format changes.

- **Title:** illustrated baby portrait, hierarchy for START, explicit SD/RAM state, and accurate button hints.
- **Nursery:** legible care dashboard, mood/hunger/energy/chaos and corruption meters, readable baby text, four raised action controls. The original FEED/CHAT/PLAY/NAP and footer hitboxes are unchanged.
- **Arcade:** refined 4×4 cabinet matrix with strong selected/locked states, currency, readable cabinet name and native key hints; all original 16 cabinet hitboxes are preserved.
- **Every arcade challenge (16/16):** individual accent theme, numbered cabinet header, progress and three-strike bar, consistent inset stage border, improved dimensional cards and buttons. Original game rules remain unchanged.
- **Ending cards:** illustrated baby keepsake with clearer good/bad color coding, one-time reward description and album progress.
- **Album:** eight individual scrapbook entries, reveal/locked state, short readable names, good/bad result indicator.
- **Reset:** explicit two-step keyboard confirmation panel explaining the scoped save deletion; cancel still available and no new touch-to-reset shortcut.
- **Touch test:** high-contrast live signal panel with the original 220×52 canvas; partial crosshair updates and last-input tracking stay intact.
- **Shared interface:** coordinated high-contrast top bars, SD-vs-RAM status, card highlight/shadow hierarchy and footer action bands.
- **Rendering:** replaced bitmap glyph-by-glyph pixel writes with identical horizontal runs; removed the redundant 49,152-pixel full-screen clear (all views fill their own content, with header and footer covering the remaining rows); reduced cabinet background dot draws. Static cabinet panels now redraw roughly once a second for timers while gesture/timing scenes retain their responsive cadence. Gardening triggers a redraw only at useful progress increments rather than every held frame.
- **QA:** new host assertions check full 256×192 coverage, untouched offscreen 256×64 rows, distinct fingerprints across all 16 cabinets, safe/SD and ending/reset visual variants, plus gardening progress redraw cadence. CI ROM build and DeSmuME touchscreen tests still have to pass before this branch is merged.

Changes to the top-screen 3D nursery geometry are deliberately out of scope for this UI refresh: they would compete with the DS polygon/frame budgets. Character animation and 3D environments remain a separate production effort.

## Baby model & vertical seam cleanup (feature branch)
The DS top-screen character/room receives a targeted geometry correction:
- Rebuilt the baby as a soft rounded, short-haired chibi with a pastel one-piece romper, attached hands and feet, smaller shallow eyes, blush, button nose and a small closed-mouth smile. Replaced the prior dark bob/denim outfit, floating eye layers and wide tooth-bearing mouth.
- Reworked the independent boot/title 3D portrait to match the nursery baby without rendering the complete room.
- Updated the 18-row ROM-embedded baby sprite for title and all mini-game screens; batched same-color horizontal sprite pixels to reduce bitmap writes.
- Removed eight full-length floor lines, six skinny plank-end strips, two narrow vertical wainscot marks and three floating mobile stickers, all subpixel-width details prone to looking like artifacts at 256×192.
- Disabled GL edge antialiasing for this opaque low-poly scene. Adjusted nursery camera to a less oblique 51-degree composition to avoid face distortion and prominent polygon edges.
- No extra framebuffers, texture VRAM, heap allocations or save schema changes. Existing gameplay and button layouts are unchanged.
- The real DeSmuME integration test now captures `game_nursery_hero_top_screen.png`, verifies that hardware 3D has nontrivial visible color variety, and fails on isolated almost-full-height dark screen stripes. Static native tests protect polygon budgets, no surprise geometry in glitch/safe display states, and visible baby title palette.

**Validation:** Actual DS and DraStic screenshots, frame times, and aesthetic approval are still necessary; CI checks and code inspection alone do not prove the baby is beautiful or that every possible hardware rendering defect is gone.

**Screenshot-driven follow-up:** Inspected the actual DeSmuME 256×192 nursery capture from this branch. It revealed that the large foreground teddy covered the baby's torso and legs despite the 3D check passing. The teddy has now been moved to the left near the dresser and grounded on the floor; the baby's pale romper and worried-looking single-line mouth were adjusted to a stronger pastel contrast and small curved smile. Fresh screenshots from the final commit must be reviewed before calling that visual occlusion resolved.

## Readable dual-screen UI redesign — inspiration-led pass

This replaces the earlier overly dense, bootleg-looking pixel menus with Nintendo DS-native layouts. Design references were reviewed as inspiration for hierarchy and ergonomic principles, **not copied artwork**:
- *Nintendogs* (Nintendo DS): keep the character on the upper display and make the lower display an uncluttered care-menu surface.
- *Cooking Mama 2* (Nintendo DS): large character identity, dominant single call-to-action, plain instruction typography.
- *Tamagotchi Connection: Corner Shop 3* (Nintendo DS): consistent visual personality while keeping the primary mini-game touch action prominent.
- *Professor Layton and the Curious Village* (Nintendo DS): restrained borders, legible list rows, selected states.

### What changed in source

- Title screen: a lightweight native 3D baby portrait appears on the top display once the bottom-screen title has been visible for 16 frames. Bottom screen uses a centered 2x title, large baby art and one full-width START action. This does not require loading textures or allocating a framebuffer.
- Nursery: a two-line dialogue area, four compact needs meters, a corruption line and four substantially larger care cards with **2x** action text. Touch hitboxes now align with the entire visible button, not just the lower part.
- Arcade: sixteen cabinets are organized into **four pages of four**. The 2x2 cabinet cards now offer real identification, readable readiness/lock state, selected border and big numbers. PREV/NEXT touch zones and L/R shoulder paging permit all cabinets to be reached; D-pad up/down navigates rows. The selected cabinet is still playable with A or a second tap.
- All sixteen mini-games: prominent 2x headings with shorter on-screen names; full longer titles remain in the cabinet catalogue. The gameplay areas and native touch hitboxes are unchanged.
- Album: one line per keepsake and a visible discovery counter, instead of overlapping text lines.
- Shared system chrome: large screen titles, stable separate day/money badges, a minimal footer, fewer decorative strokes, flat card hierarchy and stronger margins.

### Guardrails and verification

- All gameplay state, persistence, and the 256x192 bottom bitmap format remain unchanged.
- Added 2x2 pagination/hitbox tests to the host UI suite, which also validates full screen coverage and the offscreen scanlines.
- The real DeSmuME touch test now checks the actual 3D title portrait and selects/enters the second oversized cabinet using real stylus coordinates.
- Existing physical DS budget and RAM rules apply: no extra textures, framebuffer, data decoder, image assets or per-frame heap work.
- Host ROM compilation and DeSmuME checks must pass before merge. Review actual emulator screenshots for text clipping and hierarchy; a compiled program is not proof of aesthetically successful UI.

Reference pages: https://www.nintendo.com/en-gb/Games/Nintendo-DS/Cooking-Mama-2-270341.html ; https://www.nintendo.com/en-gb/Games/Nintendo-DS/Tamagotchi-Connexion-Corner-Shop-273223.html ; https://www.mobygames.com/game/32910/professor-layton-and-the-curious-village/screenshots/nintendo-ds/301323/

## UI polish follow-up: top-screen title, quiet playfields and real menu smoke tests

- Expanded the DS-native 3D title scene from a small floating portrait on the flat clear color to a closer, grounded baby hero with pastel backdrop, stage and two subtle decorations. A tiny whole-baby vertical bob adds life with no new textures, image files, heap allocations or persistent state.
- Removed the left and right dotted/dashed edge patterns that appeared as vertical lines in the 16 minigame playfields. All cabinet mechanics and touchscreen hitboxes stay at their original coordinates.
- Rebuilt the result postcard with 2× readable outcome headings, character portrait, a clear reward, and completion progress. Its continue controls are unchanged.
- Replaced the wall-of-microtext reset view with large two-step hardware confirmation text while explicitly preserving L+R+X as the only destructive input.
- Reformatted the touch diagnostic title while preserving the 220×52-pixel uniform touch canvas and its partial-crosshair erase logic.
- Added real DeSmuME tests that enter the ending scrapbook via the L shoulder button, return, and traverse the arcade's PREV/NEXT touch page controls before completing the existing four-stage Diaper Dash test; capture the album and both arcade page images for review.
- Layout acceptance checks are in CI alongside the existing host tests, 3D vertex/polygon limits, DS build, BlocksDS packaging, and native stylus smoke test. No claims of physical DS/DraStic verification or quantified FPS gains.
