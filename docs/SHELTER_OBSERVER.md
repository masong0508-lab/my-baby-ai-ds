# The Last Nursery: WARDEN, learned phrases and escalating hostility

The player shelters an unregistered baby found outside a shuttered clinic
after the Continuity Bureau's Cradle Ban. WARDEN is a salvaged nursery
computer monitoring this hidden room. Its archive is limited: it knows the
ban exists but cannot verify the Bureau's classified rationale or count
the surviving babies. The existing toy-like shell and alien egg reveal
belong to this same nursery story.

`observer.c` implements the bounded observer state, factual typed replies,
player-taught phrases and telegraphed attacks. It never writes `Game`,
advances the shared gameplay RNG, or invokes a care action. The original
conversation style adapter remains independent.

## Typed conversation

The UI supplies a role, `OBSERVER_MONITOR` or `OBSERVER_BABY`, and a short
ASCII input to `observer_answer`. Replies live in the session's 160-byte
buffer and use at most 144 source characters. WARDEN speaks as a computer;
the baby's needs and care memories use first-person wording, with explicit
WARDEN readings for numeric facts.

Supported paths include:

| Input | Source of the answer |
| --- | --- |
| STATUS / REPORT | Current stage, actual day, observed care counters, caretaker integrity and hits |
| NEEDS / HOW ARE YOU | Live Game needs using the existing care priority, then quiet-rest guidance |
| MEMORY / REMEMBER | The last recorded event, its actual day and real effect |
| ATTACKS / INTEGRITY | Recorded windups, landed hits, blocks and remaining bracer integrity |
| WORLD / BAN / CLINIC | Original authored lore, with archive limitations stated |
| SLAP / KICK / TOYBOX | Successful recorded toy inputs and counter guidance |
| EGG / SHELL | Unconfirmed mystery until the actual egg record exists |
| CALC 12+7 | A bounded integer calculation, never generated arithmetic |
| HELLO / PLAY / DREAM / SELF | The separate integer next-word model, with make-believe/dream labels where appropriate |
| MORE | Continue the current known category; needs and memories are rechecked |

The calculator accepts one `+`, `-`, `*` or `/` operation on integers from
−9,999 to +9,999. Division reports a remainder where needed. Division by
zero, extra operations and oversized values are rejected. It does not
evaluate code or general expressions.

Generated fiction is restricted to greeting, self, play and dream topics.
It uses the separately owned `narrative_lm_generate` module and a local
variation seed. Factual counters, care advice, history, arithmetic and
known lore bypass the generator. Unsupported questions receive an honest
unknown response. This is a limited offline game conversation system,
not unrestricted factual question answering or an internet service.

## Teaching and actual reuse

Select BABY and type `TEACH TINY BOSS`. Typed TEACH addressed to WARDEN
directs the player to select BABY. The same teaching API can be called by
the explicit UI lesson action.

Eight phrase slots each store at most **24 characters**. Input is uppercased,
leading/trailing whitespace is removed and interior whitespace is collapsed.
Letters, digits, spaces and the terminal's available punctuation are
accepted. Unsupported glyphs such as brackets, braces, backslash and
backtick are rejected so a lesson cannot silently lose characters on the
bitmap display. Stored phrases are data: teaching `REPAIR` and later
asking `SAY REPAIR` only repeats the word.

New phrases start at confidence 1. Teaching the same normalized phrase
again raises confidence up to 15 without consuming another slot. A ninth
distinct phrase is refused without changing any existing lesson. SAY, SAY
1–8, WHAT DID I TEACH, and natural aliases such as SAY WHAT I TAUGHT
retrieve the stored words. PHRASES 1 and PHRASES 2 list four slots per page and LESSON 1–8
reports the exact practice confidence. Select BABY and type FORGET 1–8 to
preview deletion; FORGET 1 YES (substitute the intended slot) explicitly
confirms the removal. New lessons reuse vacant slots; other lessons,
history and the existing save layout are retained.

Typed BABY dialogue can also recall a taught expression: for example,
after teaching TINY BOSS, ask DO YOU REMEMBER BOSS or RECALL TINY BOSS.
An exact learned phrase or a unique, non-generic word in an explicit
recall cue retrieves the lesson. Ambiguous words ask for clarification;
MORE stays on that lesson and reports its actual practice confidence.
Urgent care needs take priority. Recall does not reinforce a lesson,
change game state or execute its words as commands.

