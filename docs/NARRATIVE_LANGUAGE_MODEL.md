# Original offline narrative word model

The shelter terminal and the baby can now use a small **autoregressive
word-level neural language model**. It predicts one next token from the last
three tokens and the supplied speaker, topic and relationship stage. The
selected token is fed back into the next prediction until the model chooses
an end token or a strict output limit is reached.

This is separate from the existing adaptive conversation-style head. The
word model's weights are trained offline and fixed during play. Gameplay
can change its speaker/topic/stage inputs; that changes its words without
training the language weights on the DS. It is a limited fictional voice,
not a general assistant, a factual reasoning system, or a port of another
project's model.

## Model and original training data

| Item | Current implementation |
| --- | --- |
| Vocabulary | 412 tokens, including punctuation and beginning/end tokens |
| Word history | Three learned 12-dimensional token embeddings |
| Context | One learned 16-dimensional embedding for each of 64 speaker/topic/stage combinations |
| Hidden layer | 52 inputs to 48 clipped ReLU units |
| Output | 48 hidden activations to 412 next-token scores |
| Trainable parameters | 28,700 |
| Immutable compiled model data | 57,487 bytes, including vocabulary, masks and sampling table |
| Generation limits | 24 tokens and 144 printable characters; smaller caller buffers are respected |
| Runtime arithmetic | int8 weights/embeddings, int16 biases/activations, int32 accumulation |
| Allocation | Bounded stack storage; no heap, file access, network or shared RNG |

`tools/narrative_lm_corpus.tsv` contains **256 original utterances** written
for My Baby A.I. They concern the hidden nursery, a forbidden child, WARDEN,
the city, imagination, play, fear, patience and limited knowledge. They
contain no medical or other real-world advice, precise current statistics,
claims of completed player actions, or promises to mutate game state.

The offline trainer applies individual authored phrase alternatives to
produce **408 training utterances and 4,544 next-token examples**. Only one
alternative is applied to each base sentence, so combinations of two such
changes are absent from training. This allows a direct test of actual word
composition instead of merely counting variations of memorized sentences.
The DS executable contains individual words and numeric parameters, not
the corpus, full sentences, the alternative rules, or a sentence lookup.

NumPy trains embeddings and both dense layers using teacher-forced
next-token cross entropy, small label smoothing, Adam and bounded weights.
The default run uses seed **20261121**, **240 epochs** and batches of 192.
The exported model uses Q5 weights/embeddings and Q10 biases. Hidden
activation clips to 112 in its Q5 representation. Worst-case dense-layer
accumulations remain below 2^20 and fit safely in int32.

## Bounded decoding

The neural scores choose the next word. Two compact bitsets limit eligible
words to observed word transitions and the current fictional context's
vocabulary. These are word masks, not complete paths or a sentence trie.
They cannot dictate a full answer by themselves. They reduce malformed,
off-topic continuations from a very small model.

Sampling considers the four highest scores at temperature 0.5 using an
integer exponential table. A tiny low-probability tail is excluded. A
repeated three-token sequence is penalized, and legal punctuation is
favored near the token limit. The caller supplies a private seed; no
`Game` state, gameplay random stream, profile or save file can be changed.

Byte and token limits are hard bounds. Output ends at a whole word, with
punctuation when it fits. If a later sentence is cut by a limit, a completed
earlier sentence can be retained. A null pointer, zero capacity, or
one-byte capacity returns no words. Invalid enum values are sanitized
before indexing any table. All generated text is uppercase in the game's
existing supported glyph set.

## Integration contract

```c
unsigned narrative_lm_generate(char *out, size_t cap,
                               unsigned speaker, unsigned topic,
                               unsigned stage, uint32_t seed);
```

The result is the number of complete word tokens written; punctuation is
not counted. Speaker 0 is WARDEN and speaker 1 is the baby. Topics 0–7 are
greeting, world, self, play, threat, calm, dream and unknown. Stages 0–3
condition an innocent-to-uncanny progression. Invalid speaker becomes
WARDEN, invalid topic becomes unknown, and stage values above three clamp
to three. The model does not consume raw player text: the observer chooses
the relevant topic and retrieves exact facts separately.

