#ifndef MBAI_WORLD_H
#define MBAI_WORLD_H
#include <stdint.h>
#include "game.h"
#include "roughhouse.h"
void world_init(void);
void world_draw(const Game *state,uint32_t frame,int pose);
void world_draw_title(uint32_t frame);
void world_draw_roughhouse(const Game *s,uint32_t frame,const Roughhouse *r);
void world_draw_secret(const Game *s,uint32_t frame,const Roughhouse *r);
/* Submitted frame-budget counters (geometry, not measured FPS). */
unsigned world_vertex_count(void);
unsigned world_polygon_count(void);
#endif
