#!/usr/bin/env python3
"""Play real arcade cabinets in the production ROM through DeSmuME.

This test reads only the rendered screens. It does not inspect emulated
memory, predict the random generator, patch saves, or unlock debug content.
Native stylus/key events earn six wins and buy Cereal Cipher in the menu.
Screenshots and a JSON transcript preserve the visible questions/results.

Run with SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy:
    python tests/emulator_arcade.py my-baby-ai-blocksds-ds-only.nds \
        --output emulator-arcade-proof
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
from desmume.emulator import DeSmuME
from PIL import ImageChops

from emulator_pixels import read_text


COLORS = {
    "white": (248, 248, 248),
    "pink": (248, 152, 200),
    "teal": (96, 192, 208),
    "mint": (176, 240, 184),
    "lemon": (248, 224, 104),
    "gold": (240, 168, 72),
    "green": (96, 200, 120),
    "silver": (184, 200, 240),
    "rose": (240, 104, 176),
    "gray": (208, 200, 216),
    "sky": (160, 208, 248),
}
TITLES = {2: "TAX TEETHER", 4: "BABY BEATBOX", 5: "SOCK MARKET",
          10: "CEREAL CIPHER"}
NAMES = {2: "TAX RETURN TEETHER", 4: "BABY BEATBOX", 5: "SOCK MARKET",
         10: "CEREAL CIPHER"}
LANE_KEYS = (Keys.KEY_LEFT, Keys.KEY_DOWN, Keys.KEY_RIGHT)
BEAT_KEYS = (Keys.KEY_UP, Keys.KEY_RIGHT, Keys.KEY_DOWN, Keys.KEY_LEFT)


def text_at(image, x, y, max_chars=36, scale=1, ink="purple"):
    """All gameplay coordinates below are local to the touch screen."""
    return read_text(image, x, y + 192, scale=scale,
                     max_chars=max_chars, ink=ink).strip()


def color_at(image, x, y):
    actual = image.getpixel((x, y + 192))
    return next((name for name, rgb in COLORS.items()
                 if tuple(v >> 3 for v in rgb) == tuple(v >> 3 for v in actual)),
                str(actual))


def progress(image, goal):
    rounds = [color_at(image, 16 + i * 12, 73) for i in range(goal)]
    misses = [color_at(image, 214 + i * 11, 73) for i in range(3)]
    assert all(c in ("green", "teal") for c in rounds), rounds
    assert all(c in ("rose", "silver") for c in misses), misses
    done, mistakes = rounds.count("green"), misses.count("rose")
    assert rounds == ["green"] * done + ["teal"] * (goal - done), rounds
    assert misses == ["rose"] * mistakes + ["silver"] * (3 - mistakes), misses
    return done, mistakes


class Player:
    def __init__(self, path, output):
        self.emu = DeSmuME()
        self.emu.open(str(path.resolve()))
        self.output = output
        self.cycles = 0
        self.selected = 0
        self.wins, self.dollars, self.favors = 0, 8, 1
        self.events = []

    def step(self, count):
        for _ in range(count):
            self.emu.cycle(with_joystick=False)
        self.cycles += count

    def screen(self):
        image = self.emu.screenshot().convert("RGB")
        assert image.size == (256, 384), image.size
        return image

    def capture(self, name, image=None, **facts):
        image = self.screen() if image is None else image
        image.save(self.output / f"{name}.png")
        self.events.append({"screen": f"{name}.png", "cycles": self.cycles,
                            **facts})
        print(name, facts, flush=True)
        return image

    def tap(self, x, y):
        self.emu.input.touch_set_pos(x, y)
        self.step(8)
        self.emu.input.touch_release()
        self.step(8)

    def key(self, key):
        self.emu.input.keypad_add_key(keymask(key))
        self.step(8)
        self.emu.input.keypad_rm_key(keymask(key))
        self.step(8)

    def wait(self, description, predicate, limit=180, stride=3):
        """Bounded emulator cycles, with no assumptions about game tick rate."""
        for _ in range(0, limit + 1, stride):
            image = self.screen()
            try:
                if predicate(image):
                    return image
            except AssertionError:
                # A bitmap redraw can span native frames; require the whole
                # expected UI, rather than accepting a half-painted screen.
                pass
            self.step(stride)
        raise AssertionError(f"Timed out waiting for {description} ({limit} cycles)")

    def expect_progress(self, rounds, mistakes, goal=6, limit=180):
        def matches(image):
            if progress(image, goal) != (rounds, mistakes):
                return False
            x, y = (13, 53) if self.selected == 2 else (12, 53)
            if self.selected in (4, 10):
                y = 54
            status = text_at(image, x, y, 38, ink="deep_pink")
            if self.selected == 2:
                return bool(re.fullmatch(rf"SORTED {rounds}/6   [1-4] SEC", status))
            expected = {
                4: f"TUNE {rounds + 1}/3  MISS {mistakes}/3",
                5: f"PAIRS {rounds}/6  MISS {mistakes}/3",
                10: f"SOLVED {rounds}/6  WRONG {mistakes}/3",
            }[self.selected]
            return status == expected

        return self.wait(f"progress {rounds}/{goal}, misses {mistakes}/3",
                         matches, limit=limit)

    def menu(self):
        return self.wait(f"arcade menu with {self.wins} wins, ${self.dollars}, "
                         f"{self.favors} favors", lambda im:
                         text_at(im, 12, 7, 6, 2, "white") == "ARCADE" and
                         text_at(im, 71, 34, 20) ==
                         f"WINS {self.wins} FAVORS {self.favors}" and
                         text_at(im, 214, 10, 5, ink="ink") == f"${self.dollars}")

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
        image = self.menu()
        assert text_at(image, 12, 60, 38) == NAMES[cabinet]
        return image

    def start(self, cabinet):
        self.select(cabinet)
        self.key(Keys.KEY_A)
        image = self.wait(TITLES[cabinet], lambda im:
                          text_at(im, 12 if cabinet != 2 else 13,
                                  35, len(TITLES[cabinet]), 2) == TITLES[cabinet])
        return self.expect_progress(0, 0, goal=3 if cabinet == 4 else 6)

    def win(self, name):
        self.wins += 1
        self.dollars += 5
        if not self.wins % 2:
            self.favors += 1
        image = self.menu()
        self.capture(name, image, wins=self.wins, dollars=self.dollars,
                     favors=self.favors)

    def drag(self, end_x, end_y, name=None, start=(128, 123)):
        before = self.wait("complete Teether screen before drag", teether_frame_ready,
                           limit=60, stride=1) if name else self.screen()
        self.emu.input.touch_set_pos(*start)
        self.step(8)
        self.emu.input.touch_set_pos((start[0] + end_x) // 2,
                                     (start[1] + end_y) // 2)
        self.step(8)
        self.emu.input.touch_set_pos(end_x, end_y)
        # Bitmap redraws can outlast one input poll. Keep the endpoint held
        # until the next native UI refresh can show the dragged receipt.
        self.step(24)
        if name:
            held_at = self.cycles
            # A fixed hold can end during a VRAM repaint. Wait for valid
            # title, status, counters and trays, without requesting any
            # particular score; early scoring must fail the next assert.
            held = self.wait("complete Teether screen while stylus remains held",
                             teether_frame_ready, limit=60, stride=1)
            assert progress(held, 6) == progress(before, 6), (
                "Receipt was scored before the stylus released")
            delta = ImageChops.difference(before, held).crop(
                (end_x - 14, 192 + end_y - 13,
                 end_x + 14, 192 + end_y + 13))
            moved = sum(pixel != (0, 0, 0) for pixel in delta.getdata())
            assert moved > 50, f"Dragged receipt did not visibly move: {moved}"
            self.capture(name, held, changed_receipt_pixels=moved,
                         held_frame_wait_cycles=self.cycles - held_at,
                         progress_before=list(progress(before, 6)),
                         progress_while_held=list(progress(held, 6)))
        self.emu.input.touch_release()
        self.step(8)


def sock_answer(image):
    # Sample the full first motif, not one pixel which can be white
    # in the new checker pattern. Verify shape as well as hue.
    def motif(x, y):
        palette = {"lemon", "pink", "teal"}
        cells = [[color_at(image, x + 2 * col, y + 2 * row)
                  for col in range(3)] for row in range(3)]
        hues = {c for row in cells for c in row if c in palette}
        assert len(hues) == 1, cells
        hue = hues.pop()
        shape = tuple(tuple(c == hue for c in row) for row in cells)
        expected = {
            "pink": ((True, True, True), (True, True, True), (False, False, False)),
            "teal": ((True, True, False), (True, True, False), (False, False, False)),
            "lemon": ((True, False, True), (True, True, True), (False, True, False)),
        }
        assert shape == expected[hue], (hue, cells)
        return hue
    target = motif(63, 85)
    choices = [motif(40 + i * 82, 131) for i in range(3)]
    assert sorted(choices) == ["lemon", "pink", "teal"], choices
    assert target in choices, (target, choices)
    return choices.index(target), {"stripe": target, "choices": choices}


FEEDBACK = {
    "progress": ("NICE! KEEP GOING.", "mint"),
    "miss": ("OOPS! TRY AGAIN.", "pink"),
    "win": ("YOU WON! GREAT JOB.", "lemon"),
    "loss": ("ROUND LOST. TRY AGAIN.", "white"),
}


def expect_feedback(player, kind, name):
    caption, ink = FEEDBACK[kind]
    image = player.wait(f"visible {kind} feedback", lambda im:
                        text_at(im, 10, 181, 39, ink=ink) == caption,
                        limit=60, stride=2)
    player.capture(name, image, feedback=caption)
    return image


def result_feedback(player, kind):
    """Verify outcome text, archive finite acting, then check footer recovery.

    The exact joint paths and interruption continuity have host coverage.
    These are native ROM frames for the corresponding visible result;
    the image difference guards against a completely frozen reaction.
    """
    expect_feedback(player, kind, f"arcade_{kind}_feedback")
    player.step(16)
    acting = player.capture(f"arcade_{kind}_baby_reaction")
    player.step(160)
    settled = player.menu()
    assert text_at(settled, 12, 181, 6, ink="white") == "A PLAY"
    assert text_at(settled, 199, 181, 6, ink="white") == "B BACK"
    delta = ImageChops.difference(acting, settled).crop((54, 65, 198, 179))
    changed = sum(pixel != (0, 0, 0) for pixel in delta.getdata())
    assert changed > 80, f"The baby never recovered from its {kind} reaction: {changed}"
    player.capture(f"arcade_{kind}_baby_settled", settled,
                   reaction_changed_pixels=changed, footer_restored=True)
    return {"caption": FEEDBACK[kind][0], "footer_restored": True,
            "reaction_changed_pixels": changed}


def play_socks(player, name, mistakes=0, verify_feedback=False):
    answers = []
    for round_number in range(6):
        image = player.expect_progress(round_number, mistakes)
        lane, facts = sock_answer(image)
        answers.append(facts)
        if round_number % 2:
            player.key(LANE_KEYS[lane])
        else:
            player.tap(46 + lane * 82, 145)
        if round_number < 5:
            player.expect_progress(round_number + 1, mistakes)
            if verify_feedback and round_number == 0:
                expect_feedback(player, "progress", "sock_correct_feedback")
    player.win(name)
    return answers


def test_socks(player):
    player.capture("sock_start", player.start(5))
    # Three visibly wrong choices must end the game without paying a win.
    for mistake in range(3):
        lane, facts = sock_answer(player.screen())
        wrong = (lane + 1) % 3
        player.tap(46 + wrong * 82, 145)
        if mistake < 2:
            player.capture(f"sock_wrong_{mistake + 1}",
                           player.expect_progress(0, mistake + 1), **facts)
            if mistake == 0:
                expect_feedback(player, "miss", "sock_mistake_feedback")
    player.capture("sock_loss_returns_to_menu", player.menu(), wins=0, dollars=8)
    loss_reaction = result_feedback(player, "loss")
    player.capture("sock_retry_resets_counters", player.start(5))
    # Exercise the real no-input timeout and then use its replacement puzzle.
    timed_out = player.expect_progress(0, 1, limit=1400)
    player.capture("sock_timeout_retry", timed_out, **sock_answer(timed_out)[1])
    answers = play_socks(player, "sock_recovery_win", mistakes=1, verify_feedback=True)
    win_reaction = result_feedback(player, "win")
    return {"wrong_choice_loss": True, "restart_reset": True,
            "timeout_recovery_win": True, "visible_answers": answers,
            "loss_reaction": loss_reaction, "win_reaction": win_reaction,
            "progress_and_mistake_feedback": True,
            "native_touch_and_dpad": True}


def beat_phase(image):
    color = color_at(image, 68, 88)
    assert color in ("mint", "lemon"), color
    return "repeat" if color == "mint" else "watch"


def watch_tune(player, name, rounds, mistakes, ignored_tap=False):
    sequence = [None, None, None]
    captured = set()
    for _ in range(600):
        image = player.screen()
        try:
            observed_progress = progress(image, 3)
            phase = beat_phase(image)
            if phase == "repeat":
                assert text_at(image, 86, 83, 6, 2, "ink") == "REPEAT"
            else:
                assert text_at(image, 94, 83, 5, 2, "ink") == "WATCH"
        except AssertionError:
            # A screenshot may catch the bitmap renderer between its
            # background clear and final progress rule. Read a complete
            # frame before trying to interpret the sequence.
            player.step(1)
            continue
        assert observed_progress == (rounds, mistakes), observed_progress
        if phase == "repeat":
            assert all(note is not None for note in sequence), sequence
            player.capture(f"{name}_repeat", image, notes=sequence)
            return sequence
        markers = [i for i in range(3)
                   if color_at(image, 148 + i * 18, 108) == "gold"]
        active = [i for i in range(4)
                  if color_at(image, 16 + i * 62, 130) == "white"]
        if len(markers) == 1 and len(active) == 1:
            index, pad = markers[0], active[0]
            assert sequence[index] in (None, pad), (sequence, index, pad)
            sequence[index] = pad
            if index not in captured:
                captured.add(index)
                player.capture(f"{name}_watch_{index + 1}", image,
                               sequence_position=index + 1, lit_pad=pad)
            if ignored_tap:
                # Tapping while WATCH is displayed must neither score nor
                # cause a mistake. The subsequent full tune still must win.
                player.tap(36 + ((pad + 1) % 4) * 62, 145)
                ignored_tap = False
                assert progress(player.screen(), 3) == (rounds, mistakes)
        player.step(3)
    raise AssertionError("Beatbox never changed WATCH into REPEAT")


def test_beatbox(player):
    player.capture("beatbox_start", player.start(4))
    preview = watch_tune(player, "beatbox_first", 0, 0, ignored_tap=True)
    player.tap(36 + ((preview[0] + 1) % 4) * 62, 145)
    player.capture("beatbox_wrong_note_restarts_watch",
                   player.expect_progress(0, 1, goal=3))
    played = []
    for tune in range(3):
        sequence = watch_tune(player, f"beatbox_tune_{tune + 1}", tune, 1)
        played.append(sequence)
        for position, pad in enumerate(sequence):
            if position == 1:
                player.key(BEAT_KEYS[pad])
            else:
                player.tap(36 + pad * 62, 145)
            if position < 2:
                image = player.wait("accepted Beatbox note with fully drawn progress", lambda im:
                    all(color_at(im, 148 + i * 18, 108) == "green"
                        for i in range(position + 1)) and
                    progress(im, 3) == (tune, 1))
                # The renderer updates the two UI regions separately. Do not
                # accept the new note until BOTH the note markers and the
                # complete score/mistake HUD are visible together.
                assert progress(image, 3) == (tune, 1)
        if tune < 2:
            player.expect_progress(tune + 1, 1, goal=3)
    player.win("beatbox_recovery_win")
    return {"watch_tap_ignored": True, "wrong_note_retry": True,
            "visible_tunes": played, "native_touch_and_dpad": True}


def teether_destination(image):
    cue = text_at(image, 10, 77, 35)
    if cue == "REFUND: SAVE IT!":
        assert color_at(image, 14, 151) == "lemon"
        assert color_at(image, 164, 151) == "sky"
        return 50, cue
    assert cue == "JUNK BILL: SHRED!", cue
    assert color_at(image, 14, 151) == "sky"
    assert color_at(image, 164, 151) == "pink"
    return 202, cue


def teether_frame_ready(image):
    """Recognize complete cabinet paint, independent of the expected score."""
    rounds, _ = progress(image, 6)
    if text_at(image, 13, 35, 11, 2) != "TAX TEETHER":
        return False
    if not re.fullmatch(rf"SORTED {rounds}/6   [1-4] SEC",
                        text_at(image, 13, 53, 38, ink="deep_pink")):
        return False
    teether_destination(image)  # Complete cue and both tray colors.
    return True


def test_teether(player):
    player.capture("teether_start", player.start(2))
    # Neither a gesture originating outside the receipt, nor a receipt
    # released outside both trays, is a wrong answer.
    player.drag(50, 148, start=(25, 100))
    player.expect_progress(0, 0)
    player.drag(128, 166)
    player.capture("teether_invalid_releases_ignored", player.expect_progress(0, 0))
    destination, _ = teether_destination(player.screen())
    player.drag(202 if destination == 50 else 50, 148)
    player.capture("teether_wrong_bin_retry", player.expect_progress(0, 1))
    receipts = []
    for receipt in range(6):
        image = player.expect_progress(receipt, 1)
        destination, cue = teether_destination(image)
        receipts.append(cue)
        player.drag(destination, 148,
                    name="teether_paper_follows_stylus" if receipt == 0 else None)
        if receipt < 5:
            player.capture(f"teether_sorted_{receipt + 1}",
                           player.expect_progress(receipt + 1, 1), cue=cue)
    player.win("teether_recovery_win")
    return {"invalid_source_ignored": True, "outside_drop_ignored": True,
            "wrong_bin_retry": True, "score_on_release": True,
            "visible_receipts": receipts, "native_drag_win": True}


def cereal_answer(image):
    question = text_at(image, 83, 91, 18)
    match = re.fullmatch(r"([1-4]) \+ ([1-4]) = \?", question)
    assert match, f"Invalid visible Cereal question after retry: {question!r}"
    total = sum(map(int, match.groups()))
    choices = [int(text_at(image, 41 + i * 82, 139, 2, ink="ink"))
               for i in range(3)]
    assert choices.count(total) == 1, (question, choices)
    assert choices == list(range(choices[0], choices[0] + 3)), choices
    return choices.index(total), {"question": question, "choices": choices}


def test_earned_cereal(player):
    # These are six actual correct choices per game, not save/memory edits.
    for extra in range(3):
        player.start(5)
        play_socks(player, f"earned_win_{extra + 4}")
    locked = player.select(10)
    assert text_at(locked, 71, 46, 20, ink="rose") == "6 WINS $14 2 FAVOR"
    assert text_at(locked, 58, 151, 10, ink="rose") == "LOCKED"
    player.capture("cereal_earned_unlock_requirements", locked,
                   wins=player.wins, dollars=player.dollars, favors=player.favors)
    # Locked A must not begin the cabinet, even after earning requirements.
    player.key(Keys.KEY_A)
    player.menu()
    player.key(Keys.KEY_X)
    player.dollars -= 14
    player.favors -= 2
    bought = player.menu()
    assert text_at(bought, 80, 46, 23, ink="teal") == "READY TO PLAY"
    assert text_at(bought, 58, 151, 10, ink="teal") == "READY"
    player.capture("cereal_bought_with_earned_currency", bought,
                   dollars=player.dollars, favors=player.favors)
    player.capture("cereal_start", player.start(10))
    # Both timeout branches must redraw a valid sum with a matching answer.
    # The old generic question reset made these retries impossible to play.
    retries = []
    for miss in (1, 2):
        image = player.expect_progress(0, miss, limit=1400)
        _, facts = cereal_answer(image)
        retries.append(facts)
        player.capture(f"cereal_timeout_retry_{miss}", image, **facts)
    solved = []
    for question in range(6):
        image = player.expect_progress(question, 2)
        lane, facts = cereal_answer(image)
        solved.append(facts)
        player.tap(46 + lane * 82, 145)
        if question < 5:
            player.capture(f"cereal_solved_{question + 1}",
                           player.expect_progress(question + 1, 2), **facts)
    player.win("cereal_timeout_recovery_win")
    return {"wins_earned_before_purchase": 6, "unlock_cost_dollars": 14,
            "unlock_cost_favors": 2, "locked_start_rejected": True,
            "visible_timeout_questions": retries, "visible_solved_questions": solved,
            "native_touch_win_after_two_timeouts": True}


def run(path, output):
    player = Player(path, output)
    started = time.perf_counter()
    results = {"rom_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
               "emulator": "DeSmuME 0.9.12 / py-desmume 0.0.9",
               "input": "native stylus and keypad only; screen pixels only",
               "result": "FAIL"}
    try:
        player.step(120)
        player.capture("title")
        player.tap(128, 115)
        player.wait("nursery", lambda im:
                    text_at(im, 12, 7, 4, 2, "white") == "CARE")
        player.tap(45, 183)
        player.capture("arcade_fresh_resources", player.menu())
        results["sock"] = test_socks(player)
        results["beatbox"] = test_beatbox(player)
        results["teether"] = test_teether(player)
        results["cereal"] = test_earned_cereal(player)
        results["final_resources"] = {"wins": player.wins, "dollars": player.dollars,
                                      "favors": player.favors}
        results["result"] = "PASS"
    except Exception:
        results["error"] = traceback.format_exc()
        player.capture("failure")
        raise
    finally:
        results["emulator_cycles"] = player.cycles
        results["elapsed_seconds"] = round(time.perf_counter() - started, 3)
        results["events"] = player.events
        (output / "arcade-report.json").write_text(json.dumps(results, indent=2) + "\n")
    print(json.dumps({key: value for key, value in results.items() if key != "events"},
                     indent=2), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path)
    parser.add_argument("--output", type=Path, default=Path("emulator-arcade-proof"))
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    run(args.rom, args.output)


if __name__ == "__main__":
    main()
