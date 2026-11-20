#!/usr/bin/env python3
"""Real DeSmuME boot, button, stylus and game-UI integration test.

Uses py-desmume's *actual DeSmuME emulator* (not libnds stubs).
Injects touch positions, executes ARM9/ARM7 frames and asserts pixels in
the screenshot are updated by code running *inside* the emulated ROM.

A failed assertion is a failed emulator test, even if every cross-build
and host-side hitbox test passed. Images/logs are saved for triage.

Run: python3 tests/emulator_touch.py blocksds-touch-probe-ds-only.nds \
         my-baby-ai-blocksds-ds-only.nds --output emulator-proof
"""
from __future__ import annotations

import argparse
import json
import sys
import traceback
from pathlib import Path

from PIL import ImageChops
from desmume.controls import Keys, keymask
from desmume.emulator import DeSmuME

FRAMES_BOOT = 120
FRAMES_HOLD = 24


def frames(emu: DeSmuME, n: int) -> None:
    for _ in range(n):
        emu.cycle(with_joystick=False)


def picture(emu: DeSmuME, output: Path, name: str):
    img = emu.screenshot().convert("RGB")
    assert img.size == (256, 384), f"Unexpected DS screenshot size: {img.size}"
    img.save(output / f"{name}.png")
    return img


def difference(a, b, rectangle) -> int:
    # Number of pixels that differ in a fixed display rectangle.
    aa = a.crop(rectangle)
    bb = b.crop(rectangle)
    delta = ImageChops.difference(aa, bb)
    return sum(1 for value in delta.convert("RGB").getdata() if value != (0, 0, 0))


def test_probe(path: Path, output: Path) -> dict:
    emu = DeSmuME()
    emu.open(str(path.resolve()))
    frames(emu, FRAMES_BOOT)
    base = picture(emu, output, "probe_idle")
    off = base.getpixel((13, 75))
    assert off[0] > off[1] + 45, (
        f"Probe never drew its red/off touch indicator; got {off}"
    )
    # A physical DS button is our positive control for emulator input.
    emu.input.keypad_add_key(keymask(Keys.KEY_A))
    frames(emu, 12)
    a_down = picture(emu, output, "probe_A_pressed")
    a_color = a_down.getpixel((241, 74))
    assert a_color[0] > 170 and a_color[1] > 160 and a_color[2] < 150, (
        f"A-button press did not change its indicator to yellow: {a_color}"
    )
    emu.input.keypad_rm_key(keymask(Keys.KEY_A))
    frames(emu, 8)
    # This calls DeSmuME's native emulator stylus injection, NOT our game API.
    emu.input.touch_set_pos(120, 116)
    frames(emu, FRAMES_HOLD)
    pressed = picture(emu, output, "probe_touch_pressed")
    on = pressed.getpixel((13, 75))
    assert on[1] > on[0] + 45, (
        f"ROM does not show green KEY_TOUCH after injected stylus: {on}"
    )
    xy_before = pressed
    emu.input.touch_set_pos(184, 79)
    frames(emu, FRAMES_HOLD)
    moved = picture(emu, output, "probe_touch_moved")
    updated_pixels = difference(xy_before, moved, (9, 153, 203, 170))
    assert updated_pixels > 10, (
        f"ROM did not update emulated touch X/Y text: {updated_pixels} pixels"
    )
    emu.input.touch_release()
    frames(emu, 12)
    released = picture(emu, output, "probe_touch_released")
    off_after = released.getpixel((13, 75))
    assert off_after[0] > off_after[1] + 45, (
        f"ROM never saw stylus release: {off_after}"
    )
    return {
        "A_button_yellow": list(a_color),
        "touch_off_red": list(off),
        "touch_on_green": list(on),
        "touch_released_red": list(off_after),
        "xy_changed_pixels": updated_pixels,
        "result": "PASS",
    }


