# Development status — My Baby A.I.!

This is a source-based inventory of `main`, updated through November 21, 2026 after the game's DeSmuME and BlocksDS verification milestones. **Implemented** means code exists, not that a feature has passed emulator and physical-hardware validation. See [ROADMAP.md](ROADMAP.md) for completion work, [FEATURE_IDEAS.md](FEATURE_IDEAS.md) for design pitches, and [QA_RELEASE.md](QA_RELEASE.md) for the test gates.

## What exists in the repository

| Area | Current implementation | Limitations / missing verification |
| --- | --- | --- |
| Nintendo DS ROM | devkitPro `nds-dev` + `libnds` ARM9 `Makefile`, GitHub Actions `nds-build.yml`; PR #1 produced a real `.nds` ROM artifact | A compiled header is **not** proof of a playable, stable emulator/flashcart session. Re-test latest `main` on every release |
| Dual-screen rendering | `source/world.c`: perspective-3D nursery with crib, dresser, window, floorboards, toys, teddy and a procedural 3D baby; `source/ui.c`: sub-screen 16-bit bitmap interface | Most art is procedural geometry with fixed camera; limited pose variation; no authored texture/rig pipeline or multiple explorable rooms |
| Care loop | `source/game.c`: feed, chat, play and nap affect needs, mood, chaos and corruption | Room Tour now offers meaningful window, lamp, teddy and crib interactions, including quiet-night rest; long-term developmental skills and broader rooms remain missing |
| Baby dialogue | `source/dialogue.c`: fixed-weight 6-input → 8-hidden → 6-output integer classifier; six response sets of eight lines (48 authored lines) | Not a generative LLM or trained neural model. No durable conversational/event memory, word recognition, typed chat, or long-running relationship |
| Arcade | 16 named cabinets; eight unlocked initially; distinct gameplay and art for all 16 named cabinets; the formerly generic cabinets 8–15 now have jump/duck, steering, counting, timing, audit, watering, path-tracing and management gameplay | **16 of 16 have individual mechanics on main**, with host regression and real DeSmuME touchscreen coverage. Extended balance, original hardware and DraStic touch compatibility remain unconfirmed |
| Economy | Diaper Dollars, Favors and unlock costs in `game_unlock`; wins award currency | Limited sinks/long-term rewards; no purchasable décor, apparel, talent upgrades, sticker collection, or balanced price curve |
| Glitches | Five corruption tiers, occasional visual disruption, reverse directions, shorter arcade response windows | Effects are narrow; no per-mode corruption language, glitch frequency controls, or accessibility balancing beyond the visual effect toggle |
| Endings | Four good, four bad, ending album, continuing play; separately confirmed optional reset | Basic text/bitmap endings rather than bespoke cinematics; needs stress testing around reset and recovery |
| Saves | DLDI/libfat storage, two checksummed alternating save slots, validation and scoped erase | FAT write interruption, filesystem failures, version migration and real-flashcart persistence need testing |
| Audio | `source/sfx.c`: actual single-voice DS PSG care melodies, room-switch/mobile cues, Toybox impacts and egg film audio; Y mute works in Room Tour and Toybox | No authored multi-track soundtrack, sampled voices or tested original-hardware mixing |
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

**Critical before claiming a complete game:** balance and validate all sixteen mini-games on real DS hardware; further authored baby acting and environments; longer-term activities and progression; richer original soundtrack; stronger personality memory; accessible settings; an achievable economy; and end-to-end game balance.

**Important documentation correction:** The neural classifier is already implemented, but it is a compact hand-tuned integer intent classifier, not a modern language model. DeSmuME has booted real ROM artifacts and completed native stylus, minigame and secret-cinematic checks. Physical original DS, specific DraStic versions, performance telemetry and actual flashcart save restoration remain to be verified.

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

The following implementations have reached main, with host and DeSmuME automation; physical hardware playtesting and long-run balance still remain. Every cabinet keeps its existing name and unlock number; the serialized `Game` save format has not changed.

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

