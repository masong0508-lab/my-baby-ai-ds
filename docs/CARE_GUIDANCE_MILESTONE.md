# Contextual care guidance and UX acceptance

The care-screen header now gives a short **non-automatic** suggestion, using the same existing Game state as the need meters. It never spends a turn, issues a care command, uses RNG, or changes persistent data.

## Recommendation logic

| Existing state | Header | Action indicator |
| --- | --- | --- |
| Energy ≤15 | TRY NAP | Small ! on NAP |
| Otherwise hunger ≥70 | TRY FEED | Small ! on FEED |
| Otherwise energy ≤35 or chaos ≥70 | TRY NAP | Small ! on NAP |
| Otherwise mood ≤40 | TRY CHAT | Small ! on CHAT |
| Otherwise | ALL GOOD | No marker |

Critical exhaustion is prioritized over hunger so a completely depleted baby cannot be directed into more strenuous play. No automatic actions are taken, and every other care card remains available for player choice. The header hint only occupies x=126..188, y=35..42 in the native 256×192 bitmap, between the BABY SAYS title and ROOM button, and never covers speech or touch hitboxes.

## Acceptance and performance

- `game_care_hint` is one deterministic pure helper with constant work, no allocations or RNG state; actual care actions and their save effects stay unchanged.
- Strict host tests assert thresholds, tie-break priorities, stable RNG/save state and correct disappearance of a hint after FEED or NAP.
- UI tests check the actual framebuffer changes under hunger, exhaustion, low mood, and the normal state, and still enforce full visible-screen coverage.
- Standard DS / BlocksDS CI, emulator stylus walkthrough and native screenshot checks remain merge gates.
- The helper's role is a **suggestion**, not a complex agent, diagnosis, tutorial modal, or automatic baby control. No new texture assets or second framebuffers.