After three teachings, an actual healthy PLAY can reuse the phrase. The
first and every third play event are eligible; current care needs or high
corruption suppress this optional quote. `observer_record_care` exposes
the result in `session.reply` with `session.echo_ready=1`. The application
may present that text after the real care action. Other care records clear
the flag. Questions alone do not create a PLAY event or train a phrase.

## Progression and actual attacks

Exposure advances only through recorded gameplay:

| Recorded action | Exposure |
| --- | ---: |
| FEED, CHAT or NAP | 1 |
| PLAY | 2 |
| Completed arcade win or loss | 3 |
| Successful quiet crib rest, after NAP | 1 additional |
| Actual day advance | 6 per day |
| Successful Toybox SLAP | 2 |
| Successful Toybox KICK | 3 |

Stages are **FOUND**, **WATCHING** at 24, **HOSTILE** at 64 and **REVEALED**
at 120. Counters and exposure saturate. Idling, menu navigation, typed
questions, generated stories, teaching and attacks themselves do not
increase exposure. A real `END_EGG` also establishes REVEALED: the public
REVEAL event accepts only that actual Game flag and records it once. If
an older v1 save already has the egg flag but no observer companion, reset
inherits that known reveal without inventing a dated event or care count.

The root loop calls `observer_tick` with `active=1` only in CARE or TOYBOX.
At HOSTILE/REVEALED, a **240-active-tick cooldown** precedes a visible
**60-active-tick windup**. Both pause in other views. Guard during the
windup blocks the attack. A successful actual slap or kick also counters
an active windup; its ordinary Toybox physics still occur separately.

A landed strike costs 8 bracer integrity at HOSTILE or 12 at REVEALED,
clamped to the amount remaining. This separate caretaker value starts at
100 and never reduces baby care meters. At zero, attacks stop. WARDEN's
explicit REPAIR command restores 100 and restarts the cooldown without
erasing history. Merely asking about repair, or quoting a taught phrase
containing the word, does not execute it. All attack/counter events have
their own recorded counts and last-event entry.

## Size and persistence

`ObserverProfile` is **256 bytes**. It contains bounded counts, exposure,
bracer integrity, the last observed event, known-reveal state, and eight
phrases with confidence. `ObserverSession` is **192 bytes**, including the
160-byte reply buffer, timers, focus, duplicate-event guards and local
variation state. The engine uses no heap or per-frame generation.

`observer_save.c` writes an optional **276-byte**, version-2 record:
16 bytes of magic/version/size and exact-Game binding, the 256-byte profile,
and a four-byte integrity digest. The selected path is
`BASE.watch.0` or `BASE.watch.1`, based on the successfully saved Game's
sequence parity. Both sequence and the full-Game digest must match. Valid 172-byte version-1 records are upgraded in memory, preserving the first four lessons and all history; the four new slots start empty.

Integration order is ordinary `save_write` first, then optional `.mind`
and `.watch` writers independently. No v1 Game fields or bytes change.
A missing, truncated, oversized, corrupt, invalid or mismatched `.watch`
record leaves the supplied output unchanged; the application can retain a
neutral profile and still restore normal progress. An older ordinary save
fallback finds its correspondingly bound older observer profile. Scoped
erase removes only the two `.watch` records.

This is not an atomic three-file transaction. Power loss between ordinary
progress and companion writes may restore progress without the newest
phrases/history. The application must expose an observer write failure and
keep the current profile in RAM. Actual flashcart writes and power-cycle
recovery still need native-device verification.

## Verification scope

Focused strict host tests and ASan/UBSan cover factual replies, exact care
effects, no idle/query progression, stage thresholds, paused timers,
windup timing, guard/slap/kick counters, damage/zero/repair behavior,
phrase normalization/reinforcement/full-library refusal/explicit deletion/reuse, command-as-data,
integer calculation bounds, counter saturation and unchanged Game/RNG.
The local sanitizer run disables leak detection because LeakSanitizer
cannot operate under the workspace's ptrace environment; the engine itself
allocates no heap.

Real host file tests cover exact phrase/history restoration, old v1-only
progress, full-Game binding, malformed headers/profiles/checksums, exact
EOF, older-slot recovery, `/dev/full` write/flush failure, sequence rollover
and scoped erasure. Native UI typing, visible attack/guard behavior and
screenshots remain application-level acceptance gates. These checks do not
establish physical DS performance, total RAM usage or flashcart reliability.
