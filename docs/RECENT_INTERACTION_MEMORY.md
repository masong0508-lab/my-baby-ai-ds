# Recent interaction memory

Baby AI can now mention something the player just did. A healthy CHAT can
recall feeding, playing, a nap, a completed arcade win or loss, or a successful
quiet crib rest. These are twelve authored lines selected from recorded
events; the baby does not create a transcript or generate language.

For example, the first comfortable CHAT after FEED can say:

- `THAT BOTTLE HIT THE SPOT. THANK YOU.`
- `THE LAST BOTTLE EARNED FIVE STARS.`

Each new line fits the nursery's 36-character first speech row. The existing
48 dialogue lines and fixed integer intent classifier remain available.

## Behavior

The caller owns one `DialogueMemory` per live game session. It occupies
20 bytes on the host and DS ABIs and stores at most four distinct event kinds,
in recency order, plus the last selected authored reply. No heap allocation,
frame buffer, per-frame timer, global memory, or new `Game` field is needed.

- FEED, PLAY and NAP are recorded when their ordinary care reply is processed.
- Completed arcade results and conditional quiet rest are recorded explicitly
  by their successful gameplay paths. Old cumulative wins or losses cannot
  create a recent event.
- Repeating an action refreshes its existing event. Quiet crib rest replaces
  the corresponding generic nap, so one crib visit does not produce two nap
  anecdotes or age older memories twice.
- Comfortable CHAT recalls the newest event that has not already been spoken.
  Each occurrence is mentioned at most once, with two text variants per kind.
- Events expire after eight subsequent care or recorded-event interactions.
  The count advances on inputs, not rendered frames. A day change also clears
  all memory before recording or selecting the next reply.
- Hunger at least 65, energy at most 40, mood at most 40, corruption at least
  60, or chaos at least 65 suppresses recollection. The ordinary need-sensitive
  reply retains priority. Recall additionally requires a neutral or playful
  classifier intent.
- If ordinary dialogue chooses the preceding authored line again, another
  variant from the same intent is selected. Need classification is preserved.

The memory is intentionally session-only. Loading even a same-day save or
starting a new baby requires an explicit reset. Save bytes, save version,
the existing `dialogue_reply` API, and the shared RNG sequence are unchanged.

## Integration contract

Initialize after the Game is initialized or loaded, and reset after every
successful load and confirmed new-baby reset:

```c
DialogueMemory memory;
dialogue_memory_reset(&memory, baby.days);
```

Pass the original care response through the contextual selector exactly once:

```c
const char *message = game_care(&baby, action);
message = dialogue_contextual_reply(&memory, &baby, action, message);
```

`game_care` still owns all care effects and the single RNG advancement.
The selector reads the resulting Game without modifying it or advancing RNG.
It automatically records FEED, PLAY and NAP; callers must not record those
care actions a second time. A null memory retains the original response.

Record an external event only after it actually completes:

```c
dialogue_memory_event(&memory, DIALOGUE_EVENT_ARCADE_WIN, baby.days);
dialogue_memory_event(&memory, DIALOGUE_EVENT_ARCADE_LOSS, baby.days);
dialogue_memory_event(&memory, DIALOGUE_EVENT_QUIET_REST, baby.days);
```

These calls illustrate the three event types, not a sequence to run together.
Arcade cancellation records no outcome. Record quiet rest only when
`game_room_quiet_rest` returns success after NAP. Callers may explicitly reset
on END DAY as well; both contextual selection and event recording independently
detect a changed `baby.days` to prevent stale references.

## Verification

`tests/test_dialogue.c` covers an actual FEED → PLAY → NAP → CHAT sequence,
newest-first recall, once-per-event delivery, repeated-action coalescing,
four-event capacity, quiet-rest coalescing, all twelve bounded printable lines,
need priority, expiry while needs are urgent, day changes, separate sessions,
and the reset required after a same-day load.

Two 1,024-interaction checks exercise deterministic selection and prevent
immediate ordinary-line repetition. Game snapshots and comparison with the
original care path verify that contextual dialogue changes neither gameplay
state nor the shared RNG trajectory. The original six-intent census and
eight-variant dialogue checks remain in the same test executable.
