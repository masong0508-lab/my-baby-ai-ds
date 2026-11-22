# Neural conversation and learned response style

Baby AI now has a contextual conversation engine with **180 original authored
responses**, a small neural output head that learns from explicit feedback,
and a direct connection to actual care outcomes. The existing 48 ordinary
dialogue lines and twelve recent-memory lines remain available through their
original APIs.

The new responses cover current feelings, useful care requests, seven kinds
of real recent events, full/already-rested care, tired play, repeated actions,
fulfilled requests, imaginative topics, and conversational follow-ups. Each
topic has warm, playful and corporate voices with distinct initial and
continuation text. All 180 entries currently fit two explicit rows of at
most 36 characters each. They can therefore appear in both the nursery's
care panel and the larger TALK panel without truncation or copying text.

## What the network actually does

The legacy dialogue network remains a fixed, hand-tuned **6 → 8 → 6** integer
classifier. Its six inputs are the five current need/mood meters and the care
action. `dialogue_encode` now exposes the same eight clipped ReLU activations
through a pure function; its original weights, intent scores, response API
and RNG behavior are unchanged.

The conversation engine reuses those eight hidden activations and adds four
bounded context inputs: a constant bias, presence of a recent fact, repeated
care count, and whether this is a follow-up. Its **12 → 3 style head** scores
the warm, playful and corporate voices:

```text
style_score = sum((base_weight + learned_adapter) * feature)
```

The shared encoder uses 48 integer multiply-adds; the new head uses another
36. Conversation inference uses no floating point, softmax, heap, networking,
token generation or per-frame background processing. The output head learns
which authored voice to favor in the current context. Factual eligibility
and care priority are checked separately, so learned preferences cannot
invent an arcade result or make an urgent need disappear.

This is on-device adaptation of small neural output weights. It is not
language-model training: all words are authored ROM text, and the base
encoder remains fixed.

## Explicit feedback and continuity

The six prompt IDs are stable:

| ID | Engine prompt | Behavior |
| --- | --- | --- |
| 0 | `CONVERSATION_HOW` | Describe current feelings or a very recent useful event |
| 1 | `CONVERSATION_NEED` | Explain the current care priority, or say nothing is urgent |
| 2 | `CONVERSATION_REMEMBER` | Recall a recorded recent fact, or honestly say none is available |
| 3 | `CONVERSATION_MORE` | Continue the substantive topic while checking that its facts are still valid |
| 4 | `CONVERSATION_LIKE` | Reinforce the voice of the answer actually being rated |
| 5 | `CONVERSATION_ELSE` | Request a new subject with a small, deliberately weak skip adjustment |

LIKE updates the selected voice's weights by one signed step for each active
feature of the actual rated answer. The engine saves that answer's twelve
features; it does not recompute credit from a different state when feedback
arrives. Every weight is clamped to **−12 through +12**, and feedback counters
saturate at `UINT16_MAX`.

NEW SUBJECT is weaker evidence than a positive rating. It reduces only the
previous voice's bias weight by one step. It does not treat a factual care
request as incorrect. When needs are urgent, the new wording still explains
the same urgent need; otherwise it opens a clearly imaginative topic.

An acknowledgement does not become a new trainable answer. Repeated LIKE
taps cannot repeatedly reinforce the same answer, overwrite its saved
features, or replace its substantive topic. For example, a real
FEED → REMEMBER → MORE → LIKE → MORE sequence remains about the feeding, with
a different continuation after the acknowledgement. A new substantive answer
can then receive its own feedback.

An eight-ID recent-response ring discourages repetition. Exact consecutive
outputs are excluded, and MORE also excludes the previous substantive answer
even if an acknowledgement appeared between them. This supports continuity
without repeatedly printing the line the player just liked.

## Care facts and current needs

The application records actual before/after care state:

- FEED records the hunger that was really removed. A zero-effect feed gets
  the already-full topic.
- CHAT records the actual nursery chat and its mood change. Its immediate
  response can continue using the existing legacy dialogue path.
- PLAY records its real energy cost. A tired baby can describe that cost;
  an urgent rest need takes priority over playful praise.
- NAP records the energy that was really restored. A capped-energy nap does
  not claim to have added energy. The text does not deny its separate calming
  effect.

Explicitly requested care can receive a specific thank-you when the matching
action actually helps and resolves that request's threshold. The pending
request is then consumed: later unnecessary care cannot repeatedly claim to
have fulfilled the same request. A partial feed or nap that leaves an urgent
need unresolved continues to ask for the needed care.

