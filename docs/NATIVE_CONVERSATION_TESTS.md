# Native TALK and response verification

`tests/emulator_conversation.py` exercises the actual production ROM in
DeSmuME. It opens the nursery's TALK view using native touch and keypad
events, reads the bitmap-font question/reply rows, and verifies the
resulting care meters. It does not call the host conversation API, read
emulated memory, modify a save, or predict the game's random generator.

## Required player-visible scenarios

- Opening TALK with LEFT displays the automatic HOW response. Before any
  care event, REMEMBER must acknowledge the absence of a recent care
  memory. It must not invent a bottle or another completed action.
- A real touchscreen FEED changes hunger/mood and produces an appropriate
  immediate outcome. Opening TALK through the nursery speech panel then
  asking REMEMBER recalls that actual bottle. TELL ME MORE continues the
  same subject with a distinct, appropriate response.
- I LIKE THAT acknowledges the displayed response; a second LIKE reports
  that the same answer was already rated. TELL ME MORE after these
  acknowledgements still follows the bottle topic. NEW SUBJECT then
  changes to an explicitly imagined subject. This checks the visible
  interaction; parameter updates and learning correctness belong to the
  engine's host tests.
- Holding one card keeps the same rendered answer throughout unevenly
  spaced observations and after release. No hidden turn counter is used
  as a substitute for visible evidence.
- RIGHT, DOWN, LEFT and UP move around the two-column prompt grid without
  asking a question. The ASK label previews the selected prompt while the
  existing response remains unchanged. A and the footer ASK control then
  submit the selected prompt.
- B and the footer BACK return to CARE without changing the four numeric
  meters, the glitch bar, day or currency. The complete care-meter pixel
  region must also match its pre-conversation capture.
- The original CARE controls remain intact: A feeds, B chats, X plays and
  Y naps. The legacy first FEED-to-CHAT memory reply is still recognized.
- Five real PLAY actions after the specified care sequence reduce visible
  energy to 12. The new conversation must prioritize that urgent need
  over earlier conversational preferences, including on follow-up and
  after NEW SUBJECT.
  Returning to CARE preserves the exhausted state; a real NAP then raises
  energy to 37 and lowers chaos to 67, satisfying the request and producing
  an appropriate acknowledgement. Hunger is 68 at that point.

## Reading the actual display

The header is read at native scale from the full 256×384 screenshot. The
TALK preview is `ASK: <selected prompt>` at bottom-screen coordinate
(17,36). Reply rows are at (17,48), (17,59) and (17,70), with at most 36
characters per row. The reader supports both explicit newlines and the
UI's word wrapping by retaining the three rows and comparing normalized
spaces against the engine's agreed authored response families. Unknown
glyphs and incomplete redraws are not accepted as replies.

The test hardcodes the expected semantic families for this real input
sequence, then reads each family's six authored style/variant strings
from `source/conversation.c` in the matching checkout. All authored
variants are eligible; the test does not call the engine to choose its
expectation or predict its variation generator. For example, actual FEED
must yield the bottle-memory family, while five real PLAYs require the
urgent-rest family. Once enough conversation turns have passed, HOW is
allowed to describe the current comfortable state instead of continuing
to foreground an old bottle. Replies currently occupy at most two rows;
the third row must remain empty.

The six prompt cards use their drawn hitboxes: columns x=8 and x=132,
rows y=92,119,146, with 116×24-pixel cards. Native touch selects and asks
in one action. Keypad navigation uses the same two-column, three-row grid.

The CARE meter reader checks numeric hunger, mood, energy and chaos,
the contiguous visible glitch bar, and the day/currency header. It also
hashes the complete meter pixels for an exact comparison after TALK.
These screenshots cannot establish preservation of fields that are not
shown, especially gameplay RNG. Full Game/RNG isolation is a separate
host-engine assertion and is not claimed from meter images.

## Run and evidence

```sh
python -m pip install py-desmume==0.0.9 Pillow
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  python tests/emulator_conversation.py my-baby-ai-blocksds-ds-only.nds \
  --output emulator-conversation-proof
```

Use the production ROM and matching checkout in a fresh emulator process.
The script enters through the ordinary title screen in RAM-only mode.
`conversation-report.json` records the ROM SHA-256, rendered replies,
visible state comparisons, screenshots, emulator-cycle count, elapsed
test time, and any exception. All observation waits are bounded. Failure
screens and the report are preserved for triage.

This gate verifies visible response behavior and native input integration.
It does not by itself establish neural training quality, learning-weight
changes, profile persistence, actual FAT write-failure handling, hardware
performance, or physical DS/DraStic operation. Existing care and arcade
native gates remain independent regression coverage.
