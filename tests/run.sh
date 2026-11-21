#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Iinclude source/game.c source/dialogue.c source/save.c tests/test_core.c -o /tmp/my_baby_ai_core_test
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Iinclude source/roughhouse.c source/game.c source/dialogue.c source/save.c tests/test_roughhouse.c -o /tmp/my_baby_ai_secret_egg_test
/tmp/my_baby_ai_secret_egg_test
/tmp/my_baby_ai_core_test
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Iinclude source/game.c source/dialogue.c source/ui.c tests/test_ui.c -o /tmp/my_baby_ai_ui_test
/tmp/my_baby_ai_ui_test
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Iinclude source/game.c source/dialogue.c tests/test_dialogue.c -o /tmp/my_baby_ai_dialogue_test
/tmp/my_baby_ai_dialogue_test
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Itests/stubs -Iinclude \
 source/world.c source/roughhouse.c source/game.c source/dialogue.c tests/test_world.c -o /tmp/my_baby_ai_world_budget
/tmp/my_baby_ai_world_budget
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Iinclude source/game.c source/dialogue.c tests/test_minigames.c -o /tmp/my_baby_ai_minigames_test
/tmp/my_baby_ai_minigames_test
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Iinclude source/touch_input.c tests/test_touch_input.c -o /tmp/my_baby_ai_touch_test
/tmp/my_baby_ai_touch_test
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Iinclude source/virtual_stylus.c tests/test_virtual_stylus.c -o /tmp/my_baby_ai_virtual_stylus_test
/tmp/my_baby_ai_virtual_stylus_test
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Iinclude source/game.c source/dialogue.c source/save.c tests/test_stress.c -o /tmp/my_baby_ai_stress_test
/tmp/my_baby_ai_stress_test
