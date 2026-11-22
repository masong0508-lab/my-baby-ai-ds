# Native completion coverage for the starter arcade

`tests/emulator_arcade_motion.py` completes Bottle Balance, Crib Kart,
Naptime Negotiator and Rattle Rescue in the actual production ROM under
DeSmuME. Decisions come from the rendered balance marker, traffic cone,
timing window or falling rattle. Native stylus and keypad events perform
the actions; no emulated RAM, save edits, random-sequence prediction or
debug unlocks are used.

## Coverage and evidence

| Cabinet | Required native behavior |
| --- | --- |
| Bottle Balance | Holding the stylus in one direction causes three spills and a loss without a reward. Restart clears mistakes. The mint band matches the actual stable interval. Leaving it visibly reduces STEADY progress; steering back with real keypad and stylus controls completes all 150 steady units and wins. |
| Crib Kart | Steering into the displayed cone causes a collision and a playable retry. Steering itself gives no score. Avoiding six subsequent cones using both stylus and D-pad controls wins. |
| Naptime Negotiator | An early tap causes a wakeup. Four distinct green windows accept stylus/A-button inputs. A second input within an already-scored window adds no extra success or mistake. |
| Rattle Rescue | A correct-lane tap made too early is rejected. Letting the replacement rattle expire produces a second playable retry. Six catches made when the visible rattle face reaches the gold zone win despite both earlier mistakes. |

The complete route checks visible menu rewards after every cabinet and
finishes with **four wins, $28 and three Favors**. Progress marks and
rendered counter text must agree. The script captures the actual cues and
results, including the Bottle loss, outside-band decay, collision, timing
mistakes, and each successful cabinet return.

The separate blocking `desmume-starters` job in
`.github/workflows/blocksds-build.yml` downloads the production ROM built
and packaged in the same workflow run. Its output is preserved as the
`desmume-native-starter-evidence` artifact, including on assertion failure.
`starter-report.json` records the ROM SHA-256, screen observations,
resources, screenshots, emulator cycles, elapsed test time and exceptions.
Individual waits are bounded, with a further **240-second wall-clock
timeout** on the complete CI process.

## Relationship to the other native gates

| Starter cabinet | Native completion harness |
| --- | --- |
| Bottle Balance | `emulator_arcade_motion.py` |
| Diaper Dash | `emulator_touch.py` |
| Tax Return Teether | `emulator_arcade.py` |
| Crib Kart | `emulator_arcade_motion.py` |
| Baby Beatbox | `emulator_arcade.py` |
| Sock Market | `emulator_arcade.py` |
| Naptime Negotiator | `emulator_arcade_motion.py` |
| Rattle Rescue | `emulator_arcade_motion.py` |

Together, successful runs of these three native gates cover completion
of all eight starter cabinets. `emulator_arcade.py` additionally earns and
buys Cereal Cipher before testing its timeout recovery. The existing
native jobs remain mandatory; host tests for all sixteen mechanics are
separate evidence.

## Bottle guidance defect found during implementation

The `ae70a4f` production baseline drew a mint band at x=92..163, while
steady progress was awarded only for balance -22..22, displayed near
x=117..139. A player could therefore lose progress, and even spill, while
the marker still looked safely green. The new regression rejects that
old band, requires the corrected 23-pixel mint interval, and checks actual
progress loss outside it before a successful recovery.

Baseline ROM SHA-256:
`db7ec7ac8ede0a55f0e9e383d72655b7dc7e5143a8eb66a2f129ad595034f761`.
Kart, Nap and Rattle passed full native stages on that ROM. Bottle's
three-spill loss and clean restart also passed before the strict band
assertion correctly rejected its misleading graphics. A separately
labeled baseline gameplay exploration excluded only that known geometry
defect to verify the remaining control/recovery route. It is not a full
pass of the strict gate; the corrected production ROM must pass CI.

## Local use

```sh
python -m pip install py-desmume==0.0.9 Pillow
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  python tests/emulator_arcade_motion.py my-baby-ai-blocksds-ds-only.nds \
  --output emulator-starter-proof
```

For failure triage, `--stage bottle`, `--stage kart`, `--stage nap`, or
`--stage rattle` starts a fresh session for that cabinet. Use a separate
output directory for each stage. The CI gate always uses the default
`all` stage, and the Bottle band assertion cannot be disabled through the
CLI.

The player waits for complete bitmap text and progress before interpreting
moving graphics, since a native screenshot can land halfway through a
VRAM repaint. This is not a game-frame-rate benchmark. These results do
not establish all sixteen cabinets' native completion, DraStic behavior,
physical DS performance, persistent save recovery or long-session
stability.
