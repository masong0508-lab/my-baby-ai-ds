#!/usr/bin/env python3
"""Train/export the game's original tiny conditional word model with NumPy.

This is an offline development tool. The DS contains only signed integer
weights, a short vocabulary, transition masks and bounded C inference.
No downloaded model, ROM data, external text or network access is used.

Reproduce: OPENBLAS_NUM_THREADS=1 python3 tools/train_narrative_lm.py
The default seed, corpus ordering and fixed minibatch order are explicit.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import re
import sys

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
EMBED, CONTEXT, HIDDEN, HISTORY = 12, 16, 48, 3
INPUT = HISTORY * EMBED + CONTEXT
SCALE, HCLIP = 32, 112
SEED = 20261121
TOKEN = re.compile(r"[A-Z]+(?:'[A-Z]+)?|[.,?!]")

# Original, grammar-preserving fictional alternatives. Offline augmentation
# teaches local word choice instead of encouraging exact sentence recall.
# Only ONE substitution is applied to each base utterance: combinations of
# these changes are withheld and can be composed by the three-word model.
# No substitution or complete training sentence is included in DS runtime.
ALTERNATIVES = (
    ("A SMALL ", "A LITTLE "), ("A LITTLE ", "A SMALL "),
    ("A BIG ", "A LARGE "), ("A LARGE ", "A BIG "),
    ("A QUIET ", "A SILENT "), ("A GENTLE ", "A KIND "),
    ("A SOFT ", "A GENTLE "),
    ("MY LITTLE OFFICE", "MY SMALL OFFICE"),
    ("YOUR QUESTIONS", "YOUR THOUGHTS"),
    ("MY QUESTIONS", "MY THOUGHTS"),
    ("A QUESTION", "A THOUGHT"), ("A SHADOW", "A SECRET"),
    ("MY TEDDY", "MY BLANKET"), ("THE TEDDY", "THE BLANKET"),
    ("THE MOON", "THE SKY"), ("THE DARK", "THE SILENCE"),
    ("PERHAPS ", "MAYBE "), ("I WONDER ", "SOMETIMES I WONDER "),
    ("CAN A ", "COULD A "), ("CAN YOU ", "COULD YOU "),
    ("CAN WE ", "COULD WE "),
    ("KINDNESS", "PATIENCE"), ("PATIENCE", "KINDNESS"),
)


def corpus(path):
    rows = []
    for line in path.read_text(encoding="ascii").splitlines():
        if not line or line.startswith("#"):
            continue
        speaker, topic, stage, text = line.split("\t", 3)
        speaker, topic, stage = int(speaker), int(topic), int(stage)
        assert 0 <= speaker < 2 and 0 <= topic < 8 and 0 <= stage < 4
        variants = [text]
        variants.extend(text.replace(old, new) for old, new in ALTERNATIVES if old in text)
        for variant in dict.fromkeys(variants):
            tokens = TOKEN.findall(variant)
            assert "".join(tokens) == re.sub(r"\s", "", variant)
            assert tokens[-1] in (".", "?", "!")
            rows.append((speaker, topic, stage, tokens))
    counts = Counter((s, t, g) for s, t, g, _ in rows)
    assert len(counts) == 64 and min(counts.values()) >= 4
    vocabulary = ["<EOS>", "<BOS>"] + sorted({w for *_, ts in rows for w in ts})
    assert len(vocabulary) <= 512
    ids = {word: i for i, word in enumerate(vocabulary)}
    allowed = np.zeros((len(ids), len(ids)), dtype=np.bool_)
    context_allowed = np.zeros((64, len(ids)), dtype=np.bool_)
    samples, contexts, targets = [], [], []
    for speaker, topic, stage, tokens in rows:
        previous = [1] * HISTORY
        context = (speaker * 8 + topic) * 4 + stage
        for word in tokens + ["<EOS>"]:
            token = ids[word]
            allowed[previous[-1], token] = True
            context_allowed[context, token] = True
            samples.append(previous.copy())
            contexts.append(context)
            targets.append(token)
            previous = previous[1:] + [token]
    assert not allowed[:, 1].any()
    return (rows, vocabulary, allowed, context_allowed,
            np.array(samples), np.array(contexts), np.array(targets))


def initialize(vocab, rng):
    return {
        "token_embedding": rng.normal(0, .34, (vocab, EMBED)).astype(np.float32),
        "context_embedding": rng.normal(0, .34, (64, CONTEXT)).astype(np.float32),
        "input_weight": rng.normal(0, .14, (INPUT, HIDDEN)).astype(np.float32),
        "hidden_bias": np.full(HIDDEN, .15, dtype=np.float32),
        "output_weight": rng.normal(0, .12, (HIDDEN, vocab)).astype(np.float32),
        "output_bias": np.zeros(vocab, dtype=np.float32),
    }


def forward(parameters, words, context):
    inputs = np.concatenate((parameters["token_embedding"][words].reshape(-1, HISTORY * EMBED),
                             parameters["context_embedding"][context]), axis=1)
    prehidden = inputs @ parameters["input_weight"] + parameters["hidden_bias"]
    hidden = np.clip(prehidden, 0, HCLIP / SCALE)
    logits = hidden @ parameters["output_weight"] + parameters["output_bias"]
    return inputs, prehidden, hidden, logits


def train(parameters, words, context, targets, allowed, context_allowed, epochs, rng):
    first = {k: np.zeros_like(v) for k, v in parameters.items()}
    second = {k: np.zeros_like(v) for k, v in parameters.items()}
    size, batch, step = len(targets), 192, 0
    for epoch in range(epochs):
        order = rng.permutation(size)
        learning_rate = .008 * (.30 + .70 * (1 - epoch / epochs))
        total_loss, total_correct = 0., 0
        for start in range(0, size, batch):
            pick = order[start:start + batch]
            w, c, y = words[pick], context[pick], targets[pick]
            x, z, h, scores = forward(parameters, w, c)
            permitted = allowed[w[:, -1]] & context_allowed[c]
            scores[~permitted] = -1e8
            scores -= scores.max(axis=1, keepdims=True)
            probabilities = np.exp(scores)
            probabilities /= probabilities.sum(axis=1, keepdims=True)
            total_loss -= float(np.log(np.maximum(probabilities[np.arange(len(y)), y], 1e-15)).sum())
            total_correct += int((probabilities.argmax(axis=1) == y).sum())
            gradient = probabilities
            # A small uniform probability only over real word transitions
            # limits overconfident memorization of these short utterances.
            gradient -= .015 * permitted / permitted.sum(axis=1, keepdims=True)
            gradient[np.arange(len(y)), y] -= .985
            gradient /= len(y)
            hidden_gradient = gradient @ parameters["output_weight"].T
            hidden_gradient *= (z > 0) & (z < HCLIP / SCALE)
            input_gradient = hidden_gradient @ parameters["input_weight"].T
            derivatives = {
                "output_weight": h.T @ gradient,
                "output_bias": gradient.sum(axis=0),
                "input_weight": x.T @ hidden_gradient,
                "hidden_bias": hidden_gradient.sum(axis=0),
                "token_embedding": np.zeros_like(parameters["token_embedding"]),
                "context_embedding": np.zeros_like(parameters["context_embedding"]),
            }
            np.add.at(derivatives["token_embedding"], w.reshape(-1),
                      input_gradient[:, :HISTORY * EMBED].reshape(-1, EMBED))
            np.add.at(derivatives["context_embedding"], c,
                      input_gradient[:, HISTORY * EMBED:])
            step += 1
            for name, value in parameters.items():
                d = np.clip(derivatives[name], -2., 2.)
                first[name] *= .9
                first[name] += .1 * d
                second[name] *= .999
                second[name] += .001 * d * d
                value -= learning_rate * (first[name] / (1 - .9 ** step)) / (
                    np.sqrt(second[name] / (1 - .999 ** step)) + 1e-8)
                np.clip(value, -3.5 if "bias" not in name else -12.,
                        3.5 if "bias" not in name else 12., out=value)
        if epoch == 0 or (epoch + 1) % 40 == 0 or epoch + 1 == epochs:
            print(f"epoch={epoch + 1} loss={total_loss / size:.4f} "
                  f"next_token_accuracy={total_correct / size:.4f}", flush=True)


def quantize(parameters):
    return {name: np.rint(value * (SCALE * SCALE if "bias" in name else SCALE)).astype(
                np.int16 if "bias" in name else np.int8)
            for name, value in parameters.items()}


def integer_logits(q, words, context):
    inputs = np.concatenate((q["token_embedding"][words].reshape(-1, HISTORY * EMBED),
                             q["context_embedding"][context]), axis=1).astype(np.int32)
    # C uses signed division toward zero. Hidden values <=0 are discarded.
    raw = inputs @ q["input_weight"].astype(np.int32) + q["hidden_bias"]
    hidden = np.clip(raw // SCALE, 0, HCLIP)
    return hidden @ q["output_weight"].astype(np.int32) + q["output_bias"]


def c_array(name, data, dtype, dimensions):
    flat = np.asarray(data).reshape(-1).tolist()
    lines = [f"static const {dtype} {name}{dimensions} = {{"]
    # All exported C arrays are flat to keep compiler diagnostics simple.
    for start in range(0, len(flat), 24):
        lines.append(" " + ",".join(str(int(x)) for x in flat[start:start + 24]) + ",")
    return "\n".join(lines + ["};", ""])


def export(path, q, vocabulary, allowed, context_allowed, metadata):
    v = len(vocabulary)
    blob = bytearray()
    offsets = []
    for word in vocabulary:
        offsets.append(len(blob))
        blob.extend(word.encode("ascii") + b"\0")
    mask = np.packbits(allowed, axis=1, bitorder="little")
    context_mask = np.packbits(context_allowed, axis=1, bitorder="little")
    parameters = sum(x.size for x in q.values())
    content = [
        "/* Generated by tools/train_narrative_lm.py from original project text.",
        f" * Corpus SHA-256: {metadata['corpus_sha256']}",
        f" * seed={metadata['seed']}; epochs={metadata['epochs']}; NumPy={np.__version__}",
        " * Fixed weights perform next-word inference; no sentence bank is embedded.",
        " * Do not edit numerical arrays by hand. */",
        f"#define LM_VOCAB {v}u", f"#define LM_EMBED {EMBED}u",
        f"#define LM_CONTEXT {CONTEXT}u", f"#define LM_INPUT {INPUT}u",
        f"#define LM_HIDDEN {HIDDEN}u", f"#define LM_MASK_BYTES {(v + 7) // 8}u",
        f"#define LM_HIDDEN_CLIP {HCLIP}", f"#define LM_PARAMETERS {parameters}u",
        "#define LM_SCALE 32", "",
    ]
    for name, data in q.items():
        content.append(c_array("lm_" + name, data, "int16_t" if "bias" in name else "int8_t", f"[{data.size}]"))
    content.append(c_array("lm_allowed", mask, "uint8_t", f"[{mask.size}]"))
    content.append(c_array("lm_context_allowed", context_mask, "uint8_t", f"[{context_mask.size}]"))
    content.append(c_array("lm_word_offsets", offsets, "uint16_t", f"[{len(offsets)}]"))
    content.append(c_array("lm_words", blob, "char", f"[{len(blob)}]"))
    # exp(-i/8) for deterministic top-four sampling at temperature .50.
    exp_lut = [max(1, round(65535 * math.exp(-i / 8))) for i in range(65)]
    content.append(c_array("lm_exp", exp_lut, "uint16_t", f"[{len(exp_lut)}]"))
    rom_bytes = (sum(x.nbytes for x in q.values()) + mask.nbytes + context_mask.nbytes
                 + len(offsets) * 2 + len(blob) + len(exp_lut) * 2)
    assert rom_bytes <= 100 * 1024, rom_bytes
    path.write_text("\n".join(content), encoding="ascii")
    metadata.update(vocabulary=v, parameters=parameters, rom_data_bytes=rom_bytes,
                    transition_edges=int(allowed.sum()), weights_sha256=hashlib.sha256(path.read_bytes()).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--corpus", type=Path, default=ROOT / "tools/narrative_lm_corpus.tsv")
    parser.add_argument("--output", type=Path, default=ROOT / "source/narrative_lm_weights.inc")
    parser.add_argument("--epochs", type=int, default=240)
    parser.add_argument("--seed", type=int, default=SEED)
    parser.add_argument("--checkpoint", type=Path)
    args = parser.parse_args()
    if not 1 <= args.epochs <= 5000:
        parser.error("epochs must be between one and 5000")
    rows, vocabulary, allowed, context_allowed, words, context, targets = corpus(args.corpus)
    print(f"utterances={len(rows)} contexts=64 vocabulary={len(vocabulary)} samples={len(targets)}", flush=True)
    rng = np.random.default_rng(args.seed)
    parameters = initialize(len(vocabulary), rng)
    train(parameters, words, context, targets, allowed, context_allowed, args.epochs, rng)
    q = quantize(parameters)
    _, _, _, float_scores = forward(parameters, words, context)
    quant_scores = integer_logits(q, words, context)
    permitted = allowed[words[:, -1]] & context_allowed[context]
    float_scores[~permitted], quant_scores[~permitted] = -1e8, -2 ** 30
    float_choice, quant_choice = float_scores.argmax(axis=1), quant_scores.argmax(axis=1)
    grouped = {}
    for w, c, y in zip(words, context, targets):
        grouped.setdefault((*w.tolist(), int(c)), Counter())[int(y)] += 1
    achievable = sum(max(counts.values()) for counts in grouped.values()) / len(targets)
    metadata = dict(corpus_sha256=hashlib.sha256(args.corpus.read_bytes()).hexdigest(),
                    seed=args.seed, epochs=args.epochs, numpy=np.__version__, utterances=len(rows),
                    token_examples=len(targets), float_accuracy=float((float_choice == targets).mean()),
                    integer_accuracy=float((quant_choice == targets).mean()),
                    quantization_argmax_agreement=float((float_choice == quant_choice).mean()),
                    training_ambiguity_upper_bound=achievable)
    # The corpus is intentionally ambiguous (several utterances per context),
    # so next-word accuracy cannot reach 100 percent even when memorized.
    if (metadata["integer_accuracy"] < achievable - .035
            or metadata["integer_accuracy"] < metadata["float_accuracy"] - .012
            or metadata["quantization_argmax_agreement"] < .90):
        raise SystemExit("Model did not meet the declared export quality gate")
    export(args.output, q, vocabulary, allowed, context_allowed, metadata)
    print(json.dumps(metadata, sort_keys=True), flush=True)
    if args.checkpoint:
        np.savez(args.checkpoint, **parameters)


if __name__ == "__main__":
    main()
