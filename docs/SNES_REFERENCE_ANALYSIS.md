# Supplied SNES conversation references

The user supplied `ChatGPT_for_SNES_v0.7_pure.zip` and
`ChatGPT_for_SNES_v0.7_fact_book.zip` as capability references. These are
compiled projects, not source distributions. Neither ROM was executed during
the initial inspection.

## What was inspected directly

The pure archive contains one 2,097,152-byte SFC. The fact-book archive
contains a different SFC of the same size and a 32,748,480-byte MSU file.
Both ROMs contain the title `CHATGPT FOR SNES`, English vocabulary and eight
groups of 1,024 token-offset entries. The MSU begins with `CGB1` and has
structured text sections; its `FB` section includes a count of 150,000.
These are binary observations, not proof of a particular training algorithm
or runtime personal-learning mechanism.

| Supplied file | SHA-256 |
| --- | --- |
| Pure SFC | `8f5ef5851d7a36c4d3115fc2e59fe37a019236c6775a6137c63dd290ccfc8c33` |
| Fact-book SFC | `6bd57a938122d019e5b987ad69e3eb845dc11483614d512a6c1c6cdddd6e1e6a` |
| Fact-book MSU | `0a0cf27bec631d5af63c53de14f65c28e72c87073f8ca3953d505b6945e998c5` |

## Author's description

Benj Edwards's [BX Foundry project page](https://www.bxfoundry.com/chatgpt_snes/)
describes v0.7 as a roughly 2.2-million-parameter language model running on
Super FX 2, producing words locally. The optional fact edition adds lookup
and calculation, using a separate collection of Wikipedia openings and game
hints. The page credits synthetic training conversations to gpt-oss-120b and
identifies the fact data as CC BY-SA 4.0. It does not establish persistent
personal memory or runtime weight training. The linked emulator sources are
not the chat program's model implementation.

## What Baby AI takes from the references

The useful architecture is a small local language component beside a
separate, reliable source of facts. Baby AI implements its own original
word-model corpus and C inference, an original in-world knowledge store,
explicit calculator and live-state queries, and persistent game-specific
phrase memory. A model-generated sentence cannot substitute for a real
observation or trigger a game consequence.

The supplied ROMs, trained weights, vocabulary tables, graphics, music and
MSU corpus are not included in this repository or redistributed in the DS
package. Baby AI's model is substantially smaller and more limited than the
reference; this is not a port of the SNES model or a claim of equivalent
general conversation ability. Its measured model footprint and actual
validation are documented in `NARRATIVE_LANGUAGE_MODEL.md`.
