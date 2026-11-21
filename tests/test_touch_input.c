#include <assert.h>
#include <stdio.h>
#include "touch_input.h"
int main(void){
 TouchGate t;touch_gate_init(&t);
 assert(!touch_gate_step(&t,0,0,0));
 assert(!touch_gate_coordinates_ready(&t,0,80,142));
 assert(!touch_gate_coordinates_ready(&t,1,0,0));
 assert(!touch_gate_coordinates_ready(&t,1,-1,50));
 assert(!touch_gate_coordinates_ready(&t,1,256,50));
 assert(!touch_gate_coordinates_ready(&t,1,50,192));
 assert(!touch_gate_step(&t,1,0,0)); /* late calibration */
 assert(!touch_gate_step(&t,1,0,0));
 assert(touch_gate_coordinates_ready(&t,1,80,142));
 assert(touch_gate_step(&t,1,80,142));
 assert(t.x==80&&t.y==142&&t.total_presses==1);
 assert(!touch_gate_step(&t,1,90,142)); /* held should not double-tap */
 assert(!touch_gate_step(&t,0,0,0));
 assert(touch_gate_step(&t,1,234,15));
 assert(t.total_presses==2);
 assert(!touch_gate_step(&t,0,0,0));
 assert(!touch_gate_step(&t,1,0,0));
 assert(!touch_gate_step(&t,1,0,0));
 assert(touch_gate_coordinates_ready(&t,1,0,0));
 assert(touch_gate_step(&t,1,0,0)); /* legitimate upper-left point */
 assert(!touch_gate_step(&t,1,-1,25));
 puts("PASS: delayed stylus coordinates, one action per touch, release re-arm");
 return 0;
}
