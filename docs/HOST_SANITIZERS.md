# Memory and undefined-behavior regression gate

The standard DS build and native DeSmuME walkthrough check actual ROM compatibility and screen output. Neither automatically catches every out-of-bounds C write, use-after-free or undefined integer operation in the host-side model.

## New CI gate

- Run **the same** `tests/run.sh` used by the devkitARM build, on a separate stock Ubuntu runner.
- Compile every executable with GCC AddressSanitizer and UndefinedBehaviorSanitizer, with stack traces, no recovery, leak detection, and frame pointers.
- Exercise the core care/save engine, roughhouse/egg state, single-voice PSG, full 256×192 software bitmap UI, dialogue, C-side 3D submission code, all sixteen minigames, touch and virtual stylus inputs, and 25,000 seeded gameplay transitions.
- The normal devkitARM/BlocksDS ROM builds still run their original strict C checks with no sanitizer flags and no change to embedded binaries.

## Local invocation

`MBAI_HOST_FLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all' ASAN_OPTIONS='detect_leaks=1:halt_on_error=1' UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1' sh tests/run.sh`

These are **host-language** tests. They do not imply a measured original-DS memory watermark, VRAM usage, GPU timing, or real-hardware FPS. The optional instrumented build uses larger debug binaries only on Ubuntu CI, never in the shipped `.nds`.