#ifndef MBAI_VIRTUAL_STYLUS_H
#define MBAI_VIRTUAL_STYLUS_H
#include <stdint.h>
enum { VS_LEFT=1, VS_RIGHT=2, VS_UP=4, VS_DOWN=8 };
typedef struct {
 uint8_t enabled;
 uint8_t x,y;
} VirtualStylus;
void virtual_stylus_init(VirtualStylus *s);
void virtual_stylus_toggle(VirtualStylus *s);
/* Returns 1 iff position changed. Bound to full DS 256x192 space. */
int virtual_stylus_move(VirtualStylus *s,unsigned held_mask);
#endif
