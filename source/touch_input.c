#include <string.h>
#include "touch_input.h"
void touch_gate_init(TouchGate *g){memset(g,0,sizeof(*g));}
int touch_gate_step(TouchGate *g,int touching,int x,int y){
 if(!touching){
  g->held=0;g->dispatched=0;g->held_frames=0;
  return 0;
 }
 if(!g->held){g->held=1;g->held_frames=0;g->dispatched=0;}
 if(g->held_frames<65535)g->held_frames++;
 /* On some emulator/ARM7 versions the first pressed frame supplies
  * 0,0 until calibration data arrives. That coordinate becomes valid
  * after three frames so the genuine top-left pixel remains tappable. */
 if(x<0||y<0||x>255||y>191)return 0;
 if(x==0&&y==0&&g->held_frames<3)return 0;
 g->x=(int16_t)x;g->y=(int16_t)y;
 if(g->dispatched)return 0;
 g->dispatched=1;g->total_presses++;
 return 1;
}
