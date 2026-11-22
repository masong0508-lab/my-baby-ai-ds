# Saved conversational learning

The baby keeps learned **wording preferences** across saves. Its neural
adapter selects warm, playful or corporate phrasing. This does not store a
transcript, invent earlier events, or change the baby's hunger or personality
stats. Current-day conversation facts remain transient.

## Progress stays in version 1

The existing `Game`, its version-1 record, and `save_load` / `save_write`
remain unchanged. Existing babies, unlocked cabinets, currency and ending
records continue to load. Learning is an optional companion:

| File | Contents |
| --- | --- |
| `my_baby_ai.0`, `my_baby_ai.1` | Existing alternating v1 progress records |
| `my_baby_ai.mind.0`, `my_baby_ai.mind.1` | Alternating learned-preference records, selected by sequence parity |

Each learning record is 60 bytes: a 16-byte identity/version/size/binding
header, the 40-byte `ConversationProfile`, and a four-byte integrity digest.
The profile contains 36 signed adapter weights plus two saturated feedback
counters. The serializer validates the declared weight bounds before saving
or accepting a record.

## A profile belongs to one saved baby

The companion records the **successful progress save's sequence and a digest
of its entire Game**. Loading checks both, the record checksum, version,
length, exact EOF and valid profile values. Merely matching a sequence cannot
attach the preferences of a different or changed baby.

If the progress loader recovers an older intact slot, its matching learning
slot can restore the older preferences. If the learning file is missing,
truncated, corrupt, unsupported or belongs to another Game, progress still
loads and preferences start neutral. The loader leaves its output unchanged
on failure. Old software can still read and write the unchanged progress
files; a stale companion is ignored after it changes the saved Game.

## Write and reset behavior

1. Save the ordinary Game first using the existing checked two-slot writer.
2. Only after success, write the learning companion bound to that new Game.
3. If the progress save fails, keep the progress warning visible and do not
   write a companion. If only learning fails, keep the learned model in RAM
   and show `LEARNING NOT SAVED. KEPT IN RAM!`.
4. A later successful save clears the corresponding warnings. Questions,
   topic selection and ordinary follow-ups do not touch storage. Explicit
   feedback saves only if it changes the profile; care already saves progress.

This is not an atomic multi-file transaction. A power loss after the progress
write but before its companion can restore progress with neutral learning.
It cannot cause a learning error to invalidate an otherwise valid Game.
Real flashcart interruption behavior still requires device verification.

Boot remains RAM-only until the player explicitly enables saving using the
existing title SAVE/LOAD or Room SAVE control. A successful load starts fresh
short-term facts and restores a valid learned profile. A new day resets
short-term facts but retains learned preferences. The existing separately
confirmed new-baby reset erases only this game's two progress and two learning
files and restores the neutral model.

## Verification

`tests/test_conversation_save.c` exercises old v1-only saves, exact Game
binding, both learning slots, damaged-progress fallback to matching older
learning, bad checksums, truncated/oversized records, unsupported versions,
invalid weights even with a valid checksum, sequence rollover through zero,
scoped erasure and host I/O failure. A `/dev/full` injection checks a real
host write/flush failure without changing successfully saved progress.

Native DeSmuME conversation tests exercise RAM learning and save-control
availability. They do not establish real FAT writes, flashcart reboot
persistence or injected native write-failure recovery.
