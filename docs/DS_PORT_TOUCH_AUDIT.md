# Cross-repository Nintendo DS touchscreen audit

Audited on 2026-11-20 for the ongoing **DraStic touch never registers** problem in My Baby A.I.! This is based on actual source in other repositories owned by the same developer, not an assumption that similar-looking menus have comparable input handling.

## DS ports with concrete touchscreen implementations

| Repo | Files inspected | Implementation | Important findings |
|---|---|---|---|
| [Mandex](https://github.com/masong0508-lab/mandex) | `ds/bare/arm7.c`, `ds/bare/hw9.c`, `ds/bare/nds.h`, `ds/source/main.c`, `tools/build_ds_bare.sh`, CRT/linker scripts | Custom **bare-metal** ARM7 reads physical EXTKEYIN pen bit and 12-bit SPI touch ADC, calibrates, writes shared `0x027FF000`; ARM9 samples direct keypad hardware plus IPC. | Avoids libnds/Calico startup and FIFO. Correct `scanKeys()` → `touchRead()` ordering also exists here; that ordering alone cannot explain the baby's missing KEY_TOUCH. |
| [Bore: GodGuy](https://github.com/masong0508-lab/bore-gba) | `godguy/hw/arm7.c`, `godguy/compat/ndshw.h`, `godguy/source/ggmenu.h`, `godguy/build_hw.sh` | Custom bare ARM7 reads pen twice/frame, filters multiple ADC samples, preserves short taps and sends them via shared `0x023F0000`. | README identifies the bare `godguy_hw.nds` as the DraStic-compatible build. Its handler also rejects ghost and jump samples and supports finger-sized hitbox margins. |
| [Bore 2](https://github.com/masong0508-lab/bore2) | README DS section | DS port described as **in progress**. | No separate, verified DS touchscreen driver was established from examined code. |
| [DOOMOGACHI](https://github.com/masong0508-lab/DOOMOGACHI-LIFE-in-HELLvile-park-exclamationmark) | README and indexed GitHub default-branch code search, plus its named Claude branch README | Various console ports; search did not surface a concrete DS touch driver. | Don't guess at a touchscreen implementation not located in source. |

## My Baby A.I. versus those engines

My Baby A.I.'s working DraStic ROM uses **BlocksDS**, with the default ARM7 libnds input driver. Its ARM7 driver polls `touchPenDown()`, reads SPI ADC, applies five-reading noise and debouncing filters and sends a `SYS_INPUT_MESSAGE` via FIFO; ARM9 `scanKeys()` latches that message before `touchRead()`. We tested both the default Maxmod and minimal BlocksDS ARM7 cores: neither changed the reported missing touch on the user's DraStic installation. The independent BlocksDS standalone probe also recorded no `KEY_TOUCH`.

Mandex and GodGuy use a different ARM7 → shared-memory implementation, so a *different bare-metal test* is justified. The companion `bare-touch-arm7.nds` built from `diagnostics/bare_touch/` uses the Mandex-style register and SPI approach **without libnds and without the game**. It deliberately distinguishes raw pen GPIO from valid ADC data:

- **Top purple:** idle (background blinks slightly to show the ARM9 loop is alive).
- **Top yellow:** A button detected through ARM9's hardware register (control case).
- **Top bright green:** ARM7 EXTKEYIN pen bit asserted, regardless of ADC calibration.
- **Bottom blue:** no pen; **bottom pink:** pen GPIO but bad/unsupported ADC; **bottom teal (varying):** pen and valid ADC samples.

If the top goes green and the bottom teal when touching, DraStic can deliver stylus via the custom bare ARM7 path. This is evidence to plan a **real alternate native-touch game port**, not evidence that BlocksDS itself is fixed. If the top never goes green, the root issue is more fundamental; the controller-driven virtual stylus in the standard game remains the safe workaround.

## Actual playable workaround already implemented

`feature/virtual-stylus-drastic` was merged to main: **L+SELECT** toggles a small virtual stylus, **D-pad** moves it and **A** taps the game's actual touch regions. Uses only a 242-byte VRAM cursor backing patch, so it does not need the physical stylus or an extra framebuffer. That is a playable feature, while native input remains under investigation.

## Safety/engineering constraints

Never link Mandex bare ARM7 to the BlocksDS ARM9 main program without validating both CPU startup and IPC: they use **different shared-memory protocols** and replacing the firmware core blindly could break DLDI, lid handling, saved games or boot. Keep this ROM a separate disposable diagnostic. A compiled ROM doesn't prove DraStic runtime behavior; validate using the emulator before committing a runtime change to the main game.