def test_actual_game(path: Path, output: Path) -> dict:
    emu = DeSmuME()
    emu.open(str(path.resolve()))
    frames(emu, FRAMES_BOOT)
    title = picture(emu, output, "game_title")
    # Title screen should be painted by game's own bitmap UI, not white.
    center = title.getpixel((128, 192 + 75))
    assert center != (255, 255, 255), "Game remained all white at title."
    emu.input.keypad_add_key(keymask(Keys.KEY_SELECT))
    frames(emu, 6)
    emu.input.keypad_rm_key(keymask(Keys.KEY_SELECT))
    frames(emu, 18)
    before = picture(emu, output, "game_touch_diagnostic_idle")
    # The game draws a new diagnostic page when SELECT is pressed.
    new_screen = difference(title, before, (0, 192 + 33, 256, 192 + 170))
    assert new_screen > 200, (
        f"SELECT did not enter the game's touch diagnostic ({new_screen} pixels)"
    )
    emu.input.touch_set_pos(93, 123)
    frames(emu, FRAMES_HOLD)
    pressed = picture(emu, output, "game_touch_diagnostic_pressed")
    # Check the KEY_TOUCH status text region on the *actual game* bottom screen.
    status = difference(before, pressed, (104, 192 + 62, 240, 192 + 77))
    xy1 = difference(before, pressed, (62, 192 + 77, 231, 192 + 92))
    assert status > 10, (
        f"Game's KEY_TOUCH status failed to change after emulator touch ({status})"
    )
    assert xy1 > 10, (
        f"Game's touch coordinate labels did not update on injected press ({xy1})"
    )
    emu.input.touch_set_pos(191, 153)
    frames(emu, FRAMES_HOLD)
    moved = picture(emu, output, "game_touch_diagnostic_moved")
    xy2 = difference(pressed, moved, (62, 192 + 77, 231, 192 + 92))
    assert xy2 > 10, (
        f"Game's X/Y didn't track moving stylus inside emulator ({xy2})"
    )
    emu.input.touch_release()
    frames(emu, 15)
    picture(emu, output, "game_touch_diagnostic_released")
    # Native touch must also ACTIVATE controls in the real game, not only
    # update a diagnostic label. Leave the diagnostic via real B first.
    emu.input.keypad_add_key(keymask(Keys.KEY_B))
    frames(emu, 6)
    emu.input.keypad_rm_key(keymask(Keys.KEY_B))
    frames(emu, 12)
    ready = picture(emu, output, "game_title_before_touch_start")
    emu.input.touch_set_pos(128, 115)  # START GAME on lower title screen
    frames(emu, 15)
    emu.input.touch_release()
    frames(emu, 22)
    nursery = picture(emu, output, "game_nursery_after_touch_start")
    start_changed = difference(ready, nursery, (0, 192 + 30, 256, 192 + 175))
    assert start_changed > 500, (
        f"Stylus could be read, but touching START GAME did not enter nursery "
        f"({start_changed} changed pixels)"
    )
    emu.input.touch_set_pos(67, 132)  # FEED on lower nursery touchscreen
    frames(emu, 12)
    emu.input.touch_release()
    frames(emu, 12)
    after_feed = picture(emu, output, "game_nursery_after_touch_feed")
    feed_changed = difference(nursery, after_feed, (0, 192 + 55, 256, 192 + 175))
    assert feed_changed > 50, (
        f"Touch FEED did not change needs bars or dialogue UI "
        f"({feed_changed} pixels)"
    )
    return {
        "title_to_diagnostic_changed_pixels": new_screen,
        "KEY_TOUCH_status_changed_pixels": status,
        "coordinates_on_press_changed_pixels": xy1,
        "coordinates_on_move_changed_pixels": xy2,
        "touch_START_changed_pixels": start_changed,
        "touch_FEED_changed_pixels": feed_changed,
        "result": "PASS",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("probe", type=Path)
    parser.add_argument("game", type=Path)
    parser.add_argument("--output", type=Path, default=Path("emulator-proof"))
    parser.add_argument("--stage", choices=("probe", "game"), required=True,
                        help="One emulator instance per process avoids py-desmume teardown crashes")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    report = {"emulator": "DeSmuME (py-desmume)", "stage": args.stage, "status": "FAILED"}
    output = args.output / f"{args.stage}-report.json"
    try:
        if args.stage == "probe":
            report["probe"] = test_probe(args.probe, args.output)
        else:
            report["actual_game"] = test_actual_game(args.game, args.output)
        report["status"] = "PASSED: native emulator stylus input reached ARM7/ARM9 and visible UI"
    except Exception as exc:
        traceback.print_exc()
        report["error"] = f"{type(exc).__name__}: {exc}"
        output.write_text(
            json.dumps(report, indent=2) + "\n", encoding="utf-8"
        )
        return 1
    output.write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
