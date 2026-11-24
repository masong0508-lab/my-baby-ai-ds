#!/usr/bin/env python3
"""Small, reproducible recorded-speech benchmark of the real C recognizer.

Data: Free Spoken Digit Dataset, release v1.0.10 (CC BY-SA 4.0).
https://github.com/Jakobovski/free-spoken-digit-dataset
Two separate speaker-dependent profiles. Train digits 0,1,2 using clips
5,6; held-out clips 0,1,2 are never trained. Digit 3 tests unknown input.
Do not interpret this deliberately small experiment as general ASR accuracy.
Only summary metrics are retained; WAVs are temporary host test inputs.
"""
import argparse
import concurrent.futures
import ctypes as C
import io
import json
import math
from pathlib import Path
import subprocess
import tempfile
import time
import urllib.request
import wave

ROOT = Path(__file__).resolve().parents[1]
CORPUS = "https://raw.githubusercontent.com/Jakobovski/free-spoken-digit-dataset/v1.0.10/recordings/"
SPEAKERS = ("jackson", "theo")
KNOWN = (0, 1, 2)
TRAIN = (5, 6)
HELDOUT = (0, 1, 2)
BLOCK = 256
RATE = 8192
READY = 3


def read_recording(key):
    speaker, digit, index = key
    filename = f"{digit}_{speaker}_{index}.wav"
    for attempt in range(3):
        try:
            with urllib.request.urlopen(CORPUS + filename, timeout=25) as r:
                wav = r.read(256 * 1024 + 1)
            if len(wav) > 256 * 1024:
                raise ValueError(f"Oversize WAV: {filename}")
            with wave.open(io.BytesIO(wav)) as w:
                if (w.getnchannels(), w.getsampwidth(), w.getframerate()) != (1, 2, 8000):
                    raise ValueError(f"Unexpected WAV format: {filename}")
                data = w.readframes(w.getnframes())
            source = [int.from_bytes(data[i:i+2], "little", signed=True)
                      for i in range(0, len(data), 2)]
            if not source:
                raise ValueError(f"Empty WAV: {filename}")
            # Match DS 8192 Hz rate with deterministic linear interpolation.
            # Do not normalize gain: retain the real recording amplitudes.
            out = []
            for i in range(math.ceil(len(source) * RATE / 8000)):
                at, rem = divmod(i * 8000, RATE)
                left = source[min(at, len(source)-1)]
                right = source[min(at+1, len(source)-1)]
                out.append((left * (RATE-rem) + right * rem) // RATE)
            return key, out
        except (OSError, ValueError, wave.Error) as err:
            if attempt == 2:
                raise RuntimeError(f"Cannot fetch {filename}: {err}") from err
            time.sleep(attempt+1)


class Engine:
    def __init__(self, library):
        self.lib = C.CDLL(str(library))
        self.lib.bench_push.argtypes = [C.POINTER(C.c_int16), C.c_uint]
        self.lib.bench_state.restype = C.c_uint
        self.lib.bench_frames.restype = C.c_uint
        self.lib.bench_train.argtypes = [C.c_char_p]
        self.lib.bench_train.restype = C.c_int
        self.lib.bench_match.argtypes = [C.POINTER(C.c_uint)]
        self.lib.bench_match.restype = C.c_int
        self.lib.bench_label.argtypes = [C.c_uint]
        self.lib.bench_label.restype = C.c_char_p

    def capture(self, pcm, quiet=False):
        self.lib.bench_begin()
        silence = (C.c_int16 * BLOCK)()
        # Exactly the normal four calibration blocks, then the raw recording.
        for _ in range(4):
            self.lib.bench_push(silence, BLOCK)
        for start in range(0, len(pcm), BLOCK):
            if self.lib.bench_state() >= READY:
                break
            values = pcm[start:start+BLOCK]
            if quiet:
                values = [int(x / 2) for x in values]
            block = (C.c_int16 * BLOCK)(*values)
            self.lib.bench_push(block, BLOCK)
        # Allow normal end-of-word or no-speech timeout; never forge READY.
        for _ in range(84):
            if self.lib.bench_state() >= READY:
                break
            self.lib.bench_push(silence, BLOCK)
        return self.lib.bench_state(), self.lib.bench_frames()

    def train(self, label):
        return self.lib.bench_train(str(label).encode("ascii"))

    def match(self):
        score = C.c_uint()
        match = self.lib.bench_match(C.byref(score))
        return (self.lib.bench_label(match).decode("ascii") if match >= 0 else None,
                score.value if score.value != 65535 else None)


def build(source, output):
    subprocess.run(["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(source), str(ROOT / "tests/voice_corpus_bridge.c"),
                    "-o", str(output)], check=True)


def evaluate(engine, recordings):
    results = []
    for speaker in SPEAKERS:
        engine.lib.bench_reset()
        for digit in KNOWN:
            for index in TRAIN:
                state, frames = engine.capture(recordings[speaker, digit, index])
                trained = engine.train(digit) if state == READY else 0
                results.append(dict(speaker=speaker, digit=digit, index=index,
                                    split="train", state=state, frames=frames,
                                    trained=trained in (1, 2)))
        for quiet in (False, True):
            for digit in (*KNOWN, 3):
                for index in HELDOUT:
                    state, frames = engine.capture(recordings[speaker, digit, index], quiet)
                    match, score = engine.match() if state == READY else (None, None)
                    outcome = ("unknown_rejected" if match is None else "unknown_accepted") if digit == 3 else (
                        "rejected" if match is None else "correct" if match == str(digit) else "incorrect")
                    results.append(dict(speaker=speaker, digit=digit, index=index,
                                        split="quiet" if quiet else "original",
                                        state=state, frames=frames, match=match,
                                        score=score, outcome=outcome))
    summaries = {}
    for split in ("train", "original", "quiet"):
        rows = [r for r in results if r["split"] == split]
        stats = {"count": len(rows), "captured": sum(r["state"] == READY for r in rows)}
        if split == "train":
            stats["trained"] = sum(r["trained"] for r in rows)
        else:
            for outcome in ("correct", "incorrect", "rejected", "unknown_rejected", "unknown_accepted"):
                stats[outcome] = sum(r["outcome"] == outcome for r in rows)
            stats["known_captured"] = sum(r["digit"] in KNOWN and r["state"] == READY for r in rows)
        summaries[split] = stats
    return {"summary": summaries, "recordings": results}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--baseline", required=True, help="Full prior Git commit, with identical voice.h layout")
    p.add_argument("--output", type=Path, default=Path("voice-corpus-report.json"))
    args = p.parse_args()
    keys = [(s, d, i) for s in SPEAKERS for d in KNOWN for i in TRAIN]
    keys += [(s, d, i) for s in SPEAKERS for d in (*KNOWN, 3) for i in HELDOUT]
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        recordings = dict(pool.map(read_recording, keys))
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        baseline = tmp / "baseline.c"
        baseline.write_bytes(subprocess.check_output(["git", "show", args.baseline + ":source/voice.c"], cwd=ROOT))
        for name, source in (("baseline", baseline), ("candidate", ROOT/"source/voice.c")):
            build(source, tmp / (name + ".so"))
        before = evaluate(Engine(tmp / "baseline.so"), recordings)
        after = evaluate(Engine(tmp / "candidate.so"), recordings)
    report = {"dataset": "FSDD v1.0.10; CC BY-SA 4.0", "dataset_url": CORPUS,
              "protocol": "two speaker-dependent 3-label profiles, disjoint train 5/6, test 0/1/2; digit 3 unknown; half-volume test derived from same held-out clips",
              "unique_recordings": len(recordings), "baseline_commit": args.baseline,
              "baseline": before, "candidate": after}
    # An accepted capture is not the same thing as a correct word.
    # Release must preserve correct matches AND avoid increasing false
    # accepts in either volume condition. Aggregate all failures before
    # raising so the artifact identifies every blocker, not just the first.
    b, a = before["summary"], after["summary"]
    failures = []
    if a["train"]["trained"] < b["train"]["trained"]:
        failures.append("Lost usable training recordings")
    if a["original"]["known_captured"] < b["original"]["known_captured"]:
        failures.append("Lost original speech capture")
    if a["quiet"]["known_captured"] <= b["quiet"]["known_captured"]:
        failures.append("No gain in quiet speech capture")
    for split in ("original", "quiet"):
        if a[split]["correct"] < b[split]["correct"]:
            failures.append(f"{split}: fewer correctly recognized known words")
        if a[split]["incorrect"] > b[split]["incorrect"]:
            failures.append(f"{split}: more incorrectly recognized known words")
        if a[split]["unknown_accepted"] > b[split]["unknown_accepted"]:
            failures.append(f"{split}: more accepted unknown words")
    report["release_gate"] = {"passed": not failures, "failures": failures}
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"baseline": b, "candidate": a, "release_gate": report["release_gate"]}, indent=2))
    if failures:
        raise SystemExit("Recorded-speech release blocked: " + "; ".join(failures))


if __name__ == "__main__":
    main()
