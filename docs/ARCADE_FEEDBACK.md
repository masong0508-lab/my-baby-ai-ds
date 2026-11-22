# Arcade feedback

The sixteen cabinets now share short, meaningful feedback for progress,
mistakes, completed wins and completed losses. The observer reads actual
`Arcade` transitions. It does not change rules, consume RNG, infer results
from old win totals, or add anything to the saved `Game`.

## What produces feedback

`ArcadeFeedback` is a nine-byte, caller-owned session object. It stores only
the relevant previous state, a round high-water mark and a bounded display
timer. There is no heap allocation, transcript, extra frame buffer, global
session state or audio queue.

| Change | Event |
| --- | --- |
| A new completed round or Bottle steady milestone | Progress |
| A correct Baby Beatbox note or Courtroom Crayons clue | Progress |
| The first successful tape in Diaper Dash's final step | Progress |
| Increased mistake count while the game remains active | Miss |
| Active game becomes finished with outcome 1 | Win |
| Active game becomes finished with outcome 2 | Loss |

The order is final result, then mistake, then progress: a final winning
action emits one win event, and a failed CEO decision that also advances its
round emits a miss. The same observed transition cannot emit twice.

Steering, posture selection, partial wiping, individual watering frames,
ordinary animation counters, ignored inputs and Beatbox's WATCH → REPEAT
transition produce no success cue. Bottle's already-reached steady milestones
do not replay when the meter falls and recovers. Exiting to the menu clears
feedback without inventing a loss. Beginning a new game establishes a silent
baseline; observing an already-finished game does not announce an old result.

## Footer and sound

The optional `ui_arcade_feedback` pass uses the existing bottom footer. It
clears only rows 178–191 and draws one line at `(10, 181)`, with the existing
5×7 font and six-pixel character advance. It leaves the header, puzzle,
status/progress row, targets, hitboxes and footer accent rule intact. There
are no flashes or extra surfaces.

| Event | Caption | Ink | PSG duration |
| --- | --- | --- | --- |
| Progress | `NICE! KEEP GOING.` | Mint | 8 ticks |
| Miss | `OOPS! TRY AGAIN.` | Pink | 10 ticks |
| Win | `YOU WON! GREAT JOB.` | Lemon | 24 ticks |
| Loss | `ROUND LOST. TRY AGAIN.` | White | 20 ticks |

Each caption lasts 30 existing main-loop ticks unless a newer meaningful
event replaces it. Progress/miss captions appear in the active cabinet;
win/loss captions can remain on the arcade menu after automatic return.
The ordinary footer is redrawn at expiry, restoring its original controls.
Input hitboxes never change while a caption is visible.

A failed save takes priority over result feedback. The caller draws
`ui_save_warning()` last in arcade/menu views, using the same bounded footer
and pink caption `SAVE FAILED. PROGRESS IN RAM!` at `(10, 181)`. This warning
does not depend on the 30-tick result timer: the caller retains the failure
state until a successful save, load or new-baby reset. An ordinary draw then
restores the original footer. Framebuffer tests read every warning glyph,
verify the final punctuation, and check bounded writes and exact restoration.

The four original cue envelopes share the game's existing single PSG voice.
They stop explicitly and obey mute. They cannot interrupt crack, unseal,
pulse or scare sounds. Progress and miss also cannot truncate an in-flight
win/loss melody; a new intentional care cue or final result may replace it.
Rejected cues are discarded rather than queued for a later surprise sound.

## Integration contract

```c
ArcadeFeedback feedback;
arcade_feedback_reset(&feedback);

game_arcade_start(&baby, &arc, selected);
arcade_feedback_begin(&feedback, &arc);
```

Call `arcade_feedback_tick` once per existing main-loop tick. Its return
value is nonzero only on display expiry, allowing one ordinary UI repaint
instead of a full redraw every animation frame.

After the cabinet's input and simulation tick, before automatic `leave_game`,
call `arcade_feedback_observe`. A nonzero return is a new event: request a
repaint and trigger its sound once. Final events can also trigger the baby's
finite result reaction. Do not use `arcade_feedback_current` to retrigger
sound or acting; it is only the current visible caption.

Inside the normal full-draw block, call `ui_arcade_feedback` after `ui_draw`
and before drawing the virtual cursor. It is a no-op for NONE or an invalid
event. Reset feedback on manual abort, new baby, successful load and when
beginning another game. `ui_draw` keeps its existing ABI.

## Bottle Balance correction

The old mint band covered x92–163, much wider than the scoring region. The
actual inclusive safe range is balance −22 through +22. Scoring and rendering
now share `MBAI_BOTTLE_SAFE_BALANCE`, with the exact original threshold.

The mint band covers x117–139, 23 pixels centered at x128. Marker offsets
round away from zero so balance −23 and +23 center at x116 and x140, visibly
outside the band. The original STEADY readout at `(83, 115)`, marker rows
127–140, touch controls and balance simulation remain in place.

## Verification

`test_arcade_feedback.c` plays all sixteen cabinets through actual successful
and failing input/tick paths. It checks exact progress totals, recoverable
misses, once-only wins/losses, unchanged Game/Arcade snapshots, ignored inputs,
WATCH transitions, continuous counters, tape/clue substeps, Bottle milestone
recovery, session resets and exact 30-tick expiry.

`test_sfx.c` checks distinct finite arcade envelopes, hardware frequency and
volume bounds, mute, and exact frame-by-frame preservation of protected
horror/result cues when a lower-priority arcade cue is attempted.

`test_ui.c` compares actual framebuffer pixels for every cabinet and event:
everything above row 178 remains identical, out-of-bounds guards remain
intact, captions differ, and the ordinary footer returns exactly on redraw.
It also checks the complete 23-pixel Bottle band and real scoring/marker
alignment at −23, −22, −1, 0, +1, +22 and +23.

The focused feedback test is included in the existing host/sanitizer runner.
Native emulator coverage can independently read the exact footer captions
and exercise the main-loop integration without changing these module APIs.
