#ifndef MBAI_TOUCH_INPUT_H
#define MBAI_TOUCH_INPUT_H
#include <stdint.h>
/* One action per touch gesture, including when the first valid coordinate
 * arrives 1-2 frames after the KEY_TOUCH bit. No heap or backend dependency. */
typedef struct {
 uint8_t held, dispatched;
 uint16_t held_frames;
 int16_t x,y;
 uint32_t total_presses;
} TouchGate;
void touch_gate_init(TouchGate *gate);
int touch_gate_step(TouchGate *gate,int touching,int x,int y);
/* Use for continuous drag/steer actions, not just one-shot taps. */
int touch_gate_coordinates_ready(const TouchGate *gate,int touching,int x,int y);
#endif
