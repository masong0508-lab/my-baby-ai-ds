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
    # The live geometry numbers are printed only on this optional
    # diagnostics screen. Verify both counters on an actual DeSmuME
    # screenshot, not merely a host-side framebuffer mock.
    def geometry_ink(box):
        crop = before.crop(box)
        return sum(
            1 for red, green, blue in crop.get_flattened_data()
            if green > red + 60 and blue > red + 60 and green > 140
        )
    visible_vertices = geometry_ink((59, 192 + 163, 124, 192 + 174))
    visible_polygons = geometry_ink((173, 192 + 163, 238, 192 + 174))
    assert visible_vertices > 12 and visible_polygons > 12, (
        "DS stylus diagnostic is missing live vertex/polygon counters "
        f"(vertices={visible_vertices}, polygons={visible_polygons})"
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
    # The native lower screen must show the new non-intrusive care coach.
    # Starting needs are healthy, so the header should say ALL GOOD in
    # teal (x=126..188) without obscuring BABY SAYS or the ROOM hitbox.
    goal_strip = nursery.crop((126, 192 + 35, 188, 192 + 43))
    care_coach_ink = sum(
        1 for rr, gg, bb in goal_strip.get_flattened_data()
        if gg > rr + 40 and bb > rr + 40 and gg > 145
    )
    assert care_coach_ink > 28, (
        f"Care guidance is absent or unreadable on native DS lower LCD "
        f"({care_coach_ink} teal pixels)"
    )
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
    # This regression was reported from the actual care/nursery image:
    # (1) a ruler-straight pale slash across the dresser/infant cheek,
    # (2) another one across the pink wall, (3) an olive-green sheet
    # protruding between the synthetic baby's legs. Test native pixels.
    def longest_pale_run(box, minimum):
        x0, y0, x1, y1 = box
        longest = 0
        for yy in range(y0, y1):
            start = -1
            for xx in range(x0, x1 + 1):
                if xx < x1:
                    rr, gg, bb = top.getpixel((xx, yy))
                    lit = (rr >= minimum[0] and gg >= minimum[1]
                           and bb >= minimum[2])
                else:
                    lit = False
                if lit and start < 0:
                    start = xx
                elif not lit and start >= 0:
                    longest = max(longest, xx - start)
                    start = -1
        return longest
    # At y=75..81 the real dresser TOP is a legitimately continuous 40px
    # ivory slab. The reported corruption was an isolated 91px ribbon at
    # y=89, across the drawer, wall and infant cheek. Check the corruption
    # height only, and tighten the threshold from 40px to 10px.
    dresser_streak = longest_pale_run((9, 84, 112, 112), (245, 218, 178))
    wall_streak = longest_pale_run((149, 77, 220, 115), (232, 220, 190))
    assert dresser_streak < 10, (
        f"Bright dresser/face surface streak still visible ({dresser_streak}px)"
    )
    assert wall_streak < 28, (
        f"Back-wall wainscot is still a bright ruler line ({wall_streak}px)"
    )
    olive_artifact = sum(
        1 for yy in range(148, 170) for xx in range(101, 145)
        if (lambda c: c[1] > c[0] + 18 and
            c[1] > c[2] + 35 and c[0] < 170)(top.getpixel((xx, yy)))
    )
    assert olive_artifact < 20, (
        "Olive-green contact quad is still cutting through the baby's "
        f"legs ({olive_artifact} green pixels)"
    )
    start_changed = difference(ready, nursery, (0, 192 + 30, 256, 192 + 175))
    assert start_changed > 500, (
        f"Stylus could be read, but touching START GAME did not enter nursery "
        f"({start_changed} changed pixels)"
    )
    # ROOM TOUR IS REAL GAMEPLAY. Verify the HOME badge enters a different
    # lower-screen mode, and the window/lamp change real TOP 3D pixels.
    stylus_tap(emu, 216, 40)   # visible ROOM button above care cards
    room_night = picture(emu, output, "game_room_tour_night")
    room_opened = difference(
        nursery, room_night, (0, 192 + 28, 256, 192 + 174)
    )
    assert room_opened > 500, (
        f"Room button did not open an interactive prop screen ({room_opened})"
    )
    # The original global mute lived only in Toybox. Room Tour now has a
    # clearly visible Y SOUND option. Test the *physical* DS Y key, and
    # ensure a second press restores the normal state without leaving ROOM.
    emu.input.keypad_add_key(keymask(Keys.KEY_Y))
    frames(emu, 6)
    emu.input.keypad_rm_key(keymask(Keys.KEY_Y))
    frames(emu, 12)
    room_muted = picture(emu, output, "game_room_audio_muted")
    room_mute_change = difference(
        room_night, room_muted, (112, 192 + 178, 187, 192 + 192)
    )
    assert room_mute_change > 12, (
        "Y mute did not change the visible Room Tour audio indicator "
        f"({room_mute_change} pixels)"
    )
    emu.input.keypad_add_key(keymask(Keys.KEY_Y))
    frames(emu, 6)
    emu.input.keypad_rm_key(keymask(Keys.KEY_Y))
    frames(emu, 12)
    room_unmuted = picture(emu, output, "game_room_audio_restored")
    room_restore_change = difference(
        room_muted, room_unmuted, (112, 192 + 178, 187, 192 + 192)
    )
    assert room_restore_change > 12, (
        "Y audio could not be switched back on in Room Tour "
        f"({room_restore_change} pixels)"
    )
    # Real stylus users must be able to tap the visible SOUND footer;
    # those without a stylus use Y. Both manipulate one shared mute flag.
    stylus_tap(emu, 180, 183)
    room_touch_muted = picture(emu, output, "game_room_audio_touch_muted")
    room_touch_mute_change = difference(
        room_unmuted, room_touch_muted, (148, 192 + 178, 207, 192 + 192)
    )
    assert room_touch_mute_change > 12, (
        "Tapping the visible sound label did not mute audio "
        f"({room_touch_mute_change} footer pixels)"
    )
    stylus_tap(emu, 180, 183)
    room_touch_restored = picture(emu, output, "game_room_audio_touch_restored")
    room_touch_restore_change = difference(
        room_touch_muted, room_touch_restored, (148, 192 + 178, 207, 192 + 192)
    )
    assert room_touch_restore_change > 12, (
        "Tapping the sound label again did not restore audio "
        f"({room_touch_restore_change} footer pixels)"
    )
    stylus_tap(emu, 35, 94)  # WINDOW: toggle daylight
    room_day = picture(emu, output, "game_room_tour_daylight")
    day_scene_change = difference(
        room_night, room_day, (0, 0, 256, 191)
    )
    assert day_scene_change > 45, (
        f"Window changed text but not 3D daylight ({day_scene_change})"
    )
    # Real sunlight must affect the nursery WALLS as well as the blue
    # glass. This right-hand ROI is separate from the window, guarding
    # against a fake day/night switch that recolors just the moon disk.
    daylight_room_change = difference(
        room_night, room_day, (154, 32, 245, 171)
    )
    assert daylight_room_change > 400, (
        "Daylight still changes only the window, not the room surfaces "
        f"({daylight_room_change} pixels)"
    )
    # The bottom status must reflect real prop state. This catches the
    # original bug where only the boot-time UI call supplied room flags,
    # leaving the *per-frame* Room panel stuck on MOONLIGHT/WINDOW.
    day_label_change = difference(
        room_night, room_day, (85, 192 + 49, 171, 192 + 59)
    )
    assert day_label_change > 20, (
        f"Window became daylight in 3D but Room label stayed stale "
        f"({day_label_change} label pixels)"
    )
    stylus_tap(emu, 35, 94)  # reset to the regular moonlit glass
    room_night_again = picture(emu, output, "game_room_tour_moon_return")
    night_scene_change = difference(
        room_day, room_night_again, (0, 0, 256, 191)
    )
    assert night_scene_change > 45, (
        f"Window cannot return to night scene ({night_scene_change})"
    )
    stylus_tap(emu, 180, 94)  # lamp goes dark, camera turns to dresser
    room_lamp_off = picture(emu, output, "game_room_tour_lamp_off")
    stylus_tap(emu, 180, 94)  # lamp comes back on, same camera position
    room_lamp_on = picture(emu, output, "game_room_tour_lamp_on")
    lamp_scene_change = difference(
        room_lamp_off, room_lamp_on, (0, 0, 256, 191)
    )
    assert lamp_scene_change > 45, (
        f"Lamp changes only a label, not actual room color ({lamp_scene_change})"
    )
    lamp_label_change = difference(
        room_lamp_off, room_lamp_on, (85, 192 + 49, 175, 192 + 59)
    )
    assert lamp_label_change > 20, (
        f"Lamp changes in the room, but the displayed ON/OFF state is stale "
        f"({lamp_label_change} label pixels)"
    )
    assert room_night.getpixel((91, 192 + 95)) != room_lamp_on.getpixel(
        (91, 192 + 95)
    ), "Selected WINDOW card stayed highlighted after switching to LAMP"
    stylus_tap(emu, 34, 139)  # TEDDY triggers the connected PLAY gesture
    room_teddy = picture(emu, output, "game_room_tour_teddy_play")
    teddy_scene_change = difference(
        room_lamp_on, room_teddy, (0, 0, 256, 191)
    )
    assert teddy_scene_change > 35, (
        f"Teddy doesn't visibly animate or focus ({teddy_scene_change})"
    )
    # Repeated prop selection must not leave Teddy waving endlessly.
    # After its 40-tick action the plush settles and remains in the room.
    frames(emu, 58)
    teddy_settled = picture(emu, output, "game_room_tour_teddy_settled")
    teddy_recovery_change = difference(
        room_teddy, teddy_settled, (8, 35, 160, 190)
    )
    assert teddy_recovery_change > 12, (
        f"Teddy never recovered to its rest pose ({teddy_recovery_change})"
    )
    # First create the real nighttime-care conditions: WINDOW at moonlight
    # (already returned to night above) + explicitly switch LAMP OFF.
    stylus_tap(emu, 180, 94)
    room_quiet_setup = picture(emu, output, "game_room_tour_quiet_setup")
    # CRIB now performs normal NAP and a conditional +8 energy/-4 chaos.
    stylus_tap(emu, 180, 139)
    room_crib = picture(emu, output, "game_room_tour_crib_nap")
    quiet_bonus_display = difference(
        room_quiet_setup, room_crib, (18, 192 + 58, 240, 192 + 72)
    )
    assert quiet_bonus_display > 30, (
        "Crib's quiet-night status was not displayed after lamp-off NAP "
        f"({quiet_bonus_display} changed pixels)"
    )
    crib_scene_change = difference(
        room_teddy, room_crib, (0, 0, 256, 191)
    )
    assert crib_scene_change > 35, (
        f"Crib doesn't animate a nap or change camera ({crib_scene_change})"
    )
    # The nursery mobile is now made of connected hanging pieces, and
    # responds ONCE to CRIB, before settling. Inspect both native top
    # captures to reject perpetually rocking or detached decorations.
    frames(emu, 7)
    mobile_sway = picture(emu, output, "game_room_crib_mobile_sway")
    # At half-closed NAP, the previous compressed eye meshes emitted long
    # turquoise and black horizontal bars over the baby's face. Do not
    # allow the restored 3D mesh to regress into those subpixel strips.
    def longest_eyelid_streak(img, kind):
        best = 0
        for yy in range(48, 94):
            length = 0
            for xx in range(51, 171):
                rr, gg, bb = img.getpixel((xx, yy))
                if kind == "cyan":
                    bad = (rr < 130 and gg < 160 and bb < 180
                           and gg > rr + 12 and bb > rr + 20)
                else:
                    bad = rr < 72 and gg < 80 and bb < 110
                length = length + 1 if bad else 0
                best = max(best, length)
        return best
    cyan_streak = longest_eyelid_streak(mobile_sway, "cyan")
    black_streak = longest_eyelid_streak(mobile_sway, "black")
    assert cyan_streak < 30 and black_streak < 30, (
        "NAP eyelids are still emitting long horizontal GL polygon stripes "
        f"(cyan={cyan_streak}, black={black_streak})"
    )
    frames(emu, 24)
    mobile_rest = picture(emu, output, "game_room_crib_mobile_settled")
    mobile_motion_pixels = difference(
        mobile_sway, mobile_rest, (178, 14, 255, 150)
    )
    assert mobile_motion_pixels > 12, (
        "The crib mobile has no visible motion in the actual DS nursery "
        f"({mobile_motion_pixels} changed pixels)"
    )
    stylus_tap(emu, 128, 183)  # visible BACK footer, never End Day!
    frames(emu, 20)
    room_return = picture(emu, output, "game_room_tour_back_to_care")
    room_back_change = difference(
        room_crib, room_return, (0, 192 + 28, 256, 192 + 175)
    )
    assert room_back_change > 500, (
        f"Room Tour trapped player or failed BACK ({room_back_change})"
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
    # An action should not leave the baby locked in a perpetual wave. The
    # in-game pose now eases back to idle after 56 actual game ticks.
    frames(emu, 155)
    chat_recovered = picture(emu, output, "game_nursery_chat_return_to_idle")
    recovered_change = difference(
        after_chat, chat_recovered, (34, 20, 222, 182)
    )
    assert recovered_change > 25, (
        f"CHAT never recovered to an idle pose ({recovered_change})"
    )
    # Exercise ACTUAL exhaustion through real gameplay: six PLAY taps
    # subtract 72 energy from the observed 93-energy post-CHAT state,
    # reaching 21. This crosses BOTH the gradual eye-lid threshold (55)
    # and the additional arm-slump threshold (25). A mere four taps
    # stopped at energy 45 and only verified a subtle eyelid response.
    # Let the one-shot PLAY reach gesture finish before photographing.
    for _ in range(6):
        stylus_tap(emu, 66, 160)
        frames(emu, 13)
    frames(emu, 80)
    tired_baby = picture(emu, output, "game_nursery_baby_low_energy")
    # Verify the gameplay state changed, not merely the baby's looping
    # breathing animation: the energy meter at x=61..132, y=83..91
    # MUST be shorter at <25 than the >70 healthy baseline. The sample
    # is deliberately left of the overlaid numeric counter.
    exhausted_energy_bar_pixels = difference(
        chat_recovered, tired_baby,
        (89, 192 + 85, 107, 192 + 89)
    )
    assert exhausted_energy_bar_pixels > 25, (
        "Exhaustion acting test did not actually drain the baby's "
        f"ENERGY bar ({exhausted_energy_bar_pixels} changed pixels)"
    )
    tired_expression_changed = difference(
        chat_recovered, tired_baby, (65, 28, 192, 146)
    )
    assert tired_expression_changed > 45, (
        "Six normal PLAY actions failed to change the actual top-screen "
        f"exhausted baby face/arms ({tired_expression_changed} pixels)"
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
    # A signed-angle bug previously flipped the entire infant during NAP,
    # leaving only a giant upside-down floating face. Require the actual
    # cyan romper in the LOWER half of the scene at native 256x192.
    nap_torso = after_nap.crop((78, 84, 176, 179))
    upright_romper_pixels = sum(
        1 for r, g, b in nap_torso.get_flattened_data()
        if g > r + 25 and b > r + 35 and b > 125
    )
    assert upright_romper_pixels > 600, (
        "NAP flipped the body out of frame or detached the head from the "
        f"upright romper ({upright_romper_pixels} cyan torso pixels)"
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
    # Two more actual touchscreen NEXT taps reach the first paid page.
    # Its center header must spell out exact WINS / dollars / FAVOR goals,
    # not merely show four vague LOCKED labels.
    stylus_tap(emu, 220, 44)
    stylus_tap(emu, 220, 44)
    arcade_locked = picture(emu, output, "game_arcade_paid_unlock_goals")
    arcade_goal_change = difference(
        arcade, arcade_locked, (70, 192 + 44, 188, 192 + 55)
    )
    goal_ink = arcade_locked.crop((70, 192 + 45, 188, 192 + 55))
    readable_goal_pixels = sum(
        1 for red, green, blue in goal_ink.get_flattened_data()
        if red > green + 55 and blue > green + 20 and red > 130
    )
    assert arcade_goal_change > 40 and readable_goal_pixels > 22, (
        "Locked arcade cabinet has no readable red unlock requirements "
        f"(changed={arcade_goal_change}, ink={readable_goal_pixels})"
    )
    stylus_tap(emu, 34, 44)
    stylus_tap(emu, 34, 44)
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
    # The legacy 4-tick redraw captured a half-painted scene with only
    # the pink wall and truncated STEP heading; verify the illustrated
    # changing mat is actually committed to VRAM before continuing.
    def diaper_art_colors(image):
        art = image.crop((48, 192 + 81, 208, 192 + 162))
        return len(set(art.get_flattened_data()))
    diaper_start_colors = diaper_art_colors(diaper)
    assert diaper_start_colors >= 9, (
        "Diaper changing mat is absent/partially repainted on the native "
        f"256x192 lower screen ({diaper_start_colors} colors)"
    )
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
    diaper_next_colors = diaper_art_colors(diaper_step2)
    assert diaper_next_colors >= 9, (
        "Diaper Dash cleared its changing mat during WIPE stage change "
        f"({diaper_next_colors} colors)"
    )
    step_changed = difference(
        diaper, diaper_step2, (0, 192 + 33, 256, 192 + 72)
    )
    assert step_changed > 30, (
        f"Diaper Dash did not visually advance after WIPE touch "
        f"(header changed={step_changed})"
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

    last_approach = None
    def seek_phase(target: int, name: str):
        nonlocal last_approach
        for _ in range(100):
            frames(emu, 12)
            sample = emu.screenshot().convert("RGB")
            current = movie_stage(sample)
            if target == 3 and current == 2:
                # Keep the LAST real pre-scare DS frame. Screenshot at the
                # normal cadence, rather than estimating film time from
                # wall-clock FPS or synthesizing a mockup.
                last_approach = sample
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

    # Archive a later buildup frame while the lens remains inside the egg.
    # Give the eye glow enough film ticks to change before taking it.
    frames(emu, 30)
    egg_anticipation = picture(emu, output, "game_egg_anticipation_close")
    assert movie_stage(egg_anticipation) == 2, (
        "Eye-anticipation frame exited the egg camera sequence"
    )
    egg_scare = seek_phase(3, "game_egg_jump_scare")
    assert last_approach is not None, (
        "Did not observe a late in-egg anticipation frame before scare"
    )
    assert movie_stage(last_approach) == 2
    last_approach.save(output / "game_egg_final_recoil_before_cut.png")
    # The former phase-2 acid drip was a giant rigid fluorescent bar
    # along the left edge in native DeSmuME (45 straight pixels).
    longest_egg_drip_streak = 0
    for xx in range(0, 31):
        run = 0
        for yy in range(110, 192):
            red, green, blue = last_approach.getpixel((xx, yy))
            if green > 210 and red < 70 and blue < 105:
                run += 1
                longest_egg_drip_streak = max(longest_egg_drip_streak, run)
            else:
                run = 0
    assert longest_egg_drip_streak < 22, (
        f"Phase-2 shell drip is still a fluorescent rectangle "
        f"({longest_egg_drip_streak}px)"
    )
    recoil_changed = difference(
        egg_anticipation, last_approach, (55, 18, 200, 172)
    )
    assert recoil_changed > 30, (
        f"Final egg approach/recoil appears visually frozen ({recoil_changed})"
    )
    scare_difference = difference(
        egg_push, egg_scare, (8, 7, 248, 182)
    )
    assert scare_difference > 300, (
        f"Egg film did not display its one hard-cut alien face ({scare_difference})"
    )
    assert movie_stage(egg_scare) == 3, (
        "The jump-scare image was captured in the wrong cinematic phase"
    )
    # The old eyebrow glint produced a 28-pixel fluorescent ruler at
    # the forehead of the REAL 256x192 DeSmuME image. New highlights are
    # short, broad and spatially separated; protect that improvement.
    brightest_streak = 0
    for yy in range(0, 30):
        continuous = 0
        for xx in range(75, 177):
            red, green, blue = egg_scare.getpixel((xx, yy))
            if green > 150 and green > red + 65 and green > blue + 50:
                continuous += 1
                if continuous > brightest_streak:
                    brightest_streak = continuous
            else:
                continuous = 0
    assert brightest_streak < 24, (
        f"Alien forehead has a straight fluorescent streak "
        f"({brightest_streak}px) instead of short brow highlights"
    )
    # The old flat slime rectangles created 46-pixel and 36-pixel
    # vertical luminous runs in the cropped native render. Organic
    # tapered cheek rivulets should not look like GPU seam artifacts.
    longest_vertical_ichor = 0
    for xx in list(range(20, 62)) + list(range(194, 237)):
        run = 0
        for yy in range(90, 173):
            red, green, blue = egg_scare.getpixel((xx, yy))
            if green > 155 and green > red + 65 and green > blue + 35:
                run += 1
                longest_vertical_ichor = max(longest_vertical_ichor, run)
            else:
                run = 0
    assert longest_vertical_ichor < 40, (
        f"Bright vertical cheek slime ruler survived "
        f"({longest_vertical_ichor}px)"
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
    # The former glRotateZ(degreesToAngle(...)) passed fixed-angle values
    # into a FLOAT-DEGREES API. The alien was rotated sideways, leaving
    # a vertical mouth and one eye below the other. Protect actual 3D
    # framing: TWO green eyes above ONE dark, centered mouth.
    left_iris = egg_scare.crop((65, 40, 111, 80))
    right_iris = egg_scare.crop((139, 40, 190, 80))
    def luminous_eye(img):
        return sum(
            1 for r, g, b in img.get_flattened_data()
            if g > r + 25 and g > b + 18 and g > 115
        )
    left_iris_pixels = luminous_eye(left_iris)
    right_iris_pixels = luminous_eye(right_iris)
    mouth_cut = egg_scare.crop((111, 108, 160, 150))
    central_mouth_dark = sum(
        1 for r, g, b in mouth_cut.get_flattened_data()
        if r < 50 and g < 70 and b < 85
    )
    assert left_iris_pixels > 250 and right_iris_pixels > 250, (
        "Alien eyes are no longer a centered horizontal pair: "
        f"left={left_iris_pixels}, right={right_iris_pixels}"
    )
    assert central_mouth_dark > 450, (
        "Alien skull tilted sideways; no deep mouth below the eyes "
        f"({central_mouth_dark} center dark pixels)"
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
        "Care_guidance_header_ink_pixels": care_coach_ink,
        "3D_top_screen_full_height_dark_stripes": len(full_height_stripes),
        "3D_nursery_dresser_streak_length": dresser_streak,
        "3D_nursery_wall_streak_length": wall_streak,
        "3D_nursery_green_shadow_artifact_pixels": olive_artifact,
        "Room_tour_open_changed_pixels": room_opened,
        "Room_audio_mute_changed_pixels": room_mute_change,
        "Room_audio_restore_changed_pixels": room_restore_change,
        "Room_audio_touch_mute_changed_pixels": room_touch_mute_change,
        "Room_audio_touch_restore_changed_pixels": room_touch_restore_change,
        "Room_daylight_top_changed_pixels": day_scene_change,
        "Room_daylight_wall_changed_pixels": daylight_room_change,
        "Room_daylight_status_changed_pixels": day_label_change,
        "Room_lamp_status_changed_pixels": lamp_label_change,
        "Room_moon_return_top_changed_pixels": night_scene_change,
        "Room_lamp_top_changed_pixels": lamp_scene_change,
        "Room_teddy_top_changed_pixels": teddy_scene_change,
        "Room_teddy_recovery_changed_pixels": teddy_recovery_change,
        "Room_quiet_crib_status_changed_pixels": quiet_bonus_display,
        "Room_crib_top_changed_pixels": crib_scene_change,
        "Room_crib_mobile_changed_pixels": mobile_motion_pixels,
        "Room_NAP_cyan_eye_streak_pixels": cyan_streak,
        "Room_NAP_black_eye_streak_pixels": black_streak,
        "Room_back_to_care_changed_pixels": room_back_change,
        "3D_care_pose_changed_pixels": pose_changed,
        "3D_nap_upright_romper_pixels": upright_romper_pixels,
        "3D_chat_reach_changed_pixels": chat_changed,
        "3D_chat_recovery_changed_pixels": recovered_change,
        "3D_low_energy_baby_changed_pixels": tired_expression_changed,
        "low_energy_meter_changed_pixels": exhausted_energy_bar_pixels,
        "3D_title_top_screen_colors": title_colors,
        "album_screen_changed_pixels": album_diff,
        "album_next_page_changed_pixels": album_next_diff,
        "album_previous_page_changed_pixels": album_previous_diff,
        "cabinet_art_distinct_colors": icon_colors,
        "cabinet_name_ink_pixels": name_ink,
        "arcade_next_page_changed_pixels": page_changed,
        "arcade_previous_page_changed_pixels": page_returned,
        "arcade_paid_requirement_changed_pixels": arcade_goal_change,
        "arcade_paid_requirement_visible_ink": readable_goal_pixels,
        "diaper_heading_ink_pixels": dark_ink,
        "diaper_start_illustration_colors": diaper_start_colors,
        "diaper_after_wipe_illustration_colors": diaper_next_colors,
        "title_to_diagnostic_changed_pixels": new_screen,
        "live_geometry_vertex_label_pixels": visible_vertices,
        "live_geometry_polygon_label_pixels": visible_polygons,
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
        "Alien_earliest_reveal_brow_streak_pixels": brightest_streak,
        "Alien_vertical_ichor_streak_pixels": longest_vertical_ichor,
        "Egg_pre_scare_motion_changed_pixels": recoil_changed,
        "Egg_rectangular_ichor_streak_pixels": longest_egg_drip_streak,
        "Jump_scare_porcelain_left_pixels": scare_shell_left,
        "Jump_scare_porcelain_right_pixels": scare_shell_right,
        "Jump_scare_right_eye_pixels": right_iris_pixels,
        "Jump_scare_center_mouth_dark_pixels": central_mouth_dark,
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
