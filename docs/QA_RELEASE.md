# DS testing, benchmarking and release checklist

**Truth in labeling:** a successful GitHub Actions ROM build proves the cross-toolchain generated an `.nds` file with plausible NDS headers. It does **not** prove a correct first frame, readable UI, correct touch coordinates, acceptable FPS, DLDI saves or reliable original-DS play.

## What CI currently checks

The workflow `.github/workflows/nds-build.yml` installs devkitPro `nds-dev`, runs `sh tests/run.sh`, uses `make clean && make -j2`, invokes `ndstool -i`, checks ROM header/ARM9 ranges, and uploads `my-baby-ai-nds`.

Host-side tests cover game/save/endings, UI raster and hitboxes, the tiny dialogue classifier, and a stubbed geometry-submission budget. Stub tests are useful regression guards **but are not hardware rendering tests**. At least one successful ROM build was produced on the CI pull-request branch before PR #1 merged; check the latest `main` run again for any release.

## Mandatory emulator smoke tests — not yet recorded

- [ ] Open the exact `my-baby-ai.nds` artifact in a current melonDS build configured for original DS mode.
- [ ] Record emulator version, host OS, repository SHA and ROM hash.
- [ ] Capture *real emulator* screenshots of title, nursery, arcade grid, arcade game, good ending, bad ending, album and reset warning.
- [ ] Verify main 3D view renders without missing faces, Z-fighting, black screens or clipped baby.
- [ ] Verify lower-screen 256×192 UI is legible and all large stylus buttons register accurately.
- [ ] Confirm A/B/X/Y, Start/Select, L/R and directional keys across the full flow.
- [ ] Reach one good ending and one bad ending; confirm both return to normal gameplay without loss of progress.
- [ ] Confirm arcade reverse prompts match displayed text and scoring/timer behave correctly.
- [ ] Run for at least 30 minutes without a softlock, crash or severe degradation.
- [ ] Verify save write, reboot, load and corrupted-slot recovery using a compatible persistent save setup.
- [ ] Verify optional reset with **disposable test saves only**, ensuring unrelated files are untouched.

## Required original-hardware tests — not yet recorded

- [ ] Original DS or DS Lite with an appropriate DLDI-compatible flashcart/storage.
- [ ] Fresh install boot and a session longer than 30 minutes.
- [ ] Test both screens for flicker, z-order issues, clipped text and stuck pixels.
- [ ] Check stylus sensitivity at edges and corners; test left/right-handed play.
- [ ] Benchmark idle nursery, baby animation, arcade, full corruption and ending display.
- [ ] Observe sleep lid close/reopen, suspend, low-battery behavior and shutdown.
- [ ] Test interrupted writes only on a backed-up/disposable save and confirm one valid slot survives if possible.
- [ ] Verify the game still plays in RAM-only mode if FAT initialization fails.
- [ ] Record any hardware-specific differences from emulator results.

## What to measure and report

For each test record these fields:

```text
Date (UTC):
Git commit:
ROM SHA-256:
ROM bytes:
Emulator + version / DS model + flashcart:
Scenario:
Frame-rate min / median / 1% low (measured):
Frame time worst-case (ms):
Vertex / polygon count maximum:
Available RAM / observed high-water usage:
Graphics VRAM allocation:
Touch latency or missed input observations:
Save/reboot result:
Screenshots / log path:
Pass / fail and issue links:
```

**Budget guardrails, not claims:** original DS is roughly 4 MiB main RAM; main 3D hardware has finite polygon and vertex processing capacity; lower 16bpp UI uses a 256×256 bitmap backing (about 128 KiB **VRAM**, not main-RAM heap). Check actual figures on the target device. Target a stable full-rate DS VBlank cadence (~60 Hz), with explicit lower-rate modes only if justified and measured.

## Additional automated tests to add

- [ ] Per-cabinet unique gameplay tests once minigames have separate implementations.
- [ ] Long-run, seeded gameplay simulations across thousands of day/action/ending cycles.
- [ ] Save-file fuzzing/truncation/version migration (including power loss and two-slot ordering).
- [ ] Economy soft-lock test: always a path to more currency/Favors.
- [ ] Optional-reset state machine: confirmation required, cancellation available, no extra file deletion.
- [ ] Text overflow/font coverage tests for long dialogue and translations.
- [ ] VRAM and geometry budgets recorded with each feature change, not inferred from source line counts.
- [ ] Gameplay tests with reduced visual glitches and corrupted audio off.
- [ ] Release artifact provenance: source SHA, header checks, hash and downloadable build.

## Release gate

A release candidate needs (1) green cross-build/tests, (2) verified emulator gameplay, (3) original DS/DS Lite smoke test or an explicitly labeled hardware-untested preview, (4) working saves/recovery, (5) all advertised minigames actually implemented, (6) no high-severity crashes/corruption, (7) clear controls and support docs, (8) original/legally usable assets and licensing, and (9) reproducible tagged binaries with changelog.

