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
- Host tests simulate six unattended normal days, verify a delayed bottle ending, real FEED and NAP recovery, 32-bit day counter saturation, and continued 25,000-step gameplay/save stress.
- Require strict host tests, both original DS build workflows, DeSmuME native input checks and saved-game compatibility before merge. No physical DS FPS measurements yet.

**Dependency:** stacked on PR #57, the on-screen care guidance.
## Failed-write visibility

A mounted save device may stop accepting writes. The original game discarded this error silently; now every `persist()` call reports `SAVE WRITE FAILED. PROGRESS IN RAM!` via the current lower-screen message if a write fails. The game continues in RAM and the next meaningful action retries the two-slot write. A strict host test ensures an invalid save target does not modify the in-memory sequence. The save file format is unchanged.
