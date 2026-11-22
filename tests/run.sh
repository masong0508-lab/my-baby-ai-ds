#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
# Tests use the host compiler even inside devkitARM containers.
# Additional flags are optional and intentionally split on whitespace to
# permit e.g. MBAI_HOST_FLAGS="-g -fsanitize=address,undefined".
# Keep ONE set of test lists: the sanitizers run this exact script.
# Validate DS ROM metadata without requiring a cross-compiler or emulator.
python3 tests/test_rom_identity.py
"${MBAI_HOST_CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -O2 ${MBAI_HOST_FLAGS:-} -Iinclude source/game.c source/dialogue.c source/save.c tests/test_core.c -o /tmp/my_baby_ai_core_test
"${MBAI_HOST_CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -O2 ${MBAI_HOST_FLAGS:-} -Iinclude source/roughhouse.c source/game.c source/dialogue.c source/save.c tests/test_roughhouse.c -o /tmp/my_baby_ai_secret_egg_test
/tmp/my_baby_ai_secret_egg_test
"${MBAI_HOST_CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -O2 ${MBAI_HOST_FLAGS:-} -Iinclude source/sfx.c tests/test_sfx.c -o /tmp/my_baby_ai_audio_test
/tmp/my_baby_ai_audio_test
/tmp/my_baby_ai_core_test
"${MBAI_HOST_CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -O2 ${MBAI_HOST_FLAGS:-} -Iinclude source/game.c source/dialogue.c source/ui.c tests/test_ui.c -o /tmp/my_baby_ai_ui_test
/tmp/my_baby_ai_ui_test
"${MBAI_HOST_CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -O2 ${MBAI_HOST_FLAGS:-} -Iinclude source/game.c source/dialogue.c tests/test_dialogue.c -o /tmp/my_baby_ai_dialogue_test
/tmp/my_baby_ai_dialogue_test
"${MBAI_HOST_CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -O2 ${MBAI_HOST_FLAGS:-} -Iinclude source/game.c source/dialogue.c source/conversation.c tests/test_conversation.c -o /tmp/my_baby_ai_conversation_test
/tmp/my_baby_ai_conversation_test
"${MBAI_HOST_CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -O2 ${MBAI_HOST_FLAGS:-} -Iinclude source/game.c source/dialogue.c source/conversation.c source/save.c source/conversation_save.c tests/test_conversation_save.c -o /tmp/my_baby_ai_conversation_save_test
/tmp/my_baby_ai_conversation_save_test
"${MBAI_HOST_CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -O2 ${MBAI_HOST_FLAGS:-} -Itests/stubs -Iinclude \
 source/world.c source/roughhouse.c source/game.c source/dialogue.c tests/test_world.c -o /tmp/my_baby_ai_world_budget
/tmp/my_baby_ai_world_budget
"${MBAI_HOST_CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -O2 ${MBAI_HOST_FLAGS:-} -Iinclude source/game.c source/dialogue.c tests/test_minigames.c -o /tmp/my_baby_ai_minigames_test
/tmp/my_baby_ai_minigames_test
"${MBAI_HOST_CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -O2 ${MBAI_HOST_FLAGS:-} -Iinclude source/game.c source/dialogue.c source/arcade_feedback.c tests/test_arcade_feedback.c -o /tmp/my_baby_ai_arcade_feedback_test
/tmp/my_baby_ai_arcade_feedback_test
"${MBAI_HOST_CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -O2 ${MBAI_HOST_FLAGS:-} -Iinclude source/touch_input.c tests/test_touch_input.c -o /tmp/my_baby_ai_touch_test
/tmp/my_baby_ai_touch_test
"${MBAI_HOST_CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -O2 ${MBAI_HOST_FLAGS:-} -Iinclude source/virtual_stylus.c tests/test_virtual_stylus.c -o /tmp/my_baby_ai_virtual_stylus_test
/tmp/my_baby_ai_virtual_stylus_test
"${MBAI_HOST_CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -O2 ${MBAI_HOST_FLAGS:-} -Iinclude source/game.c source/dialogue.c source/save.c tests/test_stress.c -o /tmp/my_baby_ai_stress_test
/tmp/my_baby_ai_stress_test