**Second scrapbook layout pass:** The eight keepsakes now occupy two large 2×2 pages, with legible numbered stamps, short titles, and explicit GOOD END / BAD END / NOT FOUND indicators. Both hardware L/R and touchscreen PREV/NEXT change pages, while B/Start/A or the footer returns to the nursery. The page index is temporary UI state and never alters the persistent save schema or the arcade's selected cabinet. Added native framebuffer and actual DeSmuME navigation assertions for the two scrapbook pages.

## High-fidelity native DS graphics and arcade assets

This pass is a **game-code and asset implementation**, not a concept redesign.
The new art is original and embedded directly into existing DS ROM source.

- **Character sprite:** Refined the shared 20×18 baby illustration with chestnut highlights, peach skin shading, softer cheeks and a two-tone mint romper. The title and result screens reuse this one canonical sprite.
- **Sixteen distinct cabinet miniatures:** The four-card arcade pages now display much larger **2× nearest-neighbor artwork** in 36×34-pixel wells, including five entirely new 16×16 icon sprites: microphone (beatbox), pillow/moon (nap), ID badge (crawl), checked clipboard (audit), and crowned briefcase (CEO). Every cabinet has its own asset rather than recycling the same picture.
- **Five shaded items:** Bottle, rattle, diaper, sock, and cereal are given new glass/fabric/gold tonal shading using the existing static indexed-sprite format. The expanded material palette is 5-bit-per-channel DS BGR15, with no intermediate RGB888 framebuffer or conversion heap.
- **Interface typography:** All sixteen arcade cards receive genuinely readable 2× short wordmarks. Full game names remain available above the selected card. Bottle, teether, corporate crawl, audit, and CEO actions also gain larger 2× button labels, while touch geometry and gameplay mappings remain identical.
- **3D nursery:** The window now has layered cloud and distant-greenery quads **behind** its mullions instead of a featureless blue pane. The baby receives a subtly connected foot contact shadow, knitted shoe marks, a tiny stitched romper patch and a swept hair tuft; the title portrait mirrors that tuft.
- **Budget discipline:** No texture loader, extra static image file, framebuffer, heap allocation, new save data, or new game state; DS bottom display remains 256×192. Lightweight 1–6-quad scene details replace the need for high-triangle scenery. Existing world/title polygon/vertex limits, DS ROM tests and virtual-stylus fallback remain mandatory.
- **Regression coverage:** Host tests now independently hash the 36×34 **art region** of all 16 selected cabinets to ensure each has truly distinct visible artwork, verify the baby has a multitone rendered palette and assert no writes outside the DS visible surface. Real DeSmuME screenshots must contain a shaded selected-cabinet image and enough actual 2× title-ink pixels.

No measured FPS claim or physical DS/DraStic compatibility claim is made until tested on those devices. Care, arcade, data persistence, and the 16 mini-game mechanics are deliberately unchanged.

## DS interface & character-expressiveness pass — references checked on the web

This round follows original Nintendo DS gameplay examples, without copying their art:
- *Nintendogs* keeps the pet and care interactions ahead of obstructive status UI: https://www.famitsu.com/serial/gameiscool/201812/25169781.html
- *Tamagotchi Connection: Corner Shop 3* makes each play activity a distinct illustrated toy/shop rather than uniform menus: https://rawg.io/games/tamagotchi-connection-corner-shop-3
- *Rhythm Heaven* uses simple visual instructions and a specific rhythmic action: https://www.mobygames.com/game/40335/rhythm-heaven/screenshots/
- *WarioWare: Touched!* emphasizes readable stylus gestures and clear microgame objectives: https://www.nintendo.com/eu/media/downloads/games_8/emanuals/nintendo_ds_21/Manual_NintendoDS_WarioWareTouched_EN.pdf

Implemented in original game code:

