#include "virtual_stylus.h"
void virtual_stylus_init(VirtualStylus *s){
 s->enabled=0;s->x=64;s->y=131;
 s->held_frames=0;s->last_direction=0;
}
void virtual_stylus_toggle(VirtualStylus *s){
 s->enabled=(uint8_t)!s->enabled;
 s->held_frames=0;s->last_direction=0;
}
int virtual_stylus_move(VirtualStylus *s,unsigned h){
 int x=s->x,y=s->y,ox=x,oy=y,step;
 unsigned direction=h&(VS_LEFT|VS_RIGHT|VS_UP|VS_DOWN);
 if(!s->enabled)return 0;
 /* Opposing arrows cancel. A changed direction or released D-pad
  * resets speed so small precise movements remain possible. */
 if((direction&(VS_LEFT|VS_RIGHT))==(VS_LEFT|VS_RIGHT))
  direction&=~(VS_LEFT|VS_RIGHT);
 if((direction&(VS_UP|VS_DOWN))==(VS_UP|VS_DOWN))
  direction&=~(VS_UP|VS_DOWN);
 if(!direction||direction!=s->last_direction){
  s->held_frames=0;
  s->last_direction=(uint8_t)direction;
 }else if(s->held_frames<60u)s->held_frames++;
 step=s->held_frames>=28u?8:s->held_frames>=12u?5:3;
 if(direction&VS_LEFT)x-=step;
 if(direction&VS_RIGHT)x+=step;
 if(direction&VS_UP)y-=step;
 if(direction&VS_DOWN)y+=step;
 if(x<0)x=0;
 if(x>255)x=255;
 if(y<0)y=0;
 if(y>191)y=191;
 s->x=(uint8_t)x;s->y=(uint8_t)y;
 return x!=ox||y!=oy;
}