Never put an ESRB rating, Nintendo endorsement or third-party character/artwork on the package unless actually authorized. The game's parody branding and rating are separate from any official endorsement.

## DraStic double-white-screen regression (2026-11-20)

A user-provided screen recording of DraStic opening the ROM shows both displays remaining white before any title graphics; the emulator radial menu opens, but the game itself shows no visible progress. Root cause is **not yet proven**.

Check both CI binaries after the boot compatibility patch:
- `my-baby-ai.nds`: standard devkitARM packaging.
- `my-baby-ai-ds-only.nds`: identical program code, DS-only unit code and recomputed header CRC.

Expected diagnostic order:
1. Boot into a purple top screen and full-color title UI on the bottom **without** touching storage or running 3D.
2. Press A/START to enter nursery 3D. Record whether it freezes at that transition.
3. Restart, press X on the title to attempt enabling saves. Test only with disposable saves and record whether storage initialization stalls.
4. If both ROM variants still show immediate white screens, investigate libnds v2/ARM7 runtime and DraStic-specific homebrew behavior; test the same exact SHA in melonDS and real hardware before asserting the root cause.

No regression is considered fixed without actual emulator screenshots or a successful player confirmation.

## Alternative-runtime boot regression

The first DraStic tester still reported white screens with both devkitPro
ROM header variants. Based on independent libnds 2 startup/emulator reports, an
alternate **BlocksDS** runtime is now built by
`.github/workflows/blocksds-build.yml`. It ships a whole-game ROM and a
minimal dual-screen color probe. Both receive DS-only compatibility copies.

- [ ] DraStic: DS-only BlocksDS color probe produces two distinct colorful screens.
- [ ] DraStic: DS-only BlocksDS full game shows a title.
- [ ] DraStic: START enters 3D nursery without hanging.
- [ ] DraStic: X save-device probe does not prevent normal RAM-only gameplay.
- [ ] melonDS: exact same BlocksDS ROM passes boot + 30-minute gameplay.
- [ ] Original DS Lite: same build boots and passes basic input/save tests.

Without a positive report for at least the first two checks, **do not**
describe the DraStic white-screen bug as fixed.

## Field report: BlocksDS ROM boots in DraStic

The user supplied DraStic screenshots showing a purple-background title on
the lower screen and a furnished low-poly nursery on the upper screen after
START. Thus the BlocksDS runtime solved the previous **observed boot white
screen** on that emulator configuration. Actual stylus handling remains
unconfirmed and requires the new diagnostic screen; no FPS measurements
or physical DS tests have been supplied.

### Touchscreen regression checklist

- [ ] At title press SELECT, and verify `STYLUS SIGNAL TEST` is visible.
- [ ] Touch upper-left, center, lower-right of bottom screen; the crosshair
      and X/Y coordinates follow the stylus within original 256x192 bounds.
- [ ] One short tap produces one increment; holding down does not repeat.
- [ ] B/START exits touch diagnostics.
- [ ] FEED/CHAT/PLAY/NAP respond from their drawn hitboxes.
- [ ] Tapping ARCADE and END DAY on the footer navigates correctly.
- [ ] Tap a cabinet, then tap again to play; check BUY/BACK regions.
- [ ] Diaper Dash WIPE, NEW, FOLD and TAPE match their colored buttons.
- [ ] Bottle Balance responds continuously while holding the stylus
      on the left/right side and while holding D-pad left/right.
- [ ] All buttons still work without stylus input.
- [ ] Recheck DS geometry and FPS on normal, corrupt and animated scenes.

The field report does not establish that the stylus regression is fixed
until an emulator retest verifies those steps.

## ARM7 touchscreen runtime A/B isolation (November 2026)

The user's DraStic recording demonstrates the standalone stylus probe not receiving `KEY_TOUCH`, while game screens render. The user has confirmed that **every DraStic touch-toggle combination has already been tried**, so repeating settings instructions is not appropriate.

BlocksDS provides both `arm7_maxmod.elf` (original default used by this project, with optional sound handling) and `arm7_minimal.elf` (basic libnds services only). Both should support stylus input, but they have different ARM7 executable code. GitHub Actions now generates the same diagnostic and full game using the minimal core, and publishes DS-only variants. The old Maxmod build is retained unchanged.

**Next emulator comparison:**

- Open `blocksds-touch-probe-minimal-arm7-ds-only.nds` on DraStic. Press lower screen; observe TOUCH and X/Y.
- If the indicator turns green on minimal but not Maxmod: isolate a runtime/core compatibility difference; test the matching `my-baby-ai-blocksds-minimal-arm7-ds-only.nds`.
- If both remain OFF: the common input path or DraStic's stylus mapping/runtime is suspect. Stop changing on-screen button hitboxes and use a known working touchscreen DS game or a second emulator as an external control.
- If input works in the independent probe but not the matching game: investigate game-specific polling, screen modes or overlay behavior.

This is an experiment, not a confirmed fix. Always label the exact ROM and emulator version used in reports.
