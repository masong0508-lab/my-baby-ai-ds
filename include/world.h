#ifndef MBAI_WORLD_H
#define MBAI_WORLD_H
#include <stdint.h>
#include "game.h"
/* Hardware-accelerated 3D nursery, without bitmaps or heap allocation. */
void world_init(void);
void world_draw(const Game *state,uint32_t frame,int pose);
#endif