1. **Care screens:** Each FEED / CHAT / PLAY / NAP card now has an explicit two-line hierarchy: 2x large action label and a truthful small gameplay effect (HUNGER -30, MOOD +7, MOOD +10, ENERGY +25). All four original 256x192 touch rectangles, keyboard shortcuts, game mechanics and persistence remain unchanged.
2. **Beatbox art direction:** Four separate ROM-free, rasterized illustrations (drum/BOOM, hands/CLAP, toy synthesizer/BEEP, bubble/POP) replace generic identical square symbols. The 48x47 pad hitboxes remain exact. The instruction is a readable 2x WATCH or REPEAT state cue.
3. **CEO stage:** Funding ledger, toy block and sleep-crescent objects identify all three actual decisions instead of indistinguishable flat rectangles. Button captions are concise 2x terms.
4. **Naptime:** Fixed nearly invisible white instructions over the white cot; the purple TAP IN GREEN and SWEET DREAMS! messages now have sufficient contrast and remain beneath the character portrait.
5. **Upper-screen character animation:** One stable model reacts to the care action: feeding raises a hand, chatting gestures, play adds gentle bounce, and napping closes the eyes and rests both hands. Existing geometric budgets, animation cadence, sprite assets, and room appearance remain intact.

Tests: native framebuffer test hashes the four Beatbox icon regions and verifies WATCH vs REPEAT output. DeSmuME automation captures actual upper-screen feed and sleeping poses and fails if the baby does not visibly respond. The same host save/scene/touch/arcade regression and both original and BlocksDS ROM workflows are required before merge. No physical DraStic or measured-FPS claim is made.

## Optional Toybox / synthetic-egg secret ending

The design adapts the *Talking Tom* virtual-pet HUD pattern to Nintendo DS hardware, taking inspiration from the source material's selectable bottom-row activity icons rather than copying its artwork:
https://www.talkandroid.com/4687-how-to-play-my-talking-tom-2-walkthrough-tips-hints/

### UI and gameplay
- Nursery has a consistent lower-screen three-tab action dock: **ARCADE / TOYBOX / END DAY**. The four original FEED, CHAT, PLAY and NAP cards and their existing touch hitboxes are unchanged.
- TOYBOX explicitly changes to a second lower-screen mode (the upper hardware-3D baby remains visible). Tap the **SLAP** hand or **KICK** boot icon to **select** a tool. This is a non-violent action itself; a separate tap on the lower-screen doll preview applies the selected action. Hold **L/R** to choose the tool, press **A** to use it; press **B** to return safely to ordinary care. No touch on the non-touch upper DS LCD is required. The unusual actions apply exclusively to a fictional artificial toy shell.
- The shell doll uses fixed-point velocity/gravity/bounds/floor restitution/angular rotation and damping: deterministic, bounded motion without a physics dependency or per-frame allocation. An optional R+X keyboard shortcut also opens the Toybox.
- A secret trigger requires **at least six** counted actions and **at least fifteen** charge points, protected by a short cooldown so holding a button doesn't instantly complete the ending.
- Secret BAD ending **#9** is a preexisting ending-mask bit 9; the binary save struct/version is unchanged and repeat discovery is idempotent. Existing 8 endings are untouched. The third scrapbook page reveals and replays the ending after discovery.
- Staged 3D camera film: artificial shell stress, intact alien egg, **thick bright-green extraterrestrial ichor** seeping through the cracks in opaque native polygon ribbons, an unnaturally quiet sustained camera push into the opening, and a single abrupt, close-up alien reveal. No real-human injury, red human blood, gore texture, rapidly flashing strobe loops, or imported copyrighted images. The finale is a non-gory collectible BAD ENDING.
- The upper-screen cinematic is drawn by the existing native libnds geometry renderer; the lower screen shows staged suspense cards. All new 3D elements are a small fixed collection of polygons rather than loaded video, textures or framebuffers.

### Verification gates
- `tests/test_roughhouse.c` asserts deterministic repeated impulses, bounded motion, hit cooldown, secret gate, once-only bad-ending credit, and save reload.
- `tests/test_ui.c` asserts the three-way dock, selectable Toybox, tap-to-use independence, all scrapbook pages and no framebuffer overrun.
- `tests/test_world.c` exercises moving synthetic baby and egg reveal phases while checking Nintendo DS poly/vertex budgets.
- DeSmuME integration asserts the *actual rendered* Toybox selection, animated doll kick, repeated six-hit trigger, visible alien-green fluid on the 3D egg, a clearly distinct jump-scare frame, and the BAD ENDING postcard.

