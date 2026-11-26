#!/usr/bin/env python3
"""Verify the production TALK flow through real DeSmuME input and pixels.

The test reads the game's bitmap font, authored reply catalog and visible
care meters. It never
reads emulator RAM, edits a save, predicts gameplay RNG, or calls the
conversation engine directly. Full Game/RNG isolation belongs to the host
engine tests; native assertions cover the state the player can see.
"""
from __future__ import annotations

import argparse
import ast
import hashlib
import json
import re
import time
import traceback
from pathlib import Path

from desmume.controls import Keys

from emulator_arcade import Player, color_at, text_at


PROMPTS = (
    "HOW ARE YOU?", "NEED ANYTHING?", "REMEMBER?",
    "TELL ME MORE", "I LIKE THAT", "NEW SUBJECT",
)
LEGACY_FEED_MEMORY = {
    "THAT BOTTLE HIT THE SPOT. THANK YOU.",
    "THE LAST BOTTLE EARNED FIVE STARS.",
}
# These are the independently agreed semantic expectations for the real
# care/input sequence below. Only authored wording is loaded from source;
# the engine's topic selector, learned weights and variation RNG are not run.
REPLY_TOPICS = {
    "initial_how": ("CONVERSATION_FEEL_OK",),
    "empty_memory": ("CONVERSATION_NO_MEMORY",),
    "feed_care": ("CONVERSATION_MEMORY_FEED",),
    "fed_how": ("CONVERSATION_MEMORY_FEED",),
    "aged_how": ("CONVERSATION_FEEL_OK",),
    "feed_remember": ("CONVERSATION_MEMORY_FEED",),
    "feed_more": ("CONVERSATION_MEMORY_FEED",),
    "like_ack": ("CONVERSATION_ACK_LIKE",),
    "already_liked": ("CONVERSATION_ACK_RATED",),
    "new_subject": ("CONVERSATION_IMAGINE_TEDDY", "CONVERSATION_IMAGINE_DREAMS",
                    "CONVERSATION_IMAGINE_WORLD"),
    "healthy_need": ("CONVERSATION_NEED_NONE",),
    "urgent_nap": ("CONVERSATION_NEED_REST",),
    "tired_play_care": ("CONVERSATION_NEED_REST",),
    "nap_care": ("CONVERSATION_CARE_THANKS",),
}


def normalized(text):
    return " ".join(text.split())


def load_reply_contract(source=None):
    source = source or Path(__file__).resolve().parents[1] / "source" / "conversation.c"
    authored = source.read_text()
    literal = r'"(?:[^"\\]|\\.)*"'
    entry = re.compile(r'R\(\s*(CONVERSATION_[A-Z_]+)\s*,\s*'
                       r'(CONVERSATION_[A-Z_]+)\s*,\s*([01])\s*,\s*'
                       rf'((?:{literal}\s*)+)\)', re.DOTALL)
    families = {}
    for match in entry.finditer(authored):
        topic, style, variant, strings = match.groups()
        reply = "".join(ast.literal_eval(part)
                        for part in re.findall(literal, strings, re.DOTALL))
        rows = reply.splitlines()
        assert 1 <= len(rows) <= 2 and all(len(row) <= 36 for row in rows), (
            topic, style, variant, rows)
        families.setdefault(topic, set()).add(normalized(reply))
    required_topics = set().union(*REPLY_TOPICS.values())
    for topic in required_topics:
        assert len(families.get(topic, ())) == 6, (
            f"Expected six authored style/variant responses for {topic}",
            families.get(topic))
    contract = {name: set().union(*(families[topic] for topic in topics))
                for name, topics in REPLY_TOPICS.items()}
    return contract, {"source": "source/conversation.c",
                      "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
                      "semantic_families": REPLY_TOPICS,
                      "authored_choices": {name: len(replies)
                                            for name, replies in contract.items()}}


def readable(text):
    assert "\ufffd" not in text, f"Incomplete or unsupported rendered glyph: {text!r}"
    return text


