#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Iinclude source/game.c source/dialogue.c source/save.c tests/test_core.c -o /tmp/my_baby_ai_core_test
/tmp/my_baby_ai_core_test
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Iinclude source/game.c source/dialogue.c source/ui.c tests/test_ui.c -o /tmp/my_baby_ai_ui_test
/tmp/my_baby_ai_ui_test
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Iinclude source/game.c source/dialogue.c tests/test_dialogue.c -o /tmp/my_baby_ai_dialogue_test
/tmp/my_baby_ai_dialogue_test
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Itests/stubs -Iinclude \
 source/world.c source/game.c source/dialogue.c tests/test_world.c -o /tmp/my_baby_ai_world_budget
/tmp/my_baby_ai_world_budget
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Iinclude source/game.c source/dialogue.c tests/test_minigames.c -o /tmp/my_baby_ai_minigames_test
/tmp/my_baby_ai_minigames_test
