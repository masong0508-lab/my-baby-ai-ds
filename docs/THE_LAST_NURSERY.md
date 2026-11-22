# The Last Nursery — story and playable systems

## Premise

The Continuity Bureau has outlawed infancy. Its notices call newborns a
public continuity hazard; its actual evidence has disappeared behind sealed
files. You find an unregistered baby outside a shuttered clinic and take it
to a hidden nursery. It is one of the last infants you know about. You do not
yet know whether the Bureau is protecting anyone, covering something up, or
both.

WARDEN is a salvaged nursery computer. Its original job was keeping children
comfortable. It now helps keep this room hidden, watches the baby, and admits
when its records are incomplete. Its precise observations and dry office
language give the player something dependable inside an increasingly absurd
situation.

The infant learns your mannerisms and picks up phrases you teach it. The
first jokes sound like imitation. Later, the same little voice sounds as
though it has been interviewing you. Its hostile behavior eventually becomes
physical: readable wind-ups lead to attacks on the caretaker's bracer. The
existing slap/kick Toybox becomes a ridiculous countermeasure against an
unnatural shell. The shell fracture and alien-egg sequence reveal why this
particular baby was never an ordinary foundling.

## Character direction

**The caretaker** chose to shelter a forbidden child. The game should let
that choice remain understandable even after things become strange. Feeding,
rest, conversation and comfort continue to matter. Caring is not secretly a
wrong answer that invalidates progress.

**WARDEN** separates what it observed from what the Bureau claims. It can
report actual care, attacks, blocks, Toybox actions and learned phrases. It
must not pretend to have witnessed an event merely because a generated line
sounds convincing. Questions about the world draw on original shelter files;
unknown questions receive a limit or a useful suggestion.

**The baby** starts curious and awkwardly cheerful, becomes watchful, then
hostile and openly uncanny. Corporate jokes and the existing baby-game
parody remain part of its voice. Learned player phrases are quoted as learned
phrases, not presented as commands the computer executed or memories of
events that never occurred.

## Current playable progression

The observation profile counts actual completed interactions. Looking at
menus or sending repeated questions does not advance the threat stage.
Exposure thresholds identify FOUND, WATCHING, HOSTILE and REVEALED. These
are local progression labels, not a claim that the baby has biologically
aged or that every story mystery has been solved.

Hostility has a cooldown and a visible wind-up. The caretaker can press
RIGHT or tap GUARD. A successful Toybox slap or kick during a wind-up also
counters it and uses the existing bounded ragdoll physics. Attacks pause
while reading, typing, playing an arcade cabinet or watching a film, and
while the shell is tumbling. A confirmed hit reduces a separate bracer
integrity value. WARDEN's REPAIR command restores the bracer. Ordinary baby
care meters, currency and arcade RNG are not fabricated by the dialogue
layer.

The monitor is reached through CPU or UP in care. The typed terminal has
two targets: WARDEN and BABY. STATUS, MEMORY, WORLD, HELP, needs questions,
simple calculations and taught-phrase recall have explicit factual paths.
Greeting, play, dream and personality dialogue can use the small local word
model. These generated fictional responses are not allowed to perform care,
spend money, damage the player, write arbitrary files or invent observations.

## Persistent learning

The existing learned response-style adapter remains. A separate observation
profile records the shelter's actual history, bracer state and four short
taught phrases with confidence. Repetition strengthens phrase familiarity;
appropriate later play can elicit a learned quote. The profile has its own
validated companion save and leaves the existing version-1 Game unchanged.
This is bounded game memory, not an unlimited chat transcript or online
retraining of the whole word model.

## Visual direction

Keep the cheerful, artificial DS nursery recognizable. Stronger shading,
ground contact and coherent facial surfaces improve readability before the
unease arrives. Hostile acting uses the same connected character rig: an
anticipatory pose, narrowed eyes and a finite forward lunge. It does not
depend on a different unrelated monster sprite, fullscreen flashes or an
unbounded particle system. The already-authored egg reveal remains a distinct
climactic sequence.

## Further story work

The present milestone establishes the premise, monitor, typed interaction,
learning and actual attack/counter loop. Additional shelter locations,
Bureau visits, multiple foundlings, a complete campaign and alternative
resolutions remain future work. Future evidence should deepen the mystery
without rewriting established observations or making the existing caring
and continue-playing paths unusable.
