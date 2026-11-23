#!/usr/bin/env python3
"""Exercise WARDEN, typed learning and hostile counters in the production ROM.

Only native stylus/key events and rendered DS pixels are used. No emulator
memory, injected saves, hidden state, engine calls or generated-sentence
oracle are available to this test. Host tests cover the complete Game/RNG,
model mathematics and persistence contracts separately.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import time
import traceback
from pathlib import Path

from desmume.controls import Keys

from emulator_arcade import Player, color_at, text_at
from emulator_conversation import care_reply, care_values, header_values, readable


ALPHA = "QWERTYUIOPASDFGHJKL?ZXCVBNM .<"
NUMERIC = "1234567890+-*/()=?%.ZXCVBNM .<"
PHOSPHOR = (24 << 3, 31 << 3, 26 << 3)
STAGES = {"FOUND", "WATCHING", "HOSTILE", "REVEALED"}
CARE_KEYS = {"FEED": Keys.KEY_A, "CHAT": Keys.KEY_B,
             "PLAY": Keys.KEY_X, "NAP": Keys.KEY_Y}


def normalized(text):
    return " ".join(text.split())


def key_position(index):
    assert 0 <= index < 30
    return 19 + index % 10 * 24, 125 + index // 10 * 16


def observer_values(image):
    title = readable(text_at(image, 12, 7, 7, 2, "white"))
    assert title in ("MONITOR", "BABY"), title
    stage = readable(text_at(image, 190, 92, 9, ink=PHOSPHOR))
    assert stage in STAGES, stage
    integrity = readable(text_at(image, 167, 34, 13, ink=PHOSPHOR))
    match = re.fullmatch(r"BRACER (\d{1,3})%", integrity)
    assert match and 0 <= int(match[1]) <= 100, integrity
    rows = [readable(text_at(image, 17, y, 36, ink=PHOSPHOR))
            for y in (45, 56, 67, 78)]
    page = readable(text_at(image, 223, 104, 3, ink="ink"))
    assert page in ("123", "ABC"), page
    selected = [index for index in range(30)
                if color_at(image, 10 + index % 10 * 24,
                            129 + index // 10 * 16) == "green"]
    assert len(selected) == 1, selected
    keyboard = NUMERIC if page == "ABC" else ALPHA
    for index, character in enumerate(keyboard):
        x = 16 + index % 10 * 24
        y = 121 + index // 10 * 16
        actual = readable(text_at(image, x - 3 if character == " " else x,
                                   y, 2 if character == " " else 1, ink="ink"))
        assert actual == ("SP" if character == " " else character), (index, actual)
    return {**header_values(image), "speaker": title, "stage": stage,
            "integrity": int(match[1]), "input": readable(text_at(
                image, 17, 104, 32, ink="ink")),
            "reply_rows": rows, "reply": normalized(" ".join(rows)),
            "keyboard_page": "numeric" if page == "ABC" else "alpha",
            "selected_key": selected[0],
            "reply_at_display_limit": text_at(image, 137, 34, 3, ink="lemon") == "..."}


def parse_status(values):
    match = re.fullmatch(
        r"(FOUND|WATCHING|HOSTILE|REVEALED)\. DAY (\d+)\. "
        r"CARE F(\d+) C(\d+) P(\d+) N(\d+)\. "
        r"INTEGRITY (\d+)\. HITS (\d+)\. ASK NEEDS, MEMORY OR HELP\.",
        values["reply"])
    assert match, values
    fields = [int(value) for value in match.groups()[1:]]
    return dict(zip(("day", "feeds", "chats", "plays", "naps", "integrity", "hits"), fields),
                stage=match[1])


def parse_attacks(values):
    match = re.fullmatch(
        r"STAGE (FOUND|WATCHING|HOSTILE|REVEALED)\. "
        r"WINDUPS (\d+)\. HITS (\d+)\. BLOCKS (\d+)\. INTEGRITY (\d+)\. "
        r"GUARD THE WINDUP\. WARDEN CAN REPAIR YOUR BRACER\.", values["reply"])
    assert match, values
    return {"stage": match[1], "windups": int(match[2]), "hits": int(match[3]),
            "blocks": int(match[4]), "integrity": int(match[5])}


class ObserverPlayer(Player):
    def __init__(self, path, output):
        super().__init__(path, output)
        self.input_shadow = ""
        self.sent = False

    def terminal(self, description, speaker=None, input_text=None, reply=None):
        def matches(image):
            value = observer_values(image)
            return ((speaker is None or value["speaker"] == speaker) and
                    (input_text is None or value["input"] == input_text.strip()) and
                    (reply is None or reply(value["reply"])))

        image = self.wait(description, matches, limit=240)
        return image, observer_values(image)

    def care(self, description):
        image = self.wait(description, lambda im: bool(care_values(im)), limit=240)
        return image, care_values(image)

    def enter(self, method, name):
        if method == "touch":
            self.tap(104, 13)
        else:
            self.key(Keys.KEY_UP)
        self.input_shadow = ""
        self.sent = False
        image, values = self.terminal(name, "MONITOR", "")
        self.capture(name, image, **values)
        return values

    def leave(self, method, name, expected=None):
        if method == "keypad":
            self.key(Keys.KEY_B)
        else:
            self.tap(30, 183)
        image, values = self.care(name)
        if expected is not None:
            assert values == expected, {"before": expected, "after": values}
        self.capture(name, image, **values)
        return values

    def page(self, numeric, method="touch"):
        _, state = self.terminal("keyboard before page selection")
        wanted = "numeric" if numeric else "alpha"
        if state["keyboard_page"] == wanted:
            return
        if method == "keypad":
            self.key(Keys.KEY_SELECT)
        else:
            self.tap(231, 107)
        self.wait("keyboard page switch", lambda im:
                  observer_values(im)["keyboard_page"] == wanted, limit=120)

    def erase(self):
        self.key(Keys.KEY_L)
        self.input_shadow = "" if self.sent else self.input_shadow[:-1]
        self.sent = False
        self.terminal("one backspace", input_text=self.input_shadow)

    def write(self, text):
        assert 0 < len(text) <= 32 and text == text.upper(), text
        if not self.sent:
            while self.input_shadow:
                self.erase()
        for character in text:
            _, state = self.terminal("keyboard before typing")
            current = NUMERIC if state["keyboard_page"] == "numeric" else ALPHA
            if character not in current:
                self.page(character in NUMERIC)
                current = NUMERIC if character in NUMERIC else ALPHA
            assert character in current, character
            if character == " ":
                self.key(Keys.KEY_R)  # Exercise the physical SPACE shortcut.
            else:
                self.tap(*key_position(current.index(character)))
            if self.sent:
                self.input_shadow = ""
                self.sent = False
            self.input_shadow += character
            self.terminal(f"typed {self.input_shadow!r}", input_text=self.input_shadow)

    def ask(self, text, name, predicate=None, method="keypad"):
        self.write(text)
        if method == "touch":
            self.tap(224, 183)
        elif method == "start":
            self.key(Keys.KEY_START)
        else:
            self.key(Keys.KEY_X)
        self.sent = True
        image, values = self.terminal(name, input_text=text, reply=predicate or bool)
        self.capture(name, image, **values)
        return values

    def switch_speaker(self, target, method, name):
        if method == "touch":
            self.tap(96, 183)
        else:
            self.key(Keys.KEY_Y)
        image, values = self.terminal(name, target)
        self.capture(name, image, **values)
        return values

    def wait_windup(self, name, view="CARE", limit=1200):
        def matches(image):
            return (text_at(image, 12, 7, len(view), 2, "white") == view and
                    text_at(image, 88, 9, 5, ink="ink") == "GUARD" and
                    text_at(image, 10, 181, 39, ink="aqua") ==
                    "WINDUP! TAP GUARD / RIGHT")

        image = self.wait(name, matches, limit=limit, stride=2)
        # The GUARD redraw can precede the upper LCD's eased anticipation
        # pose. Keep the real warning visible for a bounded interval so
        # evidence shows its narrowed eyes and connected arm reach. This
        # is still inside the observed native windup; assert that the
        # warning survives before allowing the test's actual counter.
        self.step(36)
        image = self.screen()
        assert matches(image), "Windup ended before its pose evidence was captured"
        self.capture(name, image, view=view, warning="WINDUP! TAP GUARD / RIGHT",
                     pose_capture_delay_cycles=36)
        return image


def typing_scenario(player, initial):
    entry = player.enter("touch", "monitor_CPU_touch_entry")
    assert parse_status(entry) == {"stage": "FOUND", "day": 1, "feeds": 0,
                                   "chats": 0, "plays": 0, "naps": 0,
                                   "integrity": 100, "hits": 0}
    original_reply = entry["reply"]
    # Microphone features are opt-in; DeSmuME can test the real UI and
    # cancellation even when the host provides no microphone waveform.
    player.tap(150, 94)
    screen = player.wait("native MIC LAB opens", lambda im:
                         text_at(im, 12, 7, 7, 2, "white") == "MIC LAB",
                         limit=120)
    player.capture("microphone_lab_keyboard_fallback", screen)
    # The brand-new learner starts empty. Its browser and delete controls
    # must be visible and not delete anything until an explicit lesson
    # is selected AND DEL is confirmed a second time.
    assert text_at(screen, 13, 117, 4, ink="ink") == "PREV"
    assert text_at(screen, 53, 117, 4, ink="ink") == "NEXT"
    assert text_at(screen, 217, 117, 3, ink="ink") == "DEL"
    assert text_at(screen, 94, 117, 17, ink="ink").startswith("NO LESSONS")
    player.tap(22, 120)
    browse = player.wait("empty voice browser is harmless", lambda im:
                         text_at(im, 17, 88, 36, ink="ink").startswith(
                             "NO SAVED WORDS."), limit=120)
    player.capture("microphone_empty_library_browse", browse)
    player.tap(222, 120)
    empty = player.wait("empty delete cannot erase other progress", lambda im:
                        text_at(im, 17, 88, 36, ink="ink").startswith(
                            "NO SAVED WORDS TO FORGET."), limit=120)
    assert text_at(empty, 17, 166, 25, ink="white") == "SAVED LABELS 0 / 24"
    player.capture("microphone_empty_library_delete_inert", empty)
    player.key(Keys.KEY_X)
    player.step(25)
    player.capture("microphone_listening_without_injected_audio", player.screen())
    # A silent hardware/emulator source must complete without inventing a
    # voice training example or leaving an endless mic session running.
    # This also exercises the fallback when no mic service exists.
    silent = player.wait("silent MIC capture ends without training", lambda im:
                         text_at(im, 17, 88, 36, ink="ink").startswith(
                             ("NO CLEAR SPEECH.", "MIC UNAVAILABLE.")),
                         limit=260, stride=4)
    count = text_at(silent, 17, 166, 25, ink="white")
    assert count == "SAVED LABELS 0 / 24", count
    player.capture("microphone_silence_timeout_no_phantom_word", silent,
                   learned_labels=0,
                   status=text_at(silent, 17, 88, 36, ink="ink"))
    player.key(Keys.KEY_B)
    _, returned = player.terminal("MIC B returns to WARDEN", input_text="")
    assert parse_status(returned) == parse_status(entry), returned
    player.key(Keys.KEY_RIGHT)
    _, selected = player.terminal("D-pad selection does not type", input_text="")
    assert selected["selected_key"] == 1 and selected["reply"] == original_reply
    player.key(Keys.KEY_A)
    player.input_shadow = "W"
    image, typed = player.terminal("A types exactly the selected W", input_text="W")
    assert typed["reply"] == original_reply
    player.capture("monitor_Dpad_A_types_one_character", image, **typed)
    player.erase()

    # A stylus held over Q must be a single event, not auto-repeat.
    player.emu.input.touch_set_pos(*key_position(0))
    player.step(12)
    player.input_shadow = "Q"
    image, held = player.terminal("first held Q", input_text="Q")
    started = player.cycles
    for delta in (5, 7, 11, 13, 17, 19, 23):
        player.step(delta)
        image, current = player.terminal("held Q stays singular", input_text="Q")
        assert current["reply"] == held["reply"]
    player.capture("monitor_held_key_does_not_repeat", image,
                   held_cycles=player.cycles-started, observations=7, **current)
    player.emu.input.touch_release()
    player.step(8)
    player.erase()
    player.page(True, "keypad")
    image, numeric = player.terminal("SELECT opens numeric keyboard", input_text="")
    player.capture("monitor_numeric_keyboard_SELECT", image, **numeric)
    calculation = player.ask("12+7", "monitor_real_numeric_calculation",
                              lambda text: text == "12 + 7 = 19.", "touch")
    status = player.ask("STATUS", "monitor_initial_factual_STATUS")
    assert parse_status(status) == parse_status(entry)
    world = player.ask("WORLD", "monitor_world_lore", lambda text:
                       "CONTINUITY BUREAU" in text and "FORBIDDEN BABY" in text)
    assert "HOW MANY SURVIVE" in world["reply"], world
    trust = player.ask("TRUST", "monitor_initial_unmeasured_trust", lambda text:
                       "NO CARE INTERACTIONS RECORDED" in text and
                       "TRUST IS NOT A MEASURED METER" in text)
    unknown = player.ask("QUANTUM CUCUMBER", "monitor_honest_unknown", lambda text:
                         "DO NOT UNDERSTAND" in text and "ARCHIVE HAS NO ANSWER" in text)

    player.switch_speaker("BABY", "keypad", "monitor_Y_selects_BABY")
    first_bond = player.ask("BOND", "baby_initial_relationship", lambda text:
                            "WE JUST MET" in text and "YOUR VOICE" in text)
    player.write("TINY BOSS")
    taught = []
    for lesson in range(1, 4):
        player.tap(160, 183)
        player.sent = True
        image, value = player.terminal(f"teaching lesson {lesson}", "BABY", "TINY BOSS",
                                      lambda text: "PHRASE" in text)
        assert ("LEARNED" if lesson == 1 else "REINFORCED") in value["reply"], value
        player.capture(f"baby_teach_TINY_BOSS_{lesson}", image, **value)
        taught.append(value["reply"])
    echo = player.ask("SAY WHAT I TAUGHT", "baby_retrieves_taught_phrase",
                      lambda text: text == "YOU TAUGHT ME: TINY BOSS")
    slots = player.ask("WHAT DID I TEACH", "baby_duplicate_teaching_one_slot",
                       lambda text: text == "1: TINY BOSS")
    hello = player.ask("HELLO", "baby_generated_hello")
    dream = player.ask("DREAM", "baby_generated_dream",
                       lambda text: text.startswith("IMAGINED DREAM:"))
    assert len(hello["reply"].split()) >= 3 and len(dream["reply"].split()) >= 4
    player.switch_speaker("MONITOR", "touch", "monitor_footer_selects_WARDEN")
    monitor_hello = player.ask("HELLO", "monitor_generated_hello", method="start")
    assert len(monitor_hello["reply"].split()) >= 3
    player.leave("keypad", "monitor_B_returns_without_care_cost", initial)
    player.enter("keypad", "monitor_CARE_UP_entry")
    player.leave("touch", "monitor_footer_BACK_preserves_meters", initial)

    # The learned phrase must be picked up in an actual healthy PLAY,
    # not only returned when explicitly requested at the terminal.
    player.key(Keys.KEY_X)
    image, played = player.care("real PLAY after three lessons")
    assert {key: played[key] for key in ("hunger", "mood", "energy", "chaos")} == {
        "hunger": 47, "mood": 85, "energy": 63, "chaos": 24}
    assert care_reply(image) == "YOU TAUGHT ME: TINY BOSS", care_reply(image)
    player.capture("baby_spontaneous_learned_phrase_during_PLAY", image,
                   reply=care_reply(image), **played)
    return {"entry_methods": ["CPU touch", "CARE UP"],
            "exit_methods": ["B", "BACK touch"], "calculator": calculation["reply"],
            "world": world["reply"], "unknown": unknown["reply"],
            "monitor_initial_TRUST": trust["reply"],
            "baby_initial_BOND": first_bond["reply"],
            "teaching_acknowledgements": taught, "echo": echo["reply"],
            "one_taught_slot_after_three_lessons": slots["reply"],
            "generated_BABY_hello": hello["reply"],
            "generated_BABY_dream": dream["reply"],
            "generated_WARDEN_hello": monitor_hello["reply"],
            "spontaneous_phrase_during_actual_PLAY": care_reply(image),
            "terminal_preserves_visible_Game_state": True}


def hostility_scenario(player):
    # First PLAY already occurred above. These 63 further real actions
    # bring all four factual care counters to exactly sixteen.
    counts = {name: 0 for name in CARE_KEYS}
    counts["PLAY"] = 1
    actions = ["FEED", "CHAT", "PLAY", "NAP"] * 15 + ["FEED", "CHAT", "NAP"]
    for index, action in enumerate(actions, 2):
        player.key(CARE_KEYS[action])
        counts[action] += 1
        image, state = player.care(f"actual care event {index}: {action}")
        if index % 16 == 0:
            player.capture(f"care_observation_milestone_{index}", image, counts=dict(counts), **state)
    assert set(counts.values()) == {16}, counts
    after_care = state
    before = player.enter("keypad", "monitor_HOSTILE_from_64_real_care_events")
    profile = parse_status(before)
    assert profile == {"stage": "HOSTILE", "day": 1, "feeds": 16, "chats": 16,
                       "plays": 16, "naps": 16, "integrity": 100, "hits": 0}, profile
    hostile_bond = player.ask("BOND", "monitor_hostile_bond_counts", lambda text:
                              "CARE F16 C16 P16 N16." in text and
                              "STAGE HOSTILE" in text and
                              "SAFETY IS NOT GUARANTEED" in text)
    attacks = parse_attacks(player.ask("ATTACKS", "monitor_initial_hostile_attack_log"))
    assert attacks["windups"] in (0, 1) and attacks["hits"] == attacks["blocks"] == 0
    player.step(660)
    paused = parse_attacks(player.ask("ATTACKS", "monitor_pauses_hostile_clock"))
    assert paused == attacks, (attacks, paused)
    player.leave("keypad", "care_ready_for_first_guard", after_care)
    player.wait_windup("care_visible_hostile_windup")
    player.tap(104, 13)
    image = player.wait("GUARD blocks the windup", lambda im:
                        care_reply(im).startswith("ATTACK BLOCKED."), limit=180)
    assert care_values(image) == after_care
    player.capture("care_touch_GUARD_blocks_attack", image, reply=care_reply(image),
                   **care_values(image))
    player.enter("keypad", "monitor_after_touch_GUARD")
    blocked = parse_attacks(player.ask("ATTACKS", "monitor_truthful_block_record"))
    assert blocked == {"stage": "HOSTILE", "windups": 1, "hits": 0,
                       "blocks": 1, "integrity": 100}, blocked
    player.leave("keypad", "care_before_unblocked_attack", after_care)
    player.wait_windup("care_second_hostile_windup")
    image = player.wait("unblocked attack lands", lambda im:
                        care_reply(im).startswith("YOUR BRACER TOOK A HIT!"), limit=360, stride=2)
    assert care_values(image) == after_care
    # Capture the lunge after its connected rig has eased into the strike,
    # not the first bitmap frame that announces the already-recorded hit.
    player.step(32)
    image = player.screen()
    assert care_reply(image).startswith("YOUR BRACER TOOK A HIT!")
    assert care_values(image) == after_care
    player.capture("care_actual_unblocked_hit_and_lunge", image,
                   reply=care_reply(image), pose_capture_delay_cycles=32,
                   **care_values(image))
    player.enter("keypad", "monitor_after_actual_hit")
    hit = parse_attacks(player.ask("ATTACKS", "monitor_truthful_damage_record"))
    assert hit == {"stage": "HOSTILE", "windups": 2, "hits": 1,
                   "blocks": 1, "integrity": 92}, hit
    repair = player.ask("REPAIR", "monitor_REPAIR_restores_bracer", lambda text:
                        text.startswith("BRACER REPAIRED. INTEGRITY 100."))
    assert repair["integrity"] == 100
    repaired = parse_attacks(player.ask("ATTACKS", "monitor_repair_retains_observed_history"))
    assert repaired == {**hit, "integrity": 100}

    # Two successful toy inputs remain below the six-hit secret threshold.
    # Each occurs during a visible windup and must count as a real counter.
    toy_logs = []
    for number, tool in enumerate(("SLAP", "KICK"), 1):
        player.leave("keypad", f"care_before_{tool}_counter", after_care)
        player.tap(128, 183)
        image = player.wait("TOYBOX entry", lambda im:
                            text_at(im, 12, 7, 6, 2, "white") == "TOYBOX", limit=240)
        player.capture(f"toybox_before_{tool}_windup", image)
        player.wait_windup(f"toybox_visible_{tool}_windup", "TOYBOX", limit=2400)
        player.key(Keys.KEY_L if tool == "SLAP" else Keys.KEY_R)
        player.key(Keys.KEY_A)
        image = player.wait(f"successful native {tool}", lambda im:
                            text_at(im, 177, 42, 10, ink="rose") == f"HITS {number}/6" and
                            text_at(im, 10, 181, 39, ink="aqua").startswith("ATTACK BLOCKED."),
                            limit=180)
        player.capture(f"toybox_actual_{tool}_counter", image, shell_hits=number)
        player.key(Keys.KEY_B)
        player.care(f"CARE after {tool}")
        player.enter("keypad", f"monitor_after_{tool}_counter")
        log = parse_attacks(player.ask("ATTACKS", f"monitor_{tool}_counter_record"))
        assert log == {"stage": "HOSTILE", "windups": 2 + number, "hits": 1,
                       "blocks": 1 + number, "integrity": 100}, log
        toy = player.ask("TOYBOX", f"monitor_actual_{tool}_input_totals")
        assert toy["reply"].startswith(
            f"CONNECTED IN TOYBOX: 1 SLAPS, {number-1} KICKS."), toy
        toy_logs.append({"tool": tool, "attacks": log, "counts": toy["reply"]})
    final = parse_status(player.ask("STATUS", "monitor_final_truthful_profile"))
    assert final == {**profile, "hits": 1}, final
    player.leave("keypad", "care_final_stats_preserved_after_guard_hits_and_repair", after_care)
    return {"actual_care_events": 64, "care_counts": counts,
            "HOSTILE_profile": profile, "monitor_hostile_BOND": hostile_bond["reply"],
            "paused_terminal_cycles": 660,
            "attack_log_before_pause": attacks, "attack_log_after_pause": paused,
            "touch_GUARD": blocked, "unblocked_hit": hit,
            "repair_reply": repair["reply"], "repair_preserves_history": repaired,
            "TOYBOX_counters": toy_logs, "final_profile": final,
            "care_meters_unchanged_by_terminal_and_attacks": after_care}


def run(path, output):
    started = time.perf_counter()
    player = None
    source_root = Path(__file__).resolve().parents[1]
    report = {"rom_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
              "emulator": "DeSmuME 0.9.12 / py-desmume 0.0.9",
              "input": "native stylus and keypad; rendered pixels only", "result": "FAIL",
              "source_sha256": {name: hashlib.sha256((source_root / name).read_bytes()).hexdigest()
                                for name in ("source/observer.c", "source/narrative_lm.c",
                                             "source/ui.c", "source/main.c")}}
    try:
        player = ObserverPlayer(path, output)
        player.step(120)
        image = player.capture("title_hidden_shelter_premise")
        assert text_at(image, 18, 76, 10, ink="rose") == "INFANTS"
        assert text_at(image, 18, 87, 10, ink="rose") == "OUTLAWED"
        assert text_at(image, 178, 76, 10, ink="teal") == "WARDEN"
        player.tap(80, 153)  # The visible START card, never SAVE/LOAD.
        image, initial = player.care("initial shelter nursery")
        assert {key: initial[key] for key in ("day", "dollars", "hunger", "mood", "energy", "chaos")} == {
            "day": 1, "dollars": 8, "hunger": 35, "mood": 75, "energy": 75, "chaos": 12}
        player.capture("care_initial_shelter_graphics", image, **initial)
        report["typed_terminal"] = typing_scenario(player, initial)
        report["observed_hostility"] = hostility_scenario(player)
        report["scope"] = (
            "Native pixels establish input, visible care/profile facts, learned phrase use, "
            "guarded attacks, repair and readable generated replies. They do not prove model "
            "quality, hidden Game/RNG isolation, FAT persistence, physical DS performance or FPS.")
        report["result"] = "PASS"
    except Exception:
        report["error"] = traceback.format_exc()
        if player is not None:
            player.capture("failure")
        raise
    finally:
        if player is not None:
            player.emu.input.touch_release()
            report["emulator_cycles"] = player.cycles
            report["events"] = player.events
        report["elapsed_seconds"] = round(time.perf_counter() - started, 3)
        (output / "observer-report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({key: value for key, value in report.items() if key != "events"},
                     indent=2), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path)
    parser.add_argument("--output", type=Path, default=Path("emulator-observer-proof"))
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    run(args.rom, args.output)


if __name__ == "__main__":
    main()