Do not consider this tested on physical DS or the user's specific DraStic installation until confirmed there. These effects have not been given a measured FPS figure.

## Toybox physics and secret-shell polish, continuing after PR #25

The continuing pass upgrades the existing **optional** Talking-Tom-style TOYBOX; the normal NDS nursery care actions, arcade, native stylus, virtual stylus, and nine existing ending records stay compatible.

- **Aim rather than canned movement:** The selected SLAP/KICK still requires a *second tap*, specifically inside the 65×52px illustrated doll target. Empty text margins no longer accidentally apply an impact. The point within the doll determines a bounded horizontal and vertical impulse through `roughhouse_strike_at()`, while A is a neutral center hit. Previous velocity decays but contributes inertia to the next move; all calculations are fixed-point integers with clamped max velocities.
- **Actual collisions:** Wall X/Z boundary rebounds now also reverse angular components for a readable tumbling effect, while floor rebounds set transient impact feedback. Hit cooldown remains 4 ticks at slower DS gameplay rates. No larger physics library or save-field additions.
- **No more pose teleport:** The 3D baby's old artificial 0.46-world-unit jump at roll 90 is replaced by continuous angle-dependent compensation. A single **moving floor-shadow quad** tracks the shell and shrinks when airborne; the old stationary shadow is not left under the spawn spot.
- **Escalating reveal:** At shell charge 6, a fine crack appears on the artificial romper's chest panel using 2 ROM-native 3D decals; at charge 11, three additional green synthetic-ichor marks appear, before secret ending 9's fully staged alien egg opens. This pre-reveal uses just five extra quads (20 submitted vertices), no texture changes, no particle buffer.
- **Better lower-screen hierarchy:** A legible impact count, worsening 'SHELL STRESS' / 'CRACKED SHELL' / 'UNSEALED...' state, and 'AWAKE?' / 'ALIEN?' response foreshadow the synthetic child's hidden intelligence. The selected hand/boot icon remains unmistakable. The 256×192 screen still paints on change only.
- **Testing:** `tests/test_roughhouse.c` proves touch-left/right impulses differ, out-of-range taps clamp, collision rebound reflects with a transient pulse, and all simulated results stay deterministic. `tests/test_world.c` enforces the exact 2-quad / 5-quad stress details and 5,000-vertex / 1,850-polygon ceilings; `tests/test_ui.c` checks the reduced doll-only touch target, 2 escalating stress states, and full native framebuffer coverage. The full DS, BlocksDS and real DeSmuME integration workflows must all pass before merging.

This is additional *game code*, not a picture-only concept, and does not assert physical DS or DraStic performance measurements without those device runs.

## Toybox articulation, segmented egg and PSG audio

This pass continues from the merged PR #26's fully playable synthetic toy and secret egg.

- **Joint-coherent doll articulation:** The existing baby mesh pivots its entire face/hair/eyes/mouth around a connected neck reference (maximum ±16 degrees) as a secondary response to angular momentum. Sleeves, forearms and hands shift in coordinated pairs, with a small compression response during impacts. The main rigid-body transform remains the existing deterministic, bounded fixed-point simulation.
- **Split egg structure:** The cinematic egg now has two asymmetric curved shell plates that slide apart gradually during the slow camera push. Three more native hardware-quad ridges make the green alien shell more readable without image assets, a movie stream, or textures. The reveal remains a single non-strobing hard cut.
- **Real Nintendo DS PSG audio:** A testable one-channel event sequencer drives libnds `soundPlayPSG()` and `soundSetFreq()`/`soundSetVolume()`, and explicitly calls `soundKill()` when a cue completes. Cues include SLAP/KICK, floor/wall bounce, shell crack, alien unseal, occasional tension pulse, and the jump-scare sting. This avoids a 16-channel soundbank, uncompressed PCM, new ARM7 firmware, or large RAM allocations. See https://blocksds.skylyrac.net/tutorial/basic/audio/ and https://github.com/devkitPro/libnds/blob/master/include/nds/arm9/sound.h.
- **User control and accessibility:** The Toybox screen now displays SFX ON/OFF and supports Y to toggle its sounds for the current session. Default is enabled, and the persistent game/save struct stays unchanged. Each cue stops automatically, and exiting/skipping the film stops the active sound.
- **Synchrony:** Egg opening, slow approach, periodic low pulse, and the one-time reveal are scheduled by the exact film-state frame constants already driving the visual states. A quiet interval before the reveal increases contrast without flashing the screen or changing the script.
- **Tests:** All existing host/DS/BlocksDS/legacy/DeSmuME tests remain mandatory. A new host SFX test exercises pitch/volume bounds, mute, cue priority and completion. Geometry tests now cover just-before and just-after split/approach/scare boundaries within the existing DS frame-budget headroom.

