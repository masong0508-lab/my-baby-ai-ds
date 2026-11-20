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
