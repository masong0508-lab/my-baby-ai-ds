# Arcade question recovery after a missed timer

Cereal Cipher, Baby's First Audit and Courtroom Crayons now restart their
own question generator when the timer expires. Previously, each fell
through to the obsolete direction-game retry. That changed the visible
arithmetic or invoice values without rebuilding their answer, or retained
a partially completed evidence gesture with an unrelated timer.

- Cereal Cipher always offers exactly one bowl matching the displayed sum.
- Audit always scores APPROVE/FLAG against the numbers actually shown.
- Courtroom restarts an unfinished trace with a valid three-clue permutation.
- Existing completed rounds survive a timeout, the first two misses remain
  recoverable, and the third miss ends the session with a single loss.
- Normal and high-corruption retry timings use the existing cabinet rules,
  including a timeout that raises corruption across the difficulty threshold.
- The main loop redraws immediately when the round, mistake count or
  presentation phase changes. A replacement puzzle and the Beatbox REPEAT
  prompt no longer wait for the next throttled timer update. Unchanged
  playfields retain their existing redraw cadence.

The regression exercises all three cabinets with 16 deterministic seeds and
four starting corruption levels. It verifies two timeouts followed by a
successful completion, a partial stylus trace, three consecutive timeouts,
and exactly-once rewards/losses. The new test fails on the original code and
passes with the retry dispatch repaired. The complete minigame executable
also passes strict C compilation and AddressSanitizer/UndefinedBehaviorSanitizer
with leak detection disabled because the local executor uses ptrace. The
standard CI sanitizer job retains its full leak-detection configuration.

No persistent state, save format, framebuffer, heap allocation or geometry
changes are required. ROM builds and emulator verification are separate
integration gates; these host results do not establish original DS timing.
