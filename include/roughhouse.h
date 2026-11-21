#ifndef MBAI_ROUGHHOUSE_H
#define MBAI_ROUGHHOUSE_H
#include <stdint.h>
/* Toy-shell slapstick: deterministic 1/256-world-unit fixed-point motion.
 * No heap, physics engine, extra framebuffer, or save-format change. */
#define ROUGHHOUSE_MIN_HITS 6u
#define ROUGHHOUSE_CRACK_CHARGE 15u
#define ROUGHHOUSE_SLAP 1
#define ROUGHHOUSE_KICK 2
typedef struct {
 int32_t x,y,z,vx,vy,vz;
 int16_t roll,pitch,yaw,vr,vp,vyaw;
 uint8_t hits,charge,cooldown,moving;
} Roughhouse;
void roughhouse_init(Roughhouse *r);
int roughhouse_strike(Roughhouse *r,int kind);
void roughhouse_step(Roughhouse *r);
int roughhouse_cracked(const Roughhouse *r);
#endif
