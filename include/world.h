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
/* View-only damage portrait, normalized independently of Toybox inertia. */
Roughhouse world_egg_staged_doll(const Roughhouse *source,uint32_t frame);
/* Camera sampled in one place to keep staged film and native tests aligned. */
typedef struct {
 float x,y,z,look_x,look_y,look_z;
} EggCamera;
EggCamera world_secret_camera(uint32_t frame);
/* 0 normal light, -5 darkest; computed without modifying the nursery mesh. */
int world_egg_nursery_ambient(uint32_t frame);
/* Coherent, bounded 3D animation; pure frame samplers for film replay. */
float world_egg_creature_depth(uint32_t frame);
float world_egg_embryo_breath(uint32_t frame);
/* Deterministic, shared shoulder-joint reach for each care action. */
int world_baby_arm_reach(int action);
int world_baby_neck_tilt(int action,uint32_t frame);
/* Submitted frame-budget counters (geometry, not measured FPS). */
unsigned world_vertex_count(void);
unsigned world_polygon_count(void);
#endif
