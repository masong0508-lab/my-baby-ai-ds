#include <string.h>
#include "roughhouse.h"
#define BOUND_X 390
#define BOUND_Z 192
#define BOUND_Y 345
static int32_t abs32(int32_t x){return x<0?-x:x;}
static int32_t decay(int32_t v,int factor){return v*factor/256;}
/* Deterministic fixed-point springless posture settle; fast at large
 * angles and fine at small ones, without floats or an extra animation. */
static int16_t settle_angle(int16_t angle){
 int32_t n=angle,step;
 if(n>=-2&&n<=2)return 0;
 step=abs32(n)/4;
 if(step<2)step=2;
 return (int16_t)(n>0?n-step:n+step);
}
static int32_t clamp32(int32_t x,int32_t lo,int32_t hi){
 return x<lo?lo:x>hi?hi:x;
}
void roughhouse_init(Roughhouse *r){if(r)memset(r,0,sizeof(*r));}
uint32_t roughhouse_egg_next_act(uint32_t frame){
 if(frame<EGG_CRACK_FRAME)return EGG_CRACK_FRAME;
 if(frame<EGG_PUSH_FRAME)return EGG_PUSH_FRAME;
 if(frame<EGG_SCARE_FRAME)return EGG_SCARE_FRAME;
 return EGG_TOTAL_FRAMES;
}
int roughhouse_cracked(const Roughhouse *r){
 return r&&r->hits>=ROUGHHOUSE_MIN_HITS&&r->charge>=ROUGHHOUSE_CRACK_CHARGE;
}
int roughhouse_strike_at(Roughhouse *r,int kind,int touch_x,int touch_y){
 int side,aim_x,aim_y,energy;
 if(!r||(kind!=ROUGHHOUSE_SLAP&&kind!=ROUGHHOUSE_KICK)||
    r->cooldown||roughhouse_cracked(r))return 0;
 /* Touch positions are CLAMPED to the lower LCD. The fixed-point impulse
  * varies smoothly from one edge of the toy portrait to the other, rather
  * than toggling a canned left/right animation. */
 if(touch_x<10)touch_x=10;
 if(touch_x>246)touch_x=246;
 if(touch_y<79)touch_y=79;
 if(touch_y>141)touch_y=141;
 aim_x=(128-touch_x)/5; /* +/-23, toward the contact point's opposite side */
 aim_y=(110-touch_y)/4; /* gentle varying loft from lower or upper taps */
 side=(r->hits&1u)?-1:1;
 energy=kind==ROUGHHOUSE_KICK?52:36;
 if(r->hits<255u)r->hits++;
 r->charge=(uint8_t)(r->charge+(kind==ROUGHHOUSE_KICK?3u:2u));
 /* Carry some previous inertia. Three-axis spin is still clamped and
  * damping remains deterministic on ARM9 or host integer arithmetic. */
 r->vx=clamp32(r->vx/3+side*energy+aim_x,-112,112);
 r->vy=clamp32(r->vy/4+(kind==ROUGHHOUSE_KICK?96:75)+aim_y,32,135);
 r->vz=clamp32(r->vz/4-side*(kind==ROUGHHOUSE_KICK?35:22)+aim_x/2,-87,87);
 r->vr=(int16_t)clamp32(r->vr/3+side*(kind==ROUGHHOUSE_KICK?27:21)+aim_x/3,-48,48);
 r->vp=(int16_t)clamp32(r->vp/3+(kind==ROUGHHOUSE_KICK?22:-16)+aim_y/2,-41,41);
 r->vyaw=(int16_t)clamp32(r->vyaw/3+side*16+aim_x/4,-33,33);
 r->cooldown=4; /* still debounces physical/stylus buttons at slow FPS */
 r->impact=6;   /* transient impact flag; no extra GPU particles */
 r->moving=1;
 return 1;
}
int roughhouse_strike(Roughhouse *r,int kind){
 return roughhouse_strike_at(r,kind,128,110);
}
void roughhouse_step(Roughhouse *r){
 int collision=0;
 if(!r)return;
 if(r->cooldown)r->cooldown--;
 if(r->impact)r->impact--;
 if(!r->moving)return;
 r->x+=r->vx;r->y+=r->vy;r->z+=r->vz;
 r->vy-=5;
 if(r->x>BOUND_X){
  r->x=BOUND_X;
  r->vx=-r->vx*3/4;
  r->vr=(int16_t)-r->vr;
  collision=1;
 }
 if(r->x< -BOUND_X){
  r->x=-BOUND_X;
  r->vx=-r->vx*3/4;
  r->vr=(int16_t)-r->vr;
  collision=1;
 }
 if(r->z>BOUND_Z){
  r->z=BOUND_Z;
  r->vz=-r->vz*3/4;
  r->vyaw=(int16_t)-r->vyaw;
  collision=1;
 }
 if(r->z< -BOUND_Z){
  r->z=-BOUND_Z;
  r->vz=-r->vz*3/4;
  r->vyaw=(int16_t)-r->vyaw;
  collision=1;
 }
 if(r->y>BOUND_Y){
  r->y=BOUND_Y;
  if(r->vy>0)r->vy=0;
  collision=1;
 }
 if(r->y<0){
  r->y=0;
  if(r->vy< -8)collision=1;
  if(r->vy<0)r->vy=-r->vy*3/5;
 }
 if(r->y==0&&r->vy<6)r->vy=0;
 if(collision){
  if(r->rebound_count<255u)r->rebound_count++;
  r->impact=9;
 }
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
  /* Don't teleport the pose upright on the first grounded idle frame.
   * Continue lightweight posture recovery until all rotations settle. */
  r->roll=settle_angle(r->roll);
  r->pitch=settle_angle(r->pitch);
  r->yaw=settle_angle(r->yaw);
  if(!r->roll&&!r->pitch&&!r->yaw)r->moving=0;
 }
}
