# Baby AI — Active day/night needs simulation

## Rationale

The existing END DAY button advanced a counter but did not change ordinary needs during uneventful days. A player could skip ahead without ever feeding or resting the baby.

## Rules (same saved Game state)

| Stat | Ordinary daily change |
| --- | --- |
| Hunger | +8, saturates at 100 |
| Energy | −6, floors at 0 |
| Mood | −2, floors at 0 |
| Chaos | +2, saturates at 100 |
| Corruption | No passive change |

Apply changes after ending resolution and the original ending recovery. An overnight threshold can trigger an ending only on the NEXT end-day review. Normal days show: NEXT DAY: +8 HUNGER, -6 ENERGY.

## Integration and limits

- No change to binary save Game structure, persistent format, visuals, memory allocations, ROM texture resources or per-frame rendering.
- One deterministic saturating integer update per END DAY. The uint32 day counter saturates rather than wrapping to invalid day zero.
- The existing care guidance in PR #57 reflects the actual changed needs without automatically choosing any action.
- Host tests simulate six unattended normal days, a full 10,201-state hunger/energy boundary matrix, a delayed bottle ending, real FEED and NAP recovery, and 32-bit day counter saturation.
- A dedicated 360-day long-term rehabilitation playthrough uses only real FEED/CHAT/NAP actions chosen from the visible care hint, verifies that normal needs can always be recovered, and performs recurring two-slot save/load round trips. The pre-existing 25,000-transition stress test remains enabled.
- Require strict host tests, both original DS build workflows, DeSmuME native input checks and saved-game compatibility before merge. No physical DS FPS measurements yet.

**Dependency:** stacked on PR #57, the on-screen care guidance.
## Failed-write visibility

A mounted save device may stop accepting writes. The original game discarded this error silently; now every `persist()` call reports `SAVE WRITE FAILED. PROGRESS IN RAM!` via the current lower-screen message if a write fails. The game continues in RAM and the next meaningful action retries the two-slot write. A strict host test ensures an invalid save target does not modify the in-memory sequence. The save file format is unchanged.

## Repeatable glitch recovery

A genuine quiet crib rest in Room Tour (moonlight selected and lamp switched off) now restores +8 energy, lowers chaos by 4 **and reduces corruption by 2**. This is intentional long-term rehabilitation: without a repeatable way to remove glitches, ordinary chat and arcade mishaps could permanently saturate a long-lived save. It never erases prior endings or resets achievements; daytime/lamp-on crib care retains only normal NAP benefits. The updated Room Tour message uses `QUIET: +8 ENERGY -4 CHAOS -2 GLITCH`, which fits its 36-character lower-screen text cap. Host tests cover both gating conditions, saturation at zero, recovery from 100 corruption, and preservation of discovered endings.
