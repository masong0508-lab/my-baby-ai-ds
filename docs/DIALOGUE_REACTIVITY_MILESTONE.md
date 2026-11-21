# Baby AI — six genuinely reachable dialogue intents

## Problem verified in the previous sanitizer build

The scripted dialogue census used 120 representative CHAT interactions, twenty for each intended mood. Despite having six defined output neurons and 48 original responses, the preceding classifier returned only classes 0 and 3 (94 generic / 26 playful). Classes Hunger, Sleep, Glitch and Chaos never won that realistic test scenario.

## Correction

Retain the existing 6-input, 8-hidden, 6-output integer network and all 48 ROM-resident dialogue lines. Add tiny **bounded final-score priors** for severe saved needs, after the neural weighted scores:

- Hunger ≥65: a rising hunger score, including when a real FEED just lowered extreme hunger to 69.
- Energy ≤40: rising sleep score; a dangerously tired baby still talks about sleep after beginning to nap.
- Corruption ≥65: more recognizable glitch-oriented replies, instead of generic playtime humor.
- Chaos ≥65: more recognizable rebellious/chaotic replies, even if mood temporarily remains high.
- CHAT with no urgent need: a small neutral-conversation bonus so talking to a healthy baby does not always produce playtime lyrics.

Care actions still have their original explicit action bonuses. All scores use bounded integer arithmetic and existing Game fields: no additional RAM, heap, textures, networking, save changes, RNG calls or per-frame renderer work.

## Acceptance

- Each of the six archetypal state groups now classifies into its own intent for 20 ordinary CHAT inputs (120 total).
- Six additional tests call real `game_care` first, then assert the dialogue classification matches the state *after* FEED, NAP, PLAY or CHAT.
- Existing 512-utterance test retains deterministic replies and confirms all eight lines can be reached for one selected intent.
- Require strict host suite, sanitizer job (once PR #61 lands), both original DS ROM builds and real DeSmuME controls before merging.

The game remains an offline fixed-weight classifier with authored dialogue, **not a generative language model**.
## Sanitizer gate now active

PR #61 is merged into main, so this branch is rebased on the version that runs the exact same C gameplay and dialogue tests under ASan and UBSan. The 120-case reachability check is a required part of that runner's test inventory, in addition to both normal ROM builds and the native DeSmuME walkthrough.
