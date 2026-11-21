#ifndef MBAI_ROUGHHOUSE_H
#define MBAI_ROUGHHOUSE_H
#include <stdint.h>
/* Toy-shell slapstick: deterministic 1/256-world-unit fixed-point motion.
 * No heap, physics engine, extra framebuffer, or save-format change. */
#define ROUGHHOUSE_MIN_HITS 6u
#define ROUGHHOUSE_CRACK_CHARGE 15u
#define ROUGHHOUSE_SLAP 1
#define ROUGHHOUSE_KICK 2
/* Each scene tick may take two VBlanks on older DS/DeSmuME, so design
 * anticipation by ACTUAL running-ROM timing, not just 60 Hz arithmetic. */
/* Runtime-paced, non-strobing 4-act scene: fracture, pause, inward push,
 * abrupt single reveal, then collectible memory postcard. */
#define EGG_CRACK_FRAME 72u
#define EGG_PUSH_FRAME 158u
#define EGG_SCARE_FRAME 298u
#define EGG_HOLD_FRAME 322u
#define EGG_TOTAL_FRAMES 375u
typedef struct {
 int32_t x,y,z,vx,vy,vz;
 int16_t roll,pitch,yaw,vr,vp,vyaw;
 uint8_t hits,charge,cooldown,moving;
 uint8_t impact,rebound_count; /* transient rumble / shell rebounds */
} Roughhouse;
void roughhouse_init(Roughhouse *r);
int roughhouse_strike(Roughhouse *r,int kind);
/* Explicit lower-LCD tap point adds directional impulse without changing
 * the on-screen tool-selection hitboxes or the existing button shortcut. */
int roughhouse_strike_at(Roughhouse *r,int kind,int touch_x,int touch_y);
void roughhouse_step(Roughhouse *r);
int roughhouse_cracked(const Roughhouse *r);
/* A CONTINUE advances to the next cinematic act, never past the finale. */
uint32_t roughhouse_egg_next_act(uint32_t frame);
#endif
