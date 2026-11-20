#ifndef MBAI_WORLD_H
#define MBAI_WORLD_H
#include <stdint.h>
#include "game.h"
void world_init(void);
void world_draw(const Game *state,uint32_t frame,int pose);
/* Submitted frame-budget counters (geometry, not measured FPS). */
unsigned world_vertex_count(void);
unsigned world_polygon_count(void);
#endif
