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
    return sum(1 for value in delta.convert("RGB").get_flattened_data() if value != (0, 0, 0))


def stylus_tap(emu: DeSmuME, x: int, y: int) -> None:
    """One complete native stylus gesture, with enough release frames to re-arm."""
    emu.input.touch_set_pos(x, y)
    frames(emu, 8)
    emu.input.touch_release()
    frames(emu, 8)


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
    # The TOP screen is no longer an empty purple slab. It displays the
    # lightweight hardware 3D baby portrait after the bottom title appears.
    title_top = title.crop((0, 0, 256, 192))
    title_top.save(output / "game_title_3d_hero.png")
    title_colors = len(set(title_top.get_flattened_data()))
    assert title_colors > 20, (
        f"Title top screen remained blank/unrendered ({title_colors} colors)"
    )
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
    released = picture(emu, output, "game_touch_diagnostic_released")
    cross_removed = difference(
        moved, released, (20, 192 + 108, 238, 192 + 161)
    )
    assert cross_removed > 10, (
        f"Touch-test crosshair did not disappear on stylus release ({cross_removed})"
    )
    xy_after_release = difference(
        moved, released, (62, 192 + 77, 231, 192 + 92)
    )
    assert xy_after_release == 0, (
        f"Touch-test X/Y jumped to fake 0,0 on release ({xy_after_release})"
    )
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
    # Validate and archive the actual hardware-3D TOP screen, not only the
    # touch UI. The last defect was a malformed baby and long vertical lines.
    top = nursery.crop((0, 0, 256, 192))
    top.save(output / "game_nursery_hero_top_screen.png")
    hero_colors = len(set(top.crop((16, 12, 240, 178)).get_flattened_data()))
    assert hero_colors > 20, (
        f"3D nursery render is blank or under-detailed ({hero_colors} colors)"
    )
    def brightness(rgb):
        return (rgb[0] * 3 + rgb[1] * 6 + rgb[2]) // 10
    full_height_stripes = []
    for x in range(16, 240):
        dark_column = 0
        for y in range(14, 178):
            center = brightness(top.getpixel((x, y)))
            left = brightness(top.getpixel((x - 1, y)))
            right = brightness(top.getpixel((x + 1, y)))
            if center + 65 < left and center + 65 < right:
                dark_column += 1
        if dark_column >= 148:
            full_height_stripes.append(x)
    assert not full_height_stripes, (
        f"Top-screen full-height black line artifacts at x={full_height_stripes}"
    )
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
    # Reach the ending album on the actual ARM9 program, then return.
    # The album was previously only covered by a host-side framebuffer test.
    emu.input.keypad_add_key(keymask(Keys.KEY_L))
    frames(emu, 6)
    emu.input.keypad_rm_key(keymask(Keys.KEY_L))
    frames(emu, 12)
    album = picture(emu, output, "game_ending_album")
    album_diff = difference(after_feed, album, (0, 192 + 30, 256, 192 + 176))
    assert album_diff > 400, (
        f"Shoulder shortcut failed to open ending scrapbook ({album_diff})"
    )
    emu.input.keypad_add_key(keymask(Keys.KEY_B))
    frames(emu, 6)
    emu.input.keypad_rm_key(keymask(Keys.KEY_B))
    frames(emu, 12)
    # Exercise real arcade hitboxes, four-page navigation, and a complete
    # multi-step touchscreen minigame.
    stylus_tap(emu, 35, 183)  # ARCADE footer on nursery screen
    arcade = picture(emu, output, "game_arcade_via_touch")
    arcade_changed = difference(
        after_feed, arcade, (0, 192 + 30, 256, 192 + 175)
    )
    assert arcade_changed > 500, (
        f"Touch ARCADE footer did not open cabinet grid ({arcade_changed})"
    )
    stylus_tap(emu, 220, 44)  # tap NEXT: second page of four cabinets
    arcade_second_page = picture(emu, output, "game_arcade_page_2")
    page_changed = difference(
        arcade, arcade_second_page, (0, 192 + 32, 256, 192 + 172)
    )
    assert page_changed > 300, (
        f"Arcade NEXT page did not replace cabinet cards ({page_changed})"
    )
    stylus_tap(emu, 34, 44)  # tap PREV to restore first four cabinets
    arcade_first_page = picture(emu, output, "game_arcade_page_1_returned")
    page_returned = difference(
        arcade_second_page, arcade_first_page, (0, 192 + 32, 256, 192 + 172)
    )
    assert page_returned > 300, (
        f"Arcade PREV failed to restore cabinet cards ({page_returned})"
    )
    stylus_tap(emu, 35, 76)  # Bottle Balance is preselected
    bottle = picture(emu, output, "game_bottle_via_touch")
    bottle_changed = difference(
        arcade, bottle, (0, 192 + 30, 256, 192 + 175)
    )
    assert bottle_changed > 500, (
        f"Touching the selected bottle cabinet did not start it ({bottle_changed})"
    )
    emu.input.keypad_add_key(keymask(Keys.KEY_B))
    frames(emu, 6)
    emu.input.keypad_rm_key(keymask(Keys.KEY_B))
    frames(emu, 8)
    stylus_tap(emu, 176, 91)  # select Diaper Dash, large second card
    stylus_tap(emu, 176, 91)  # enter selected cabinet
    frames(emu, 12)  # capture a settled frame, not an intermediate VRAM write
    diaper = picture(emu, output, "game_diaper_start")
    # The first frame used to photograph a blank panel while UI drew into
    # VRAM. Enforce the presence of the actual 2x "DIAPER DASH" heading.
    title_crop = diaper.crop((14, 192 + 35, 162, 192 + 51))
    dark_ink = sum(
        1 for r, g, b in title_crop.get_flattened_data()
        if r < 185 and g < 160
    )
    assert dark_ink > 45, (
        f"Diaper game heading was missing/partially cleared ({dark_ink} pixels)"
    )
    diaper_changed = difference(
        arcade, diaper, (0, 192 + 30, 256, 192 + 175)
    )
    assert diaper_changed > 500, (
        f"Diaper Dash did not open from native stylus taps ({diaper_changed})"
    )
    # WIPE is now a genuine continuous scrub gesture, NOT an old tap quiz.
    emu.input.touch_set_pos(110, 128)
    frames(emu, 5)
    for i in range(8):
        emu.input.touch_set_pos(145 if i % 2 == 0 else 116, 128)
        frames(emu, 5)
    emu.input.touch_release()
    frames(emu, 10)
    diaper_step2 = picture(emu, output, "game_diaper_after_wipe")
    step_changed = difference(
        diaper, diaper_step2, (0, 192 + 48, 256, 192 + 80)
    )
    assert step_changed > 30, (
        f"Diaper Dash did not advance after WIPE touch ({step_changed})"
    )
    # DRAG a clean diaper from the tray onto the baby.
    emu.input.touch_set_pos(35, 155)
    frames(emu, 8)
    emu.input.touch_set_pos(127, 135)
    frames(emu, 8)
    emu.input.touch_release()
    frames(emu, 10)
    # SWIPE upward to fold; then fasten LEFT and RIGHT tabs in order.
    emu.input.touch_set_pos(126, 157)
    frames(emu, 8)
    emu.input.touch_set_pos(126, 110)
    frames(emu, 8)
    emu.input.touch_release()
    frames(emu, 10)
    stylus_tap(emu, 83, 128)
    stylus_tap(emu, 169, 128)
    won = picture(emu, output, "game_arcade_after_diaper_win")
    returned = difference(
        diaper, won, (0, 192 + 30, 256, 192 + 175)
    )
    # The redesigned shared header places money in the upper-right pill.
    # Compare the REAL badge, not the former x=150..190 layout.
    reward = difference(arcade, won, (208, 192 + 5, 252, 192 + 23))
    assert returned > 500 and reward > 5, (
        f"Four native stylus diaper tasks did not finish and award currency "
        f"(screen={returned}, reward={reward})"
    )
    return {
        "3D_top_screen_distinct_colors": hero_colors,
        "3D_top_screen_full_height_dark_stripes": len(full_height_stripes),
        "3D_title_top_screen_colors": title_colors,
        "album_screen_changed_pixels": album_diff,
        "arcade_next_page_changed_pixels": page_changed,
        "arcade_previous_page_changed_pixels": page_returned,
        "diaper_heading_ink_pixels": dark_ink,
        "title_to_diagnostic_changed_pixels": new_screen,
        "KEY_TOUCH_status_changed_pixels": status,
        "coordinates_on_press_changed_pixels": xy1,
        "coordinates_on_move_changed_pixels": xy2,
        "crosshair_release_changed_pixels": cross_removed,
        "touch_START_changed_pixels": start_changed,
        "touch_FEED_changed_pixels": feed_changed,
        "touch_ARCADE_changed_pixels": arcade_changed,
        "touch_BOTTLE_changed_pixels": bottle_changed,
        "touch_DIAPER_changed_pixels": diaper_changed,
        "touch_DIAPER_stage_changed_pixels": step_changed,
        "touch_DIAPER_win_reward_changed_pixels": reward,
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
