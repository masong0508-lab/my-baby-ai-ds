#ifndef MBAI_SAVE_H
#define MBAI_SAVE_H
#include "game.h"
int save_load(const char *base,Game *out);
int save_write(const char *base,Game *state);
int save_erase(const char *base);
#endif
