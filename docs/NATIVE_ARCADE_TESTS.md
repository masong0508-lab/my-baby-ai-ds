# Native arcade regression coverage

`tests/emulator_arcade.py` plays the production DS ROM in DeSmuME using
native keypad and stylus events. It reads the actual 256×384 screenshot,
including the game's bitmap text, to choose actions and verify results.
It never reads or writes emulated game memory, predicts random answers,
edits a save, or enables a debug cabinet.

The `desmume-arcade` job in `.github/workflows/blocksds-build.yml` is a
separate blocking gate after DS-only packaging. It uses the production ROM
from that same workflow run and preserves screenshots plus
`arcade-report.json` in `desmume-native-arcade-evidence`, including on test
failure. The existing native touch job remains unchanged.

## Scenarios

| Cabinet or flow | Native assertion |
| --- | --- |
| Sock Market loss | Three intentionally mismatched stripe choices return to the arcade without a win or currency reward. |
| Sock Market recovery | Restart clears counters; a real timeout produces another playable matching puzzle; six correct answers win. Both stylus lanes and D-pad controls are used. |
| Baby Beatbox | The visible WATCH cue, sequence-position lights, and lit instrument pads determine each tune, including repeated notes. A WATCH tap is ignored; a wrong REPEAT note restarts WATCH; three correctly replayed tunes win. Stylus and D-pad controls both work. |
| Tax Return Teether | A drag starting outside the receipt and a drop outside both trays cause no penalty. A wrong bin starts a retry. Six correct receipt drags win. The held receipt remains visible at its destination and scores only after release. |
| Earned purchase | Three more Sock wins bring the total to six. The displayed Cereal requirement is checked; A cannot start the locked cabinet; X buys it for exactly $14 and two Favors. |
| Cereal Cipher recovery | After two real timeouts, each replacement question still shows a valid sum and exactly one correct answer. Solving six questions with native stylus taps wins despite both earlier mistakes. |
| Economy | Visible menu text confirms every win, $5 reward, alternate-win Favor, and purchase deduction. The complete route finishes at seven wins, $29, and two Favors. |

Progress rules and exact rendered counter text must agree. The test waits
for complete bitmap frames before reading transient WATCH/REPEAT state;
it does not treat a screenshot captured halfway through a VRAM repaint as
a finished puzzle. All waits have bounded emulator-cycle budgets, and the
CI process has an additional 240-second wall-clock timeout.

The held Teether drag also waits, with the stylus still down, for the
title, status, progress counters, cue and both tray colors to be readable.
This wait accepts any internally consistent score. The unchanged-score
and moved-receipt assertions run afterward, so the wait cannot accept
early scoring or hide a receipt drawn underneath the tray. Its extra
cycles and before/held counters are preserved in the screenshot event.

## Regression found by this test

The `b13aab7` production baseline failed the Teether held-receipt visibility
assertion: zero pixels changed in the receipt's destination rectangle.
The moving receipt was drawn before both trays, so their artwork painted
over it when the stylus reached a valid drop target. Scoring still worked.
Drawing the held receipt after the trays and labels fixes that ordering;
the native regression requires more than 50 changed pixels while the
stylus is still held, with progress unchanged until release.

Baseline ROM SHA-256:
`b612c11a34c31bdea2616756f59f4fa11e964a2c712c9d34b5d067bfead7a34d`.
On that baseline, Sock loss/restart/timeout recovery and Beatbox recovery
completed. A separate native exploration confirmed six Teether sorts and
the earned Cereal purchase/two-timeout recovery. Those partial checks do
not replace a full green run of the strict harness on the corrected ROM.

The Last Nursery candidate `5d652724` exposed a separate capture-timing
assumption in that held-receipt check. On the exact production ROM below,
both CI and a local replay reached cycle 2226 while the title and counter
panel were blank during a repaint. Keeping the native stylus held for one
more emulator cycle restored the complete panel with zero sorts and one
mistake; the dragged receipt remained on its tray. Another repaint at
cycle 2234 completed at 2235. Waiting for the rendered cabinet, instead
of assuming the fixed hold ends on a complete paint, resolves this test
synchronization issue without changing ROM code or the scoring/visibility
assertions.

Reproduced ROM SHA-256:
`bbe3f7d57d94fc829f9176cc9d0295d0a7b57e0d5e5f0bf6730caa1a3393d007`.

The corrected harness completed the full route on that unchanged ROM in
3,749 emulator cycles. The held check needed one extra cycle, retained
zero sorts/one mistake before release, and measured 588 changed receipt
pixels. All six released receipts then scored normally, and the earned
Cereal route finished with seven wins, $29 and two Favors.

## Run locally

```sh
python -m pip install py-desmume==0.0.9 Pillow
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  python tests/emulator_arcade.py my-baby-ai-blocksds-ds-only.nds \
  --output emulator-arcade-proof
```

Use a fresh process and the exact production ROM. The script starts a
fresh RAM-only session through the normal title screen. Its JSON report
records the ROM hash, observed questions, screenshot names, resource
totals, emulator cycles, elapsed test time, and any exception.

This covers four cabinets natively. Host tests for all sixteen mechanics
remain a separate kind of evidence. It does not establish all-cabinet
native completion, DraStic behavior, physical DS performance, persistent
save recovery, or long-session stability.
