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
    # The shoulders must move as articulated limbs, not independent
    # floating hand ellipsoids. This samples a real CHAT gesture.
    stylus_tap(emu, 187, 131)
    after_chat = picture(emu, output, "game_nursery_chat_reach")
    chat_changed = difference(after_feed, after_chat, (34, 20, 222, 182))
    assert chat_changed > 28, (
        f"CHAT failed to visibly raise the connected infant arms ({chat_changed})"
    )
    # The new 3D emotion/gesture pass must affect the actual TOP display:
    # FEED lifts an arm, while NAP closes both eyes and relaxes the arms.
    emu.input.touch_set_pos(187, 160)  # NAP on unchanged lower-screen hitbox
    frames(emu, 12)
    emu.input.touch_release()
    frames(emu, 12)
    after_nap = picture(emu, output, "game_nursery_sleeping_baby")
    pose_changed = difference(after_feed, after_nap, (15, 5, 241, 185))
    assert pose_changed > 35, (
        f"3D nursery baby did not respond visibly to care pose ({pose_changed})"
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
    stylus_tap(emu, 221, 43)
    album_second_page = picture(emu, output, "game_album_page_2")
    album_next_diff = difference(
        album, album_second_page, (0, 192 + 32, 256, 192 + 171)
    )
    assert album_next_diff > 200, (
        f"Scrapbook NEXT failed to show second four memories ({album_next_diff})"
    )
    stylus_tap(emu, 34, 43)
    album_first_page = picture(emu, output, "game_album_page_1_returned")
    album_previous_diff = difference(
        album_second_page, album_first_page, (0, 192 + 32, 256, 192 + 171)
    )
    assert album_previous_diff > 200, (
        f"Scrapbook PREV failed to show first four memories ({album_previous_diff})"
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
    # Inspect pixels INSIDE the selected cabinet icon and its 2x wordmark.
    # Previously the 16 cards held tiny duplicated glyphs or empty tiles.
    thumbnail = arcade.crop((15, 192 + 78, 51, 192 + 112))
    icon_colors = len(set(thumbnail.get_flattened_data()))
    assert icon_colors >= 6, (
        f"Cabinet 01 illustration lost its shaded palette ({icon_colors} colors)"
    )
    name_pixels = arcade.crop((58, 192 + 81, 107, 192 + 97))
    name_ink = sum(
        1 for r, g, b in name_pixels.get_flattened_data()
        if r < 185 and g < 160
    )
    assert name_ink >= 50, (
        f"Enlarged 2x cabinet name absent or underdrawn ({name_ink} dark pixels)"
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
    # Regression: the optional Talking-Tom-style tool mode is SELECTED
    # from the home dock. Its icons must NOT themselves strike the doll.
    emu.input.keypad_add_key(keymask(Keys.KEY_B))
    frames(emu, 6)
    emu.input.keypad_rm_key(keymask(Keys.KEY_B))
    frames(emu, 12)
    stylus_tap(emu, 128, 183)  # middle TOYBOX dock tab
    toybox = picture(emu, output, "game_toybox_select_hand")
    toy_opened = difference(
        won, toybox, (0, 192 + 31, 256, 192 + 175)
    )
    assert toy_opened > 500, (
        f"Touching the center Toybox tab did not enter selectable mode ({toy_opened})"
    )
    stylus_tap(emu, 184, 159)  # choose boot/KICK without impact
    selected_kick = picture(emu, output, "game_toybox_select_boot")
    tool_selected = difference(
        toybox, selected_kick, (9, 192 + 145, 247, 192 + 174)
    )
    assert tool_selected > 80, (
        f"Talking-Tom-style KICK tool did not visibly highlight ({tool_selected})"
    )
    # Only touching the doll activates a selected tool.
    stylus_tap(emu, 126, 110)
    first_bounce = picture(emu, output, "game_toybox_first_kick")
    bounce_motion = difference(
        selected_kick, first_bounce, (10, 8, 244, 180)
    )
    assert bounce_motion > 35, (
        f"Ragdoll did not visibly bounce after tapping doll ({bounce_motion})"
    )
    for _ in range(5):
        frames(emu, 12)  # stress slower 20-30 FPS gameplay; allow debounce
        stylus_tap(emu, 126, 110)
    egg_begin = picture(emu, output, "game_egg_shell_fracture")
    fracture_changed = difference(
        selected_kick, egg_begin, (0, 192 + 32, 256, 192 + 174)
    )
    assert fracture_changed > 400, (
        f"Six toy impacts did not trigger secret egg ending ({fracture_changed})"
    )
    # Sample the REAL 4-stage signal meter, not a guessed frame count.
    # DeSmuME's VBlank pace can differ from game logic tick throughput.
    bar_y = 192 + 137
    bar_x = (31, 83, 135, 187)
    lit = egg_begin.getpixel((bar_x[0], bar_y))
    dim = egg_begin.getpixel((bar_x[1], bar_y))
    assert lit != dim, "Egg film's signal meter failed to render"

    def movie_stage(img) -> int:
        values = [img.getpixel((x, bar_y)) for x in bar_x]
        if not all(v in (lit, dim) for v in values):
            return -1  # A different screen, such as the ending postcard.
        n = sum(1 for v in values if v == lit)
        return n - 1 if values == [lit] * n + [dim] * (4 - n) else -1

    assert movie_stage(egg_begin) == 0, (
        "The egg film did not start with the shell fracture prologue"
    )
    # A recognizable baby face must appear in the centered prologue,
    # never the enormous back-of-hair disk left by a random physics roll.
    # The hair/eye shadow pixels are inside the character focal area.
    opening_face = egg_begin.crop((80, 50, 170, 145))
    recognizable_features = sum(
        1 for r, g, b in opening_face.get_flattened_data()
        if 25 < r < 120 and g < 115 and b < 100
    )
    assert recognizable_features >= 20, (
        "Fracture prologue lost the frontal baby face after Toybox physics "
        f"({recognizable_features} dark facial/hair pixels)"
    )

    def seek_phase(target: int, name: str):
        for _ in range(100):
            frames(emu, 12)
            sample = emu.screenshot().convert("RGB")
            current = movie_stage(sample)
            if current == target:
                return picture(emu, output, name)
            if current > target:
                raise AssertionError(
                    f"Film skipped phase {target} and advanced to {current}"
                )
        raise AssertionError(f"Egg film never reached phase {target}")

    egg_green = seek_phase(1, "game_egg_green_ichor")
    # Exclude the nursery's *green rug*. The upper-screen background must
    # now be the dark, isolated alien egg; green fluid must exist in its core.
    top_green = egg_green.crop((64, 30, 194, 168))
    green_ichor_pixels = sum(
        1 for r, g, b in top_green.get_flattened_data()
        if g > r + 28 and g > b + 19 and g > 80
    )
    assert green_ichor_pixels >= 12, (
        f"Alien-green ichor not visible in actual egg film ({green_ichor_pixels})"
    )
    # A black recess BETWEEN two pale halves is now essential to the
    # artwork. Previously the center was still pastel green and the whole
    # subject resembled a sealed polygon ball. Sample INSIDE the subject,
    # not the dark stage background surrounding it.
    recess = egg_green.crop((113, 67, 143, 137))
    cavity_dark_pixels = sum(
        1 for r, g, b in recess.get_flattened_data()
        if r < 96 and g < 130 and b < 145
    )
    assert cavity_dark_pixels >= 300, (
        f"Egg has no clearly recessed dark inner cavity ({cavity_dark_pixels})"
    )
    left_shell = egg_green.crop((81, 65, 113, 145))
    ivory_shell_pixels = sum(
        1 for r, g, b in left_shell.get_flattened_data()
        if r >= 135 and g >= 155 and b >= 105
    )
    assert ivory_shell_pixels >= 160, (
        f"Porcelain half is too small or dark to read ({ivory_shell_pixels})"
    )
    assert movie_stage(egg_green) == 1, "Wrong image labeled green alien egg"

    egg_push = seek_phase(2, "game_egg_slow_push")
    frames(emu, 52)  # linger inside the longer camera-approach interval
    egg_push = picture(emu, output, "game_egg_slow_push")
    assert movie_stage(egg_push) == 2, "Approach shot advanced too soon"

    egg_scare = seek_phase(3, "game_egg_jump_scare")
    scare_difference = difference(
        egg_push, egg_scare, (8, 7, 248, 182)
    )
    assert scare_difference > 300, (
        f"Egg film did not display its one hard-cut alien face ({scare_difference})"
    )
    assert movie_stage(egg_scare) == 3, (
        "The jump-scare image was captured in the wrong cinematic phase"
    )
    # Regress against the old isolated polygon head: retain two PALE
    # porcelain halves beside the alien when the final scare camera cuts.
    left_wing = egg_scare.crop((18, 25, 83, 167))
    right_wing = egg_scare.crop((174, 25, 245, 167))
    def ivory_count(img):
        return sum(
            1 for r, g, b in img.get_flattened_data()
            if r >= 135 and g >= 155 and b >= 105
        )
    scare_shell_left = ivory_count(left_wing)
    scare_shell_right = ivory_count(right_wing)
    assert scare_shell_left > 90 and scare_shell_right > 90, (
        "Scare cut lost its readable split porcelain egg: "
        f"left={scare_shell_left}, right={scare_shell_right}"
    )
    # Capture the NEW animated reaction, not just the first scare frame.
    # The jaw, fangs, eyes and brow must lunge as one 3D subject.
    frames(emu, 12)
    egg_lunge = picture(emu, output, "game_egg_creature_lunge")
    assert movie_stage(egg_lunge) == 3, (
        "Lunge exited the horror film before the reaction was visible"
    )
    lunge_changed = difference(
        egg_scare, egg_lunge, (22, 10, 234, 182)
    )
    assert lunge_changed > 25, (
        f"Creature is frozen after the scare cut ({lunge_changed})"
    )

    finale_heading_ink = 0
    for _ in range(100):
        frames(emu, 12)
        sample = emu.screenshot().convert("RGB")
        if movie_stage(sample) == -1:
            # A title-band switch can arrive BEFORE the rest of the 256x192
            # bitmap has finished drawing on a busy DS frame. Require the
            # actual completed 2x BAD ENDING heading, not a blank card.
            heading = sample.crop((35, 192 + 40, 218, 192 + 77))
            finale_heading_ink = sum(
                1 for r, g, b in heading.get_flattened_data()
                if r >= 216 and g >= 216 and b >= 216
            )
            if finale_heading_ink > 45:
                frames(emu, 8)  # settled, visually inspectable capture
                egg_final = picture(emu, output, "game_egg_secret_bad_ending")
                break
    else:
        raise AssertionError(
            "Alien egg did not finish rendering its actual BAD ENDING postcard"
        )
    finale_difference = difference(
        egg_begin, egg_final, (5, 192 + 35, 251, 192 + 173)
    )
    assert finale_difference > 350, (
        f"Alien-egg cutscene did not resolve to final ending ({finale_difference})"
    )
    # Do not allow a screenshot labeled "ending" that is still a film
    # frame; the real bad-ending postcard uses a fresh ending UI layout.
    finale_header = difference(
        egg_scare, egg_final, (0, 192 + 3, 256, 192 + 26)
    )
    assert finale_header > 75, (
        f"Egg cutscene did not exit into collectible BAD ENDING ({finale_header})"
    )
    # The egg is an unlock, NOT a permanent disable of the toy controls.
    # Exit through the real ending UI, reselect the dock and strike again.
    stylus_tap(emu, 122, 106)
    stylus_tap(emu, 128, 183)
    fresh_toybox = picture(emu, output, "game_toybox_after_egg_replay")
    stylus_tap(emu, 126, 110)
    replay_bounce = picture(emu, output, "game_toybox_reusable_after_egg")
    replay_movement = difference(
        fresh_toybox, replay_bounce, (10, 8, 244, 180)
    )
    assert replay_movement > 35, (
        f"Toybox controls stayed locked after secret ending ({replay_movement})"
    )
    return {
        "3D_top_screen_distinct_colors": hero_colors,
        "3D_top_screen_full_height_dark_stripes": len(full_height_stripes),
        "3D_care_pose_changed_pixels": pose_changed,
        "3D_chat_reach_changed_pixels": chat_changed,
        "3D_title_top_screen_colors": title_colors,
        "album_screen_changed_pixels": album_diff,
        "album_next_page_changed_pixels": album_next_diff,
        "album_previous_page_changed_pixels": album_previous_diff,
        "cabinet_art_distinct_colors": icon_colors,
        "cabinet_name_ink_pixels": name_ink,
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
        "Toybox_entry_changed_pixels": toy_opened,
        "Toybox_selected_tool_changed_pixels": tool_selected,
        "Toybox_first_bounce_changed_pixels": bounce_motion,
        "Toybox_egg_trigger_changed_pixels": fracture_changed,
        "Alien_green_ichor_pixels": green_ichor_pixels,
        "Egg_dark_cavity_pixels": cavity_dark_pixels,
        "Egg_visible_porcelain_pixels": ivory_shell_pixels,
        "Alien_jump_scare_changed_pixels": scare_difference,
        "Jump_scare_porcelain_left_pixels": scare_shell_left,
        "Jump_scare_porcelain_right_pixels": scare_shell_right,
        "Alien_lunge_animation_changed_pixels": lunge_changed,
        "Alien_ending_finale_changed_pixels": finale_difference,
        "Egg_green_stage": movie_stage(egg_green),
        "Egg_push_stage": movie_stage(egg_push),
        "Egg_jump_scare_stage": movie_stage(egg_scare),
        "Alien_ending_header_changed_pixels": finale_header,
        "Alien_finale_readable_ink_pixels": finale_heading_ink,
        "Toybox_post_ending_replay_changed_pixels": replay_movement,
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
