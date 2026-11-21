#include <string.h>
#include "roughhouse.h"
#define BOUND_X 390
#define BOUND_Z 192
#define BOUND_Y 345
static int32_t abs32(int32_t x){return x<0?-x:x;}
static int32_t decay(int32_t v,int factor){return v*factor/256;}
void roughhouse_init(Roughhouse *r){memset(r,0,sizeof(*r));}
int roughhouse_cracked(const Roughhouse *r){
 return r&&r->hits>=ROUGHHOUSE_MIN_HITS&&r->charge>=ROUGHHOUSE_CRACK_CHARGE;
}
int roughhouse_strike(Roughhouse *r,int kind){
 int side;
 if(!r||(kind!=ROUGHHOUSE_SLAP&&kind!=ROUGHHOUSE_KICK)||
    r->cooldown||roughhouse_cracked(r))return 0;
 side=(r->hits&1u)?-1:1;
 if(r->hits<255u)r->hits++;
 r->charge=(uint8_t)(r->charge+(kind==ROUGHHOUSE_KICK?3u:2u));
 r->vx=side*(kind==ROUGHHOUSE_KICK?52:36);
 r->vy=kind==ROUGHHOUSE_KICK?96:75;
 r->vz=-side*(kind==ROUGHHOUSE_KICK?35:22);
 r->vr=(int16_t)(side*(kind==ROUGHHOUSE_KICK?27:21));
 r->vp=(int16_t)(kind==ROUGHHOUSE_KICK?22:-16);
 r->vyaw=(int16_t)(side*16);
 /* Stylus updates at roughly 20-30 logic FPS on some real/emulated DS
  * builds. Four ticks debounces a press without silently dropping every
  * other deliberate tap on a slower device. */
 r->cooldown=4;
 r->moving=1;
 return 1;
}
void roughhouse_step(Roughhouse *r){
 if(!r)return;
 if(r->cooldown)r->cooldown--;
 if(!r->moving)return;
 r->x+=r->vx;r->y+=r->vy;r->z+=r->vz;
 r->vy-=5;
 if(r->x>BOUND_X){r->x=BOUND_X;r->vx=-r->vx*3/4;}
 if(r->x< -BOUND_X){r->x=-BOUND_X;r->vx=-r->vx*3/4;}
 if(r->z>BOUND_Z){r->z=BOUND_Z;r->vz=-r->vz*3/4;}
 if(r->z< -BOUND_Z){r->z=-BOUND_Z;r->vz=-r->vz*3/4;}
 if(r->y>BOUND_Y){r->y=BOUND_Y;if(r->vy>0)r->vy=0;}
 if(r->y<0){r->y=0;if(r->vy<0)r->vy=-r->vy*3/5;}
 if(r->y==0&&r->vy<6)r->vy=0;
 r->vx=decay(r->vx,242);r->vz=decay(r->vz,242);
 r->roll=(int16_t)(r->roll+r->vr);
 r->pitch=(int16_t)(r->pitch+r->vp);
 r->yaw=(int16_t)(r->yaw+r->vyaw);
 if(r->roll>640)r->roll=640;
 if(r->roll< -640)r->roll=-640;
 if(r->pitch>400)r->pitch=400;
 if(r->pitch< -400)r->pitch=-400;
 r->vr=(int16_t)decay(r->vr,233);
 r->vp=(int16_t)decay(r->vp,233);
 r->vyaw=(int16_t)decay(r->vyaw,233);
 if(abs32(r->vx)<2)r->vx=0;
 if(abs32(r->vz)<2)r->vz=0;
 if(abs32(r->vr)<2)r->vr=0;
 if(abs32(r->vp)<2)r->vp=0;
 if(abs32(r->vyaw)<2)r->vyaw=0;
 if(r->y==0&&r->vy==0&&!r->vx&&!r->vz&&!r->vr&&!r->vp&&!r->vyaw){
  r->moving=0;r->roll=0;r->pitch=0;r->yaw=0;
 }
}
