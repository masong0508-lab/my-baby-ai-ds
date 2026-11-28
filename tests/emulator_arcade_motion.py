#!/usr/bin/env python3
"""Complete the four moving/timing starter cabinets in the production ROM.

All choices come from actual DeSmuME screenshots. The player uses native
stylus/keypad input, never game RAM, save edits, RNG prediction, or debug
unlocks. Companion emulator_arcade.py covers Sock, Beatbox and Teether;
emulator_touch.py covers Diaper. Together they exercise all eight starters.

    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy python \
        tests/emulator_arcade_motion.py my-baby-ai-blocksds-ds-only.nds \
        --output emulator-starter-proof
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import time
import traceback
from pathlib import Path

from desmume.controls import Keys, keymask

from emulator_arcade import LANE_KEYS, Player, color_at, progress, text_at


CABINETS = {
    0: ("BOTTLE BALANCE", "BOTTLE BALANCE", 15, 6),
    3: ("CRIB KART", "CRIB KART", 12, 6),
    6: ("NAPTIME NEGOTIATOR", "NAPTIME", 13, 4),
    7: ("RATTLE RESCUE", "RATTLE RESCUE", 13, 6),
}
PINK5 = (28, 11, 18)
SUN5 = (31, 28, 16)
GOLD5 = (30, 21, 9)


def rgb5(image, x, y):
    return tuple(channel >> 3 for channel in image.getpixel((x, y + 192)))


def marker(image, y, start, end, width):
    columns = [x for x in range(start, end) if rgb5(image, x, y) == PINK5]
    assert len(columns) == width, f"Incomplete timing marker: {columns}"
    assert columns == list(range(columns[0], columns[0] + width)), columns
    return columns[0]


def status(image, cabinet):
    _, title, x, goal = CABINETS[cabinet]
    assert text_at(image, x, 35, len(title), 2) == title
    done, mistakes = progress(image, goal)
    if cabinet == 0:
        line = text_at(image, 83, 115, 27)
        match = re.fullmatch(r"STEADY (\d{1,3})/150", line)
        assert match, line
        steady = int(match[1])
        assert 0 <= steady < 150 and done == steady * 6 // 150, (done, line)
        return {"round": done, "mistakes": mistakes, "steady": steady,
                "marker_x": marker(image, 139, 90, 166, 5) + 2}
    x, y, ink, prefix, suffix = {
        3: (12, 54, "deep_pink", "SAFE", "CRASH"),
        6: (13, 53, "deep_pink", "CALM", "WAKEUPS"),
        7: (13, 53, "teal", "CAUGHT", "MISSED"),
    }[cabinet]
    line = text_at(image, x, y, 39, ink=ink)
    assert line == f"{prefix} {done}/{goal}  {suffix} {mistakes}/3", line
    state = {"round": done, "mistakes": mistakes}
    if cabinet == 6:
        state["marker_x"] = marker(image, 147, 20, 239, 4)
        # Read only the text itself: the teal crib border is within the
        # old 19-character OCR span and is not part of the readiness cue.
        state["cue"] = text_at(image, 82, 134, 14)
        if state["cue"] not in ("WAIT FOR GREEN", "SWEET DREAMS!"):
            state["cue"] = text_at(image, 82, 134, 8, ink="teal")
        assert state["cue"] in ("WAIT FOR GREEN", "TAP NOW!", "SWEET DREAMS!"), state
    return state


class MotionPlayer(Player):
    def __init__(self, path, output):
        super().__init__(path, output)
        self.steering = None

    def observe(self, description, predicate=lambda state: True, limit=300):
        image = self.wait(description, lambda im:
                          predicate(status(im, self.selected)), limit=limit, stride=2)
        return image, status(image, self.selected)

    def expect_progress(self, rounds, mistakes, goal=None, limit=900):
        image, _ = self.observe(f"{rounds} rounds and {mistakes} mistakes", lambda s:
                                (s["round"], s["mistakes"]) == (rounds, mistakes),
                                limit=limit)
        return image

    def select(self, cabinet):
        self.menu()
        while self.selected != cabinet:
            distance = (cabinet - self.selected) % 16
            if 4 <= distance <= 12:
                self.key(Keys.KEY_R)
                self.selected = (self.selected + 4) % 16
            elif distance <= 8:
                self.key(Keys.KEY_RIGHT)
                self.selected = (self.selected + 1) % 16
            else:
                self.key(Keys.KEY_LEFT)
                self.selected = (self.selected - 1) % 16
        self.menu()
        # Require the newly selected card, not just the unchanged menu
        # header/resource counters during an in-progress bitmap repaint.
        return self.wait("selected " + CABINETS[cabinet][0], lambda im:
                         text_at(im, 12, 60, 38) == CABINETS[cabinet][0],
                         limit=60, stride=1)

    def start(self, cabinet):
        self.select(cabinet)
        self.key(Keys.KEY_A)
        return self.expect_progress(0, 0)

    def await_menu(self, limit=900):
        return self.wait("return to arcade", lambda im:
                         text_at(im, 12, 7, 6, 2, "white") == "ARCADE",
                         limit=limit)

    def steer(self, direction, method="keypad"):
        """Continuous Bottle input; zero releases both real input devices."""
        wanted = (direction, method)
        if self.steering == wanted:
            return
        self.emu.input.keypad_rm_key(keymask(Keys.KEY_LEFT) | keymask(Keys.KEY_RIGHT))
        self.emu.input.touch_release()
        if direction:
            if method == "stylus":
                self.emu.input.touch_set_pos(55 if direction < 0 else 195, 155)
            else:
                key = Keys.KEY_LEFT if direction < 0 else Keys.KEY_RIGHT
                self.emu.input.keypad_add_key(keymask(key))
        self.steering = wanted


def centered_direction(x, center=128):
    return -1 if x > center + 2 else 1 if x < center - 2 else 0


def complete_bottle(player):
    peak, outside_decay, checked_band = 0, None, False
    samples, devices = [], set()
    phase = "build"
    deadline = player.cycles + 4800
    while player.cycles < deadline:
        image = player.screen()
        if text_at(image, 12, 7, 6, 2, "white") == "ARCADE":
            player.steer(0)
            assert outside_decay is not None, "Bottle finished before testing recovery"
            assert checked_band, "Safe band never became readable"
            assert devices == {"keypad", "stylus"}, devices
            player.win("bottle_recovery_win")
            # A held steering control must not navigate the menu when
            # the final stability tick automatically ends the cabinet.
            player.step(8)
            settled = player.menu()
            assert text_at(settled, 12, 60, 38) == CABINETS[0][0], (
                "Bottle steering leaked into menu navigation")
            player.capture("bottle_menu_selection_preserved", settled)
            return {"loss_and_restart": True, "outside_band_decay": outside_decay,
                    "safe_band_checked": checked_band, "controls": sorted(devices),
                    "visible_steady_samples": samples}
        try:
            state = status(image, 0)
        except AssertionError:
            player.step(1)
            continue
        assert state["mistakes"] == 0, state
        x, steady = state["marker_x"], state["steady"]
        if not checked_band and 125 <= x <= 131:
            # The drawn mint band must agree with the real stability window.
            # Sampling outside the centered marker avoids its five-pixel mask.
            assert color_at(image, 116, 136) != "mint", "Bottle safe band is too wide"
            assert color_at(image, 117, 136) == "mint", "Missing left safe boundary"
            assert color_at(image, 139, 136) == "mint", "Missing right safe boundary"
            assert color_at(image, 140, 136) != "mint", "Bottle safe band is too wide"
            checked_band = True
            player.capture("bottle_safe_band_matches_rules", image, **state)
        if phase == "build" and steady >= 45:
            peak = steady
            phase = "leave_band"
            player.capture("bottle_before_outside_excursion", image, **state)
        if phase == "leave_band":
            peak = max(peak, steady)
        if phase == "leave_band" and x >= 142 and steady < peak:
            outside_decay = {"before": peak, "after": steady, "marker_x": x}
            player.capture("bottle_outside_band_loses_steady", image, **outside_decay)
            phase = "recover"
        direction = 1 if phase == "leave_band" else centered_direction(x)
        method = "stylus" if phase == "recover" and steady >= 75 else "keypad"
        player.steer(direction, method)
        if direction:
            devices.add(method)
        if not samples or steady >= samples[-1]["steady"] + 25:
            samples.append({"steady": steady, "marker_x": x})
            player.capture(f"bottle_steady_{steady:03}", image, **state)
        player.step(2)
    raise AssertionError("Bottle did not complete within the bounded native play session")


def test_bottle(player):
    player.capture("bottle_start", player.start(0))
    # Continuous native stylus pressure in one direction must spill three
    # times and return to the arcade without paying a win.
    player.steer(1, "stylus")
    seen = set()
    for _ in range(900):
        image = player.screen()
        if text_at(image, 12, 7, 6, 2, "white") == "ARCADE":
            break
        try:
            state = status(image, 0)
        except AssertionError:
            player.step(1)
            continue
        if state["mistakes"] and state["mistakes"] not in seen:
            seen.add(state["mistakes"])
            player.capture(f"bottle_spill_{state['mistakes']}", image, **state)
        player.step(2)
    else:
        raise AssertionError("Deliberate Bottle spills never ended the game")
    player.steer(0)
    # Give the actual release an input poll before issuing a new press.
    # Do not bypass the post-result guard or alter cabinet scoring.
    player.step(8)
    assert seen == {1, 2}, seen
    player.capture("bottle_loss_without_reward", player.menu())
    player.capture("bottle_restart_clears_spills", player.start(0))
    return complete_bottle(player)


def kart_scene(image):
    counts = []
    for lane in range(3):
        counts.append(sum(rgb5(image, x, y) == GOLD5
                          for y in range(81, 137)
                          for x in range(32 + lane * 82, 61 + lane * 82)))
    cones = [i for i, count in enumerate(counts) if count >= 12]
    cars = [i for i in range(3) if color_at(image, 20 + i * 82, 164) == "mint"]
    assert len(cones) == 1 and len(cars) == 1, (counts, cars)
    return cones[0], cars[0]


def test_kart(player):
    image = player.start(3)
    cone, _ = kart_scene(image)
    player.capture("kart_first_cone", image, obstacle_lane=cone)
    player.tap(46 + cone * 82, 150)
    image, state = player.observe("car deliberately enters obstacle lane", lambda s:
                                  s["round"] == 0 and s["mistakes"] == 0)
    assert kart_scene(image)[1] == cone
    player.capture("kart_steering_does_not_score", image, **state)
    player.capture("kart_collision_retry", player.expect_progress(0, 1))
    safe_lanes = []
    for round_number in range(6):
        image = player.expect_progress(round_number, 1)
        cone, _ = kart_scene(image)
        lane = (cone + 1) % 3
        safe_lanes.append({"obstacle": cone, "steer_to": lane})
        if round_number % 2:
            player.key(LANE_KEYS[lane])
        else:
            player.tap(46 + lane * 82, 150)
        image, state = player.observe("visible safe steering", lambda s:
                                      s["round"] == round_number and s["mistakes"] == 1)
        assert kart_scene(image)[1] == lane
        player.capture(f"kart_avoid_cone_{round_number + 1}", image,
                       obstacle_lane=cone, player_lane=lane, **state)
        if round_number < 5:
            player.expect_progress(round_number + 1, 1)
    player.await_menu()
    player.win("kart_collision_recovery_win")
    return {"collision_retry": True, "steering_alone_does_not_score": True,
            "native_stylus_and_dpad": True, "visible_obstacles": safe_lanes}


def test_nap(player):
    image = player.start(6)
    initial = status(image, 6)
    assert initial["marker_x"] < 90, initial
    player.capture("nap_early_marker", image, **initial)
    player.tap(128, 155)
    player.capture("nap_early_tap_wakes_baby", player.expect_progress(0, 1))
    windows = []
    for calm in range(4):
        image, state = player.observe("unscored green timing window", lambda s:
            s["round"] == calm and s["mistakes"] == 1 and
            s["cue"] == "TAP NOW!" and 121 <= s["marker_x"] <= 139,
            limit=1000)
        # Both sides of the visible marker belong to the mint timing band.
        assert color_at(image, 110, 155) == "mint"
        assert color_at(image, 160, 155) == "mint"
        windows.append(state["marker_x"])
        player.capture(f"nap_green_window_{calm + 1}", image, **state)
        if calm % 2:
            player.key(Keys.KEY_A)
        else:
            player.tap(128, 155)
        if calm < 3:
            image = player.expect_progress(calm + 1, 1)
            if calm == 0:
                state = status(image, 6)
                assert state["cue"] == "SWEET DREAMS!", state
                assert state["marker_x"] < 150, state
                player.key(Keys.KEY_A)
                player.capture("nap_one_success_per_window",
                               player.expect_progress(1, 1))
    player.await_menu()
    player.win("nap_timing_recovery_win")
    return {"early_tap_mistake": True, "one_success_per_window": True,
            "native_stylus_and_A": True, "visible_hit_positions": windows}


def rattle_scene(image):
    lanes = [i for i in range(3) if color_at(image, 18 + i * 82, 98) == "white"]
    assert len(lanes) == 1, lanes
    lane = lanes[0]
    # The rattle's sun-yellow face is distinct from its gold catch zone.
    # Read its actual falling position; no timer or random state is used.
    rows = [y for y in range(80, 164)
            if any(rgb5(image, x, y) == SUN5
                   for x in range(33 + lane * 82, 62 + lane * 82))]
    assert rows, "Falling rattle face is not visible"
    edge = color_at(image, 18 + lane * 82, 130)
    fill = color_at(image, 18 + lane * 82, 145)
    assert (edge, fill) in (("gold", "lemon"), ("green", "mint")), (edge, fill)
    return lane, min(rows)


def test_rattle(player):
    image = player.start(7)
    lane, face_y = rattle_scene(image)
    assert face_y < 110, face_y
    player.capture("rattle_too_early", image, lane=lane, face_top_y=face_y)
    player.tap(46 + lane * 82, 145)
    player.capture("rattle_early_catch_retry", player.expect_progress(0, 1))
    # Also let one replacement rattle fall completely out of reach.
    player.capture("rattle_timeout_retry", player.expect_progress(0, 2, limit=1300))
    catches = []
    for caught in range(6):
        def catchable(image):
            state = status(image, 7)
            assert state == {"round": caught, "mistakes": 2}, state
            lane, face = rattle_scene(image)
            return (132 <= face <= 140 and
                    color_at(image, 18 + lane * 82, 145) == "mint")

        image = player.wait("rattle face enters the gold catch zone", catchable,
                            limit=1100, stride=2)
        lane, face_y = rattle_scene(image)
        catches.append({"lane": lane, "face_top_y": face_y})
        player.capture(f"rattle_in_catch_zone_{caught + 1}", image,
                       lane=lane, face_top_y=face_y)
        if caught % 2:
            player.key(LANE_KEYS[lane])
        else:
            player.tap(46 + lane * 82, 145)
        if caught < 5:
            player.expect_progress(caught + 1, 2)
    player.await_menu()
    player.win("rattle_timing_recovery_win")
    return {"early_catch_rejected": True, "timeout_retry": True,
            "native_stylus_and_dpad": True, "visible_catches": catches}


TESTS = {"bottle": test_bottle, "kart": test_kart,
         "nap": test_nap, "rattle": test_rattle}


def run(path, output, stage):
    player = MotionPlayer(path, output)
    started = time.perf_counter()
    report = {"rom_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
              "emulator": "DeSmuME 0.9.12 / py-desmume 0.0.9", "stage": stage,
              "input": "native stylus and keypad; rendered pixels only",
              "result": "FAIL"}
    try:
        player.step(120)
        player.capture("title")
        player.tap(128, 115)
        player.wait("nursery", lambda im:
                    text_at(im, 12, 7, 4, 2, "white") == "CARE")
        player.tap(45, 183)
        player.capture("arcade_initial_resources", player.menu())
        for name, scenario in TESTS.items():
            if stage == "all" or stage == name:
                report[name] = scenario(player)
        report["final_resources"] = {"wins": player.wins, "dollars": player.dollars,
                                     "favors": player.favors}
        report["result"] = "PASS"
    except Exception:
        report["error"] = traceback.format_exc()
        player.capture("failure")
        raise
    finally:
        player.steer(0)
        report["emulator_cycles"] = player.cycles
        report["elapsed_seconds"] = round(time.perf_counter() - started, 3)
        report["events"] = player.events
        (output / "starter-report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "events"}, indent=2),
          flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path)
    parser.add_argument("--output", type=Path, default=Path("emulator-starter-proof"))
    parser.add_argument("--stage", choices=("all", *TESTS), default="all")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    run(args.rom, args.output, args.stage)


if __name__ == "__main__":
    main()