Keep observations, care urgency, story facts, attack decisions, arithmetic,
and saved learning outside this module. A generated sentence is fictional
flavor. A fluent response must not be treated as evidence that an event
occurred or as authorization to change the simulation. WARDEN's exact
readings and the baby's actual care needs remain independently grounded.

## Verification and limits

The production C regression runs 2,048 speaker/topic/stage/seed cases and
their exact replays. Every context varies with seed, all 16 stage-zero
versus stage-three pairs change their text, and topic changes affect the
actual output. It also checks every capacity from 0 to 192, guard bytes,
oversized capacity requests, invalid enum values, extreme seeds, supported
glyphs, complete word counts, termination and four-row text wrapping.

An empirical review generated 4,096 replies using the actual quantized C
module. It produced 407 distinct outputs, including **20 distinct new
compositions across 44 samples** that are absent from all 408 training
utterances. The new combinations were inspected for readable grammar.
The longest sampled reply was 61 characters and occupied two 36-column
wrapped rows, leaving headroom in the four-row terminal.
Examples include:

- `WARDEN ONLINE. A LITTLE ROOM CAN HOLD A BIG FEAR.`
- `I AM A SMALL PERSON WITH A BIG IMAGINATION.`
- `SOMETIMES I WONDER HOW LONG A SECRET CAN HOLD ITS BREATH.`
- `COULD A GAME HAVE A SECRET THAT PLAYS TOO?`

Most output still closely follows the small authored training set. Novel
combination does not imply broad language understanding, new knowledge or
unlimited conversational memory. The model can still sound repetitive or
awkward; unknown questions should retain honest limits in the surrounding
observer flow.

The quantized model's training next-token accuracy is **91.0211%**, matching
the maximum possible accuracy for ambiguous examples with this context
window. Float and integer argmax choices agree on **97.9093%** of examples.
These are training/quantization checks, not held-out knowledge benchmarks.

Strict C11 and ASan/UBSan host runs pass. LeakSanitizer had to be disabled
in the local ptrace environment; address and undefined-behavior checks
remained active. The runtime does not allocate heap memory. Host GCC's
`-fstack-usage` reports approximately 560 bytes for generation including
inlined prediction. That is a host compiler report, not a physical DS RAM
watermark or speed measurement. Native build, text rendering and gameplay
integration still require the project's exact-head emulator gates.

## Reproducing the exported model

```sh
OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python3 tools/train_narrative_lm.py
```

The checked-in weights were exported with NumPy 2.3.5. The seed, corpus
hash and epoch count are in the generated include. A pinned NumPy/BLAS
environment is needed for bit-identical floating training; the checked-in
integer weights replay identically across supported C targets. CI builds
the exported arrays and does not train or download a model.

Corpus SHA-256:
`f37c967fb186143e7e1f1813c4c08eee1fed059a5f799a165dcc7dab59b2ccce`

Exported include SHA-256:
`54fc823d35da6cb4d1a48c9530f5785dee6e92cde3a356ed2f8725529f647131`

## Capability reference, without copied assets

The user's uploaded *ChatGPT for SNES v0.7* builds motivated a genuine
local generator plus separate factual retrieval. The
[author's project page](https://www.bxfoundry.com/chatgpt_snes/) describes
a roughly 2.2-million-parameter model on Super FX 2, a pure edition and
an optional MSU-1 fact book. That is the author's description; this work
did not execute those ROMs or establish runtime weight learning in them.

The supplied pure archive contains one 2 MiB ROM. The fact-book archive
contains a different 2 MiB ROM and a 32,748,480-byte data file whose `FB`
header records 150,000 entries. Neither archive contains source code.
No ROM data, reference model weights, vocabulary, graphics, audio, game
hints, or Wikipedia text were copied into this project.

Inspected archive hashes:

- Pure ZIP: `8cfa865271ea0219b7c6468c1e5dd80dbef3b1512960e8b0e4a42266acc19381`
- Fact-book ZIP: `e3f9bca26cfaf197d075e34cc47cb7dbde92e32d440f38b2866a1f030f6ab7fb`

The new model is trained only from this repository's original fictional
corpus and the trainer's original phrase alternatives.
