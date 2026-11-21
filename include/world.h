#ifndef MBAI_WORLD_H
#define MBAI_WORLD_H
#include <stdint.h>
#include "game.h"
#include "roughhouse.h"
void world_init(void);
void world_draw(const Game *state,uint32_t frame,int pose);
/* Live gameplay: age since last care tap, for finite easing and recovery. */
void world_draw_care(const Game *state,uint32_t frame,int pose,uint32_t age);
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
/* Reuse a single room model with five art-directed camera targets.
 * Smooth camera transition, daytime and lamp are temporary scene state. */
EggCamera world_nursery_camera(int focus,int previous,uint32_t age);
/* Interpolate from a captured in-flight view to prevent rapid-switch jumps. */
EggCamera world_nursery_camera_from(EggCamera from,int focus,uint32_t age);
void world_draw_nursery(const Game *state,uint32_t frame,int pose,
                        uint32_t action_age,int focus,int previous,
                        uint32_t camera_age,int daylight,int lamp,
                        uint32_t prop_age,const EggCamera *camera_start);
/* 0 normal light, -5 darkest; computed without modifying the nursery mesh. */
int world_egg_nursery_ambient(uint32_t frame);
/* Coherent, bounded 3D animation; pure frame samplers for film replay. */
float world_egg_creature_depth(uint32_t frame);
float world_egg_embryo_breath(uint32_t frame);
/* Deterministic anticipation: dim-to-bright eyes and shallow forward lean. */
int world_egg_eye_glow(uint32_t frame);
float world_egg_embryo_approach(uint32_t frame);
float world_egg_shell_open(uint32_t frame);
int world_egg_head_tilt(uint32_t frame);
/* Lower-jaw hinge angle, degree units, bounded to 0..18. */
int world_egg_jaw_angle(uint32_t frame);
/* Deterministic, shared shoulder-joint reach for each care action. */
int world_baby_arm_reach(int action);
int world_baby_neck_tilt(int action,uint32_t frame);
/* Q8 gesture and eye-aperture channels, cheap deterministic samples. */
int world_baby_care_weight(uint32_t age);
int world_baby_lid_open(uint32_t frame,int action,uint32_t age);
int world_baby_speech_open(uint32_t age);
/* Finite plush reaction and bounded whole-head furniture attention. */
int world_teddy_bow_angle(uint32_t age);
/* One-shot, anchored nursery mobile swing during a CRIB interaction. */
int world_crib_mobile_swing(int focus,uint32_t age);
int world_baby_attention_angle(int focus,uint32_t age);
/* Submitted frame-budget counters (geometry, not measured FPS). */
unsigned world_vertex_count(void);
unsigned world_polygon_count(void);
#endif
