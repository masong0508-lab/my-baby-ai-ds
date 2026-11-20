#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Iinclude source/game.c source/save.c tests/test_core.c -o /tmp/my_baby_ai_core_test
/tmp/my_baby_ai_core_test
