#include "virtual_stylus.h"
void virtual_stylus_init(VirtualStylus *s){
 s->enabled=0;s->x=64;s->y=131;
}
void virtual_stylus_toggle(VirtualStylus *s){
 s->enabled=(uint8_t)!s->enabled;
}
int virtual_stylus_move(VirtualStylus *s,unsigned h){
 int x=s->x,y=s->y,ox=x,oy=y;
 if(!s->enabled)return 0;
 if((h&VS_LEFT)&&!(h&VS_RIGHT))x-=3;
 if((h&VS_RIGHT)&&!(h&VS_LEFT))x+=3;
 if((h&VS_UP)&&!(h&VS_DOWN))y-=3;
 if((h&VS_DOWN)&&!(h&VS_UP))y+=3;
 if(x<0)x=0;
 if(x>255)x=255;
 if(y<0)y=0;
 if(y>191)y=191;
 s->x=(uint8_t)x;s->y=(uint8_t)y;
 return x!=ox||y!=oy;
}