Audio is implemented in the ROM using the supported libnds PSG API; emulator screenshot tests and host audio-envelope tests cannot certify speaker balance on an actual DS/DraStic instance. No measured FPS gain or sound-output recording is claimed.

## Visual legibility redesign — readable synthetic nursery and alien egg

The earlier screenshots made the old egg look like an indistinct mint poly blob. This is a targeted, code-level replacement of its layout, silhouette, materials and final ending UI rather than another round of isolated decorations.

### Upper screen 3D (libnds / BlocksDS)
- **Shell**: replace the center-filling pastel ovum with two visibly distinct pale-ceramic halves separated by a wide gap, each with a darker bevel and its own warm highlight; the opening increases during the existing dramatic dolly.
- **Incubation cavity**: a deep blue-black concavity sits BEHIND the two halves, ringed by shadowed moss/green lip geometry; no pale solid remains in front of the opening.
- **Parasitic occupant**: an intentional nested head silhouette is visible in the hole before the reveal, with matched pairs of luminous eyes, dark slits and a breathing mouth. Neon-green ichor now follows the physical inner lips and collects at the base rather than covering the creature.
- **Jump scare**: dark sculpted cranial silhouette, anchored brow ridges, two black eye sockets with contrasty yellow-green iris slits and glints, a recessed black maw, and seven actual pointed triangular fangs. Green trails are aligned with cheeks and edges, not free-floating neon bars.
- **Nursery**: redesign the aggressively lime play mat as dusty lavender with blush-pink flowers to restore a warm and non-alien visual baseline. The chibi baby gains dimensionally layered collar and two-tone blue-gray irises with sclera, eye rim and highlights so the facial expression reads at native 256×192.
- All changes use fixed native hardware primitives and existing 5-bit vertex colors; zero new VRAM textures, heap allocations, framebuffers, soundbanks, or major per-frame collections. Geometry regression enforces less than 5000 vertices / 1850 polygons even in the horror scene.

### Lower screen, ending, scrapbook
- **A coherent horror-interface palette:** deep ink + dark teal + acidic luminous green, without the unrelated bright baby-pink content panels, while preserving the existing readable 2x cinematic headline and four phase indicators.
- **Original cracked-egg art:** 32×24 ROM indexed pixel icon depicts two ivory shell halves, a dark cavity, green seams and creature eyes. Its 768 source characters are rendered via the already-existing run-length horizontal sprite blitter; no new decoder, transient image memory or extra BG allocation.
- The same graphic appears in the BAD ENDING #9 postcard and the *discovered* third scrapbook page. The undiscovered page remains sealed. All existing page and touchscreen hitboxes, original ending unlocks, 16 arcade games, and save binary stay intact.

### Regression and proof requirements
- Host 3D tests now cover eight frames across the visible cavity, separating shell, final face, and their polygon/vertex budgets.
- Native lower-screen 256x192 framebuffer test distinguishes undiscovered/discovered egg art, counts at least eight colors in the actual postcard sprite and checks entire VRAM coverage.
- Real DeSmuME native stylus test must complete feeding, 16-game navigation, Toybox hits, green egg, deliberate camera push, single jump scare and ending; screenshots are used for HUMAN visual review.
- Neither physical Nintendo DS nor the user's exact DraStic instance is inferred to be validated by DeSmuME. New FPS measurements are not claimed until instrumented.