def card_position(index):
    assert 0 <= index < len(PROMPTS)
    return 66 + (index % 2) * 124, 104 + (index // 2) * 27


def header_values(image):
    day = readable(text_at(image, 173, 10, 4, ink="ink"))
    money = readable(text_at(image, 214, 10, 5, ink="ink"))
    assert re.fullmatch(r"D\d+", day), day
    assert re.fullmatch(r"\$\d+", money), money
    return {"day": int(day[1:]), "dollars": int(money[1:])}


def care_values(image):
    assert text_at(image, 12, 7, 4, 2, "white") == "CARE"
    values = header_values(image)
    for label, x, y in (("hunger", 111, 71), ("mood", 228, 71),
                        ("energy", 111, 84), ("chaos", 228, 84)):
        number = readable(text_at(image, x, y, 3, ink="ink"))
        assert re.fullmatch(r"\d{1,3}", number), (label, number)
        values[label] = int(number)
        assert 0 <= values[label] <= 100, values
    colors = [color_at(image, x, 98) for x in range(62, 237)]
    assert all(color in ("rose", "gray") for color in colors), colors
    filled = colors.count("rose")
    assert colors == ["rose"] * filled + ["gray"] * (175 - filled)
    values["glitch_bar_pixels"] = filled
    values["meter_pixels_sha256"] = hashlib.sha256(
        image.crop((8, 192 + 68, 248, 192 + 104)).tobytes()).hexdigest()
    return values


def care_reply(image):
    return normalized(" ".join(readable(text_at(image, 17, y, 36, ink="ink"))
                                for y in (47, 56)))


def talk_values(image):
    assert text_at(image, 12, 7, 4, 2, "white") == "TALK"
    question = readable(text_at(image, 17, 36, 36))
    assert question.startswith("ASK: "), question
    prompt = question[5:]
    assert prompt in PROMPTS, question
    rows = [readable(text_at(image, 17, y, 36, ink="ink")) for y in (48, 59, 70)]
    assert not rows[2], f"Authored two-line reply overflowed: {rows}"
    for index, label in enumerate(PROMPTS):
        x, y = 18 + (index % 2) * 124, 100 + (index // 2) * 27
        actual = readable(text_at(image, x, y, len(label), ink="ink"))
        assert actual == label, (index, actual, label)
    return {**header_values(image), "selected_prompt": PROMPTS.index(prompt),
            "question": question, "reply_rows": rows,
            "reply": normalized(" ".join(rows))}


class ConversationPlayer(Player):
    def care(self, name, expected=None):
        def matches(image):
            values = care_values(image)
            return expected is None or all(values[key] == value
                                           for key, value in expected.items())

        image = self.wait(name, matches, limit=240)
        return image, care_values(image)

    def talk(self, name, selected=None, replies=None):
        allowed = None if replies is None else {normalized(reply) for reply in replies}
        assert allowed is None or allowed, f"No engine reply contract supplied for {name}"

        def matches(image):
            values = talk_values(image)
            return (bool(values["reply"]) and
                    (selected is None or values["selected_prompt"] == selected) and
                    (allowed is None or values["reply"] in allowed))

        image = self.wait(name, matches, limit=240)
        return image, talk_values(image)

    def ask(self, index, name, replies):
        self.tap(*card_position(index))
        image, values = self.talk(name, selected=index, replies=replies)
        assert values["reply"], name
        self.capture(name, image, **values)
        return values

    def held_ask(self, index, name, replies):
        """One held card keeps one visible answer until the stylus releases."""
        self.emu.input.touch_set_pos(*card_position(index))
        self.step(12)
        image, first = self.talk(name, selected=index, replies=replies)
        self.capture(f"{name}_pressed", image, **first)
        started = self.cycles
        observations = []
        # Uneven intervals avoid repeatedly observing only one phase of a
        # hypothetical repeating reply cycle. No game clock is inspected.
        for elapsed in (5, 7, 11, 13, 17, 19, 23):
            self.step(elapsed)
            image, current = self.talk(f"{name} remains held", selected=index,
                                       replies=replies)
            assert current["reply_rows"] == first["reply_rows"], (first, current)
            observations.append(self.cycles)
        self.capture(f"{name}_still_held", image, **current)
        self.emu.input.touch_release()
        self.step(12)
        image, released = self.talk(f"{name} released", selected=index, replies=replies)
        assert released["reply_rows"] == first["reply_rows"], (first, released)
        self.capture(f"{name}_released", image, **released)
        return {"observations": len(observations),
                "held_emulator_cycles": observations[-1] - started,
                "reply": first["reply"]}

    def exit_and_compare(self, before, name, method):
        if method == "keypad":
            self.key(Keys.KEY_B)
        else:
            self.tap(40, 183)
        image, after = self.care(name)
        assert after == before, {"before": before, "after": after}
        self.capture(name, image, **after)
        return after


def boot(player):
    player.step(120)
    player.capture("title")
    player.tap(128, 115)
    image, state = player.care("initial nursery", {
        "day": 1, "dollars": 8, "hunger": 35, "mood": 75,
        "energy": 75, "chaos": 12,
    })
    player.capture("care_initial", image, **state)
    return state


def require_reply(reply, allowed, context):
    assert reply in {normalized(line) for line in allowed}, (context, reply)


def conversation_scenario(player, contract):
    """Use the engine's agreed authored families, never guessed answers."""
    required = {"initial_how", "empty_memory", "feed_care", "fed_how", "aged_how",
                "feed_remember", "feed_more", "like_ack", "already_liked",
                "new_subject", "healthy_need", "urgent_nap",
                "tired_play_care", "nap_care"}
    assert required <= contract.keys(), f"Missing engine contract: {required - contract.keys()}"
    assert all(contract[key] for key in required)
    initial = boot(player)
    player.key(Keys.KEY_LEFT)
    image, fresh_entry = player.talk("TALK before any care event", 0,
                                     contract["initial_how"])
    player.capture("talk_initial_keypad_entry", image, **fresh_entry)
    empty_memory = player.ask(2, "talk_no_invented_care_memory", contract["empty_memory"])
    player.exit_and_compare(initial, "talk_first_exit_preserves_initial_state", "keypad")

    player.tap(66, 123)
    image, fed = player.care("actual native FEED", {
        "hunger": 5, "mood": 78, "energy": 75, "chaos": 12,
    })
    immediate_feed = care_reply(image)
    require_reply(immediate_feed, contract["feed_care"], "immediate FEED outcome")
    player.capture("care_feed_outcome", image, reply=immediate_feed, **fed)

    player.tap(59, 38)
    image, entry = player.talk("TALK through nursery speech panel", 0,
                               contract["fed_how"])
    assert (entry["day"], entry["dollars"]) == (fed["day"], fed["dollars"])
    player.capture("talk_touch_entry_auto_how", image, **entry)
    remembered = player.ask(2, "talk_remembers_real_feed", contract["feed_remember"])
    followup = player.ask(3, "talk_followup_keeps_feed_topic", contract["feed_more"])
    assert followup["reply"] != remembered["reply"], (remembered, followup)
    liked = player.ask(4, "talk_like_acknowledged", contract["like_ack"])
    duplicate_like = player.ask(4, "talk_like_cannot_rate_same_answer_twice",
                                contract["already_liked"])
    after_like = player.ask(3, "talk_MORE_keeps_topic_after_LIKE", contract["feed_more"])
    changed = player.ask(5, "talk_change_subject_acknowledged", contract["new_subject"])
    assert changed["reply"] != followup["reply"], (followup, changed)
    held = player.held_ask(0, "talk_held_card_single_reply", contract["aged_how"])
    player.exit_and_compare(fed, "talk_B_preserves_visible_care_state", "keypad")

    player.key(Keys.KEY_LEFT)
    image, keypad_entry = player.talk("TALK through nursery LEFT", 0,
                                      contract["aged_how"])
    player.capture("talk_keypad_entry_auto_how", image, **keypad_entry)
    unchanged_reply = keypad_entry["reply"]
    previews = []
    for key, selection in ((Keys.KEY_RIGHT, 1), (Keys.KEY_DOWN, 3),
                            (Keys.KEY_LEFT, 2), (Keys.KEY_UP, 0)):
        player.key(key)
        image, preview = player.talk("selection previews without asking", selection,
                                     {unchanged_reply})
        previews.append(PROMPTS[selection])
    player.capture("talk_dpad_selects_without_asking", image, **preview)
    player.key(Keys.KEY_RIGHT)
    player.talk("NEED preview before A", 1, {unchanged_reply})
    player.key(Keys.KEY_A)
    image, needed = player.talk("A asks selected NEED prompt", 1,
                                contract["healthy_need"])
    assert needed["reply"] != unchanged_reply, (keypad_entry, needed)
    player.capture("talk_A_asks_selected_prompt", image, **needed)
    player.key(Keys.KEY_DOWN)
    player.talk("MORE preview before footer ask", 3, {needed["reply"]})
    player.key(Keys.KEY_DOWN)
    player.talk("NEW SUBJECT preview before footer ask", 5, {needed["reply"]})
    player.tap(214, 183)
    image, footer_asked = player.talk("footer ASK asks selected NEW SUBJECT", 5,
                                     contract["new_subject"])
    assert footer_asked["reply"] != needed["reply"], (needed, footer_asked)
    player.capture("talk_footer_ASK_responds", image, **footer_asked)
    player.exit_and_compare(fed, "talk_footer_BACK_preserves_care_state", "touch")

    # B in CARE must still apply the original CHAT action. TALK's B must
    # only have returned to CARE above, without applying that care action.
    player.key(Keys.KEY_B)
    image, chatted = player.care("original B care CHAT", {
        "hunger": 5, "mood": 85, "energy": 72, "chaos": 12,
    })
    legacy_reply = care_reply(image)
    require_reply(legacy_reply, LEGACY_FEED_MEMORY, "legacy FEED-to-CHAT memory")
    player.capture("care_B_still_CHATS", image, reply=legacy_reply, **chatted)
    player.key(Keys.KEY_A)
    image, _ = player.care("original A FEED", {
        "hunger": 0, "mood": 88, "energy": 72, "chaos": 12,
    })
    require_reply(care_reply(image), contract["feed_care"], "A-button FEED outcome")
    player.capture("care_A_still_FEEDS", image, reply=care_reply(image))

    # Reach a real urgent state using five actual PLAY actions. No save,
    # debug state, emulator memory or hidden RNG observation is involved.
    for played in range(1, 6):
        player.key(Keys.KEY_X)
        image, exhausted = player.care(f"native PLAY {played}", {
            "hunger": played * 12, "mood": min(100, 88 + played * 10),
            "energy": 72 - played * 12, "chaos": 12 + played * 12,
        })
    tired_play_reply = care_reply(image)
    require_reply(tired_play_reply, contract["tired_play_care"], "exhausting PLAY outcome")
    assert exhausted["energy"] == 12
    player.capture("care_real_PLAY_causes_urgent_tiredness", image,
                   reply=tired_play_reply, **exhausted)
    player.key(Keys.KEY_LEFT)
    image, urgent_entry = player.talk("urgent state overrides automatic HOW", 0,
                                      contract["urgent_nap"])
    player.capture("talk_urgent_need_overrides_preferences", image, **urgent_entry)
    urgent_need = player.ask(1, "talk_NEED_reports_real_exhaustion", contract["urgent_nap"])
    urgent_more = player.ask(3, "talk_MORE_respects_urgent_need", contract["urgent_nap"])
    urgent_change = player.ask(5, "talk_NEW_SUBJECT_cannot_override_urgent_need",
                                contract["urgent_nap"])
    player.exit_and_compare(exhausted, "talk_urgent_exit_preserves_meters", "touch")
    player.key(Keys.KEY_Y)
    image, rested = player.care("original Y NAP", {
        "hunger": 68, "mood": 100, "energy": 37, "chaos": 67,
    })
    nap_reply = care_reply(image)
    require_reply(nap_reply, contract["nap_care"], "actual NAP outcome")
    player.capture("care_Y_still_NAPS", image, reply=nap_reply, **rested)

    return {
        "entry_methods": ["native nursery speech-panel touch", "native LEFT keypad"],
        "no_event_memory_reply": empty_memory["reply"],
        "immediate_FEED_reply": immediate_feed,
        "real_feed_recall": remembered["reply"],
        "on_topic_feed_followup": followup["reply"],
        "explicit_like_acknowledgment": liked["reply"],
        "duplicate_like_acknowledgment": duplicate_like["reply"],
        "on_topic_followup_after_like": after_like["reply"],
        "explicit_subject_change": changed["reply"],
        "held_touch": held,
        "dpad_preview_sequence": previews,
        "A_asked_reply": needed["reply"],
        "footer_ASK_reply": footer_asked["reply"],
        "visible_care_state_unchanged_across_all_TALK_exits": True,
        "legacy_care_CHAT_memory_reply": legacy_reply,
        "all_original_care_key_mappings": {"A": "FEED", "B": "CHAT", "X": "PLAY", "Y": "NAP"},
        "urgent_state_from_real_PLAY": exhausted,
        "urgent_reply": urgent_need["reply"],
        "urgent_followup": urgent_more["reply"],
        "urgent_subject_change": urgent_change["reply"],
        "actual_NAP_satisfies_requested_care_reply": nap_reply,
        "final_visible_care_state": rested,
        "scope_note": "Native pixels prove visible meters/day/currency and responses. Full Game/RNG isolation, learning weights and persistence require host tests.",
    }


def run(path, output):
    started = time.perf_counter()
    player = None
    report = {"rom_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
              "emulator": "DeSmuME 0.9.12 / py-desmume 0.0.9",
              "input": "native stylus and keypad; rendered pixels only",
              "result": "FAIL"}
    try:
        contract, metadata = load_reply_contract()
        report["reply_contract"] = metadata
        player = ConversationPlayer(path, output)
        report["conversation"] = conversation_scenario(player, contract)
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
        (output / "conversation-report.json").write_text(
            json.dumps(report, indent=2) + "\n")
    print(json.dumps({key: value for key, value in report.items() if key != "events"},
                     indent=2), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path)
    parser.add_argument("--output", type=Path, default=Path("emulator-conversation-proof"))
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    run(args.rom, args.output)


if __name__ == "__main__":
    main()