Need priority follows `game_care_hint`, keeping speech and the nursery hint
coherent: critical exhaustion first, hunger at least 70 next, then energy at
most 35 or chaos at least 70, then mood at most 40. If none applies,
corruption at least 60 asks for quiet crib rest. The quiet-rest advice names
the actual moonlight, lamp-off and crib interaction. Mild hunger at 65–69
may be described as a feeling without contradicting a non-urgent hint.

Completed arcade wins/losses and successful quiet rest are recorded only by
their actual gameplay paths. Old cumulative win totals cannot create a
recent story. Quiet rest upgrades its fresh NAP without aging other facts a
second time.

The session stores four distinct recent facts. Each records its event kind,
age, detail and care effect. Facts expire after twelve subsequent care/event
or conversation interactions, or on a day change. The immediate care selector
does not age a freshly recorded action again. MORE checks the live facts
before continuing; expired facts cannot sustain a remembered event.

## Memory, persistence and randomness

`ConversationProfile` is exactly **40 bytes**: 36 signed adapter weights and
two unsigned feedback counters. `conversation_profile_valid` rejects a null
profile or any weight outside the declared bounds; all `uint16_t` counter
values are valid. Reset produces a deterministic neutral profile.

`ConversationSession` is **68 bytes**. It holds recent facts, response IDs,
the substantive topic, rating credit, pending care request, and a local
variation state. It is not saved. A new baby or successful load resets the
session, including a load of another save on the same day. Day changes clear
session facts while preserving the separate learned profile.

The engine never writes `Game`. TALK prompts do not apply another care
action, drain energy, increase corruption, or advance the shared RNG. The
ordinary `game_care` call still advances that RNG exactly once; subsequent
conversation selection reads the resulting Game. A local, explicitly owned
variation state supplies bounded wording variety without changing future
arcade randomness.

The profile is designed for the application's separate validated companion
save. The engine itself opens no files, and adding its profile does not
change the v1 Game record. Missing or rejected learning data can be replaced
by a neutral profile while retaining ordinary gameplay progress.

## Integration

```c
ConversationProfile profile;
ConversationSession conversation;
conversation_profile_reset(&profile);
conversation_session_reset(&conversation, baby.days, baby.rng);
```

Record care after the existing effects and legacy dialogue selection:

```c
Game before = baby;
const char *message = game_care(&baby, action);
message = dialogue_contextual_reply(&memories, &baby, action, message);
conversation_record_care(&conversation, &before, &baby, action);
const ConversationReply *reply = conversation_care_response(
    &profile, &conversation, &baby, action);
if (reply) message = reply->text;
```

`conversation_care_response` returns a new reply only for a just-recorded
FEED, PLAY or NAP. It returns null for CHAT, retaining the original immediate
CHAT and twelve-memory-line behavior. The new TALK view uses
`conversation_respond` for all six prompts and can retain the returned ROM
text pointer directly. `ConversationReply` also carries a stable ID, topic
and style for diagnostics and content tools.

Call `conversation_record_event` for actual completed arcade outcomes or a
successful quiet rest. It deliberately rejects care event IDs: before/after
care belongs in `conversation_record_care`, preventing an event-only caller
from fabricating a useful feeding or nap delta.

## Verification

The focused conversation test verifies all 180 entries are unique, printable,
bounded, and selected through the public prompt/care/event APIs in relevant
scenarios with valid style profiles. It also verifies the original encoder's
bounded, deterministic activations and unchanged Game state.

A separate learning experiment starts with neutral weights, rates only
answers actually selected by inference, and proves that the learned profile
changes the style selected in a fresh session. It does not install the desired
weights directly. Copying that 40-byte profile then produces identical
responses and updates over a 2,000-prompt replay.

Other tests cover actual care deltas, already-full/rested responses, tired
play, repeated care, fulfilled-request consumption, multiple simultaneous
urgent needs, current-hint boundary agreement, real arcade results, quiet
rest coalescing, empty/expired memories, same-day load isolation, day resets,
stored feature credit, weaker skip updates, duplicate-LIKE suppression and
MORE continuity across acknowledgement. Every reply helper checks that the
Game and shared RNG remain unchanged.

Dedicated strict C11 and ASan/UBSan conversation tests pass. The existing
dialogue classifier, care-reactivity, RNG and recent-memory suite also passes
unchanged after the encoder extraction. Native TALK and persistence tests
belong to the application integration milestone.
