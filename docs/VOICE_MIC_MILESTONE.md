# Original DS microphone & trainable short-phrase lab

## Scope of this implemented milestone

The Nintendo DS/DS Lite microphone is real hardware served by the ARM7.
Baby AI now provides an explicit **MIC LAB** reached from the WARDEN/BABY
typed terminal (CPU -> type a 1-24 character label -> tap MIC).
TRAIN starts an opt-in short recording; LISTEN attempts recognition against
previously taught examples; USE forwards a recognized textual label to the
already existing WARDEN/BABY conversation API. Pressing B cancels.

This is **speaker-trained, limited-vocabulary template recognition**, not
general speech-to-text or an open-ended voice assistant. The small neural
word model continues to handle the already supported fiction topics;
it is not retrained from microphone recordings. Unknown/ambiguous
voices produce an explicit UNSURE result, never an automatic care action,
shelter repair, attack, save erase or conversation command.

## Original hardware capture and performance

- Both devkitPro/libnds/calico and BlocksDS/libnds expose
  soundMicRecord(buffer, total_bytes, MicFormat_12Bit, hz, callback)
  and soundMicOff(). Recording is ARM7-driven.
- The backend uses a static **1,024-byte, 32-byte-aligned double buffer**
  with two 512-byte (256 sample) chunks, at **8,192 samples/sec**.
- The ARM9 invalidates just the completed cache-aligned half using
  DC_InvalidateRange before reading PCM16 written by ARM7. No heap,
  framebuffer, raw WAV files or extra PCM history buffer is created.
- On each completed half, the fixed-point frontend first removes the
  microphone's per-block DC baseline (real or emulated ADC idle offset)
  before measuring voice level, zero-crossings, short differences and
  longer-lag differences. A quiet nonzero PCM offset must not register
  as speech or fill the input meter; signed 32-bit accumulation is bounded.
  Recording calibrates the noise floor for four chunks, waits for speech,
  then keeps at most 32 feature frames. Up to 80 chunks can elapse
  before a no-speech rejection. The UI has an additional finite
  timeout when no callbacks arrive (e.g. unsupported emulator).
- Two user recordings of each short label build two independent
  12-bin x 4-feature models. Training replaces the older of the
  two samples when the same label is taught repeatedly. A 24-slot
  ring replaces the oldest distinct label once full. Each slot
  retains at most 24 ASCII label characters.
- The nearest-template matcher uses bounded integer distances,
  explicit thresholds and a runner-up margin. A noisy, silent or
  ambiguous attempt does not automatically pick a word.
- The microphone is off during normal CARE, TALK, WARDEN, arcade,
  Room Tour, Toybox, films and idle play. PSG sounds are stopped
  before every microphone session to avoid self-triggered signals.

## Learning and save format

A VoiceProfile holds 24 names and up to 48 examples, under 3.5 KiB.
The optional voice.0 and voice.1 companions are separate from the
existing version-1 Game, 40-byte learned-style and 152-byte observer
profiles. A companion has an independent magic/version/size,
an exact saved-Game sequence and FNV digest, full profile validation
and record digest. Missing, corrupt, truncated or mismatched files
leave voice lessons in a neutral state without losing the baby.

No raw audio is stored, played back, transmitted or written to FAT.
The optional profile is only saved when FAT/DLDI storage is explicitly
enabled; when RAM-only, lessons disappear at restart.
Failure to save learned microphone features keeps the lessons in RAM
and displays a higher-priority warning. Reset erases only this game's
two voice companions alongside its existing files.

## Controls

1. In CARE, press UP or tap CPU for WARDEN.
2. Type HELLO, BOND, WORLD or another short label (1-24 characters).
3. Touch MIC above the terminal keyboard. The microphone starts **off**.
4. Press A or tap TRAIN; wait for quiet calibration, then speak once.
5. Repeat TRAIN with the same word. Two examples improve reliability.
6. Press X or tap LISTEN, then speak. A confident match appears.
7. Press Y or tap USE. This explicitly submits the matched label to
   the current WARDEN/BABY role. B returns to the typed keyboard.
8. If the microphone is unsupported in your emulator, use the keyboard.
   All original controls remain usable.

## Tests and remaining accuracy work

Strict host tests exercise 24 training slots, duplicate teaching,
two-example matching, silence/no-speech, malformed labels and
profile corruption. Separate real-file tests cover exact-Game binding,
optional save recovery, older slots, truncation, extra trailing bytes,
checksum damage and write failures. Both ROM toolchains must compile
the ARM7 callback and linked model. The DeSmuME native WARDEN route
checks reaching MIC LAB, starting a bounded capture and exiting cleanly.

**Not yet verified:** live microphone signals on a physical DS/DS Lite,
actual word error rates, environmental noise, voice-to-voice variation,
DraStic microphone passthrough, hardware frame pacing while capturing,
and power-cycle storage on a real flashcart. A passing emulator UI test
does not prove real speech recognition. More robust acoustic embeddings,
MFCC/DTW alignment, confidence calibration and more languages belong
in subsequent milestones after actual microphone examples are collected.

## Research references

- BlocksDS Peripherals tutorial, section 7 Microphone:
  https://blocksds.skylyrac.net/tutorial/intermediate/peripherals/
- Current libnds mic callback and capture format:
  https://github.com/devkitPro/libnds/blob/master/include/nds/arm9/sound.h
- BlocksDS libnds sound header:
  https://github.com/blocksds/libnds/blob/master/include/nds/arm9/sound.h
- Calico microphone buffer/cache requirements:
  https://github.com/devkitPro/calico/blob/master/include/calico/nds/arm9/mic.h
