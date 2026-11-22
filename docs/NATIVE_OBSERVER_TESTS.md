# Native shelter terminal, learning and hostile counter test

`tests/emulator_observer.py` drives the production BlocksDS DS-only ROM through
real DeSmuME stylus and keypad events. It reads the game's visible bitmap font,
keyboard, care meters, WARDEN reports and bracer readout. It does not read emulator
RAM, inject saves, alter progression, call the C engine or predict gameplay RNG.

The workflow job **Type to WARDEN, teach the baby and counter hostile attacks**
depends on the existing DS-only packaging job and retrieves the exact ROM built
in the same run. Its report records that binary's SHA-256, source hashes,
emulator cycles, screenshots, inputs and observed facts. Evidence is uploaded as
`desmume-observer-evidence` even when the test fails.

## Real input and readable responses

The scenario starts a new RAM-only baby through the visible START card. It
checks the title's outlawed-infant/shelter premise and captures the nursery.
CPU touch and CARE UP must open MONITOR. B and the BACK footer must return
without changing visible care meters, day or currency.

The test reads every visible glyph on each keyboard page. D-pad selection alone
must not type or ask; A types exactly the selected character. A held Q key
remains one Q across seven observations over 95 emulator cycles. Physical
backspace, space and SELECT page selection are used alongside native keyboard
touches. The visible numeric page types `12+7`, and SEND must produce the exact
result `12 + 7 = 19.`. X, START and the SEND footer are exercised.

STATUS must report the actual care counters. WORLD must describe the hidden
nursery and acknowledge its limited knowledge of survivors. An unsupported
question must receive an honest fallback. Y and the TARGET footer switch
between WARDEN and BABY. Generated HELLO and DREAM replies must be nonempty,
readable with the production font, and correctly framed for an imagined dream;
no exact generated sentence is presumed.

## Teaching must affect actual play

The player types **TINY BOSS** and presses TEACH three times. The first lesson
must be learned and subsequent lessons reinforced. `SAY WHAT I TAUGHT` retrieves
the exact stored phrase, and `WHAT DID I TEACH` shows one slot rather than three
duplicates. After leaving the terminal, an actual healthy PLAY must produce
**YOU TAUGHT ME: TINY BOSS** without another SAY request.

These screenshots establish user-visible phrase acquisition, reinforcement,
retrieval and use. They do not expose or independently measure neural weights.
Host tests verify the model's numerical and deterministic learning properties.

## Earned hostility, visible warning and recoverable damage

The first PLAY and 63 further genuine care actions bring each of FEED, CHAT,
PLAY and NAP to sixteen recorded uses. No hidden event or stage field is set.
WARDEN must report HOSTILE, those four exact counters, zero hits, and full
bracer integrity. A windup may already have begun near the end of the care
sequence, but no damage is permitted to occur unnoticed during setup.

The terminal remains open for 660 emulator cycles. Its attack log must stay
unchanged, demonstrating that typing pauses the hostile clock. Returning to
CARE must expose a clear GUARD badge and windup footer. Touching GUARD blocks
the attack, keeps integrity at 100, and records one actual block.

The next windup is allowed to finish. The test captures the hit/lunge, then
checks that WARDEN reports one confirmed hit and integrity 92. Typing REPAIR
restores integrity 100 while preserving the observed hit and block history.
Care meters, day and currency must remain unchanged by these attacks/repairs.

Finally, two more real windups are countered in TOYBOX: one successful SLAP and
one successful KICK. Their visible shell-hit counter must reach 1/6 and 2/6,
and WARDEN must report one of each successful input and the additional blocks.
Two hits stay below the optional six-hit egg reveal threshold. The test waits
for visible windups; it does not inspect ragdoll motion or cooldown memory.

## Run locally

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  timeout 240 python tests/emulator_observer.py \
  my-baby-ai-blocksds-ds-only.nds --output emulator-observer-proof
```

Requires the same pinned `py-desmume==0.0.9` and Pillow setup used by the other
native suites. Run a fresh emulator process against the exact candidate ROM.
`observer-report.json` is written on success and failure, with a failure
screenshot when an emulator instance exists.

## Evidence limits

This native gate validates actual input, visible behavior, factual reports,
phrase use and recoverable hostility. Host tests separately cover full Game/RNG
isolation, bounded inference, parsing, save compatibility and fault handling.
RAM-only emulator captures do not establish FAT storage or learning persistence
across reboot, physical DS/DraStic acceptance, measured RAM, sustained FPS,
long-session stability, or open-domain language understanding. Emulator cycle
and wall-clock totals are test diagnostics, not a gameplay FPS benchmark.

The suite is a required gate for its candidate; a committed script alone is not
a passing native result. Inspect the resulting production-ROM screenshots and
the exact-head CI report before recording this milestone as verified.
