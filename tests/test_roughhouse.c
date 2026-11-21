#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "roughhouse.h"
#include "game.h"
#include "save.h"

static void in_bounds(const Roughhouse *r){
 assert(r->x>=-390&&r->x<=390);
 assert(r->z>=-192&&r->z<=192);
 assert(r->y>=0&&r->y<=345);
 assert(r->roll>=-640&&r->roll<=640);
 assert(r->pitch>=-400&&r->pitch<=400);
}
int main(void){
 Roughhouse a,b;
 Game state,loaded;
 unsigned i,j;
 const char *base="/tmp/my_baby_ai_secret_egg";
 roughhouse_init(&a);
 assert(!roughhouse_cracked(&a));
 assert(!roughhouse_strike(&a,0));
 assert(!roughhouse_strike(&a,9));
 assert(roughhouse_strike(&a,ROUGHHOUSE_KICK));
 assert(a.hits==1&&a.charge==3&&a.moving);
 assert(!roughhouse_strike(&a,ROUGHHOUSE_SLAP));
 /* Two different touch points create distinct, bounded takeoff impulses,
  * without affecting tool selection or the secret ending threshold. */
 roughhouse_init(&a);roughhouse_init(&b);
 assert(roughhouse_strike_at(&a,ROUGHHOUSE_KICK,101,96));
 assert(roughhouse_strike_at(&b,ROUGHHOUSE_KICK,156,128));
 assert(a.vx>b.vx);
 assert(a.vy>b.vy);
 assert(a.impact==6&&b.impact==6);
 /* Extreme touch coordinates must clamp instead of overdriving physics. */
 roughhouse_init(&a);
 assert(roughhouse_strike_at(&a,ROUGHHOUSE_KICK,-999,-999));
 assert(a.vx<=112&&a.vy<=135);
 roughhouse_init(&b);
 assert(roughhouse_strike_at(&b,ROUGHHOUSE_KICK,10,79));
 assert(memcmp(&a,&b,sizeof(a))==0);
 /* Walls actually reflect, and collision feedback is deterministic. */
 roughhouse_init(&a);
 a.x=390;a.vx=60;a.y=30;a.vy=0;a.moving=1;
 roughhouse_step(&a);
 assert(a.x==390&&a.vx<0&&a.rebound_count==1&&a.impact==9);
 roughhouse_init(&a);roughhouse_init(&b);
 for(i=0;i<6;i++){
  assert(roughhouse_strike(&a,ROUGHHOUSE_KICK));
  assert(roughhouse_strike(&b,ROUGHHOUSE_KICK));
  for(j=0;j<12;j++){
   roughhouse_step(&a);roughhouse_step(&b);
   in_bounds(&a);in_bounds(&b);
   assert(memcmp(&a,&b,sizeof(a))==0);
  }
 }
 assert(a.hits==6&&a.charge==18&&roughhouse_cracked(&a));
 assert(!roughhouse_strike(&a,ROUGHHOUSE_KICK));
 roughhouse_init(&a);
 assert(!roughhouse_cracked(&a));
 for(i=0;i<8;i++){
  assert(roughhouse_strike(&a,ROUGHHOUSE_SLAP));
  for(j=0;j<12;j++)roughhouse_step(&a);
 }
 assert(roughhouse_cracked(&a));
 assert(!roughhouse_strike(&a,ROUGHHOUSE_SLAP));
 save_erase(base);
 game_init(&state);
 assert(game_secret_egg(&state));
 assert((state.ending_mask&(1u<<END_EGG))!=0);
 assert(!game_is_good_ending(END_EGG));
 assert(state.bad_endings==1);
 assert(!game_secret_egg(&state));
 assert(state.bad_endings==1); /* repeat discovery never farms an ending */
 assert(save_write(base,&state));
 assert(save_load(base,&loaded));
 assert(loaded.ending_mask==state.ending_mask);
 assert(loaded.bad_endings==1);
 assert(!game_secret_egg(&loaded));
 assert(save_erase(base));
 /* Landing never instantly pops a twisted doll to neutral orientation. */
 roughhouse_init(&a);
 a.y=0;a.vy=0;a.roll=400;a.pitch=-240;a.yaw=180;
 a.moving=1;
 roughhouse_step(&a);
 assert(a.moving && a.roll>0 && a.roll<400);
 assert(a.pitch<0 && a.pitch>-240 && a.yaw>0 && a.yaw<180);
 for(i=0;i<100&&a.moving;i++)roughhouse_step(&a);
 assert(!a.moving && a.roll==0 && a.pitch==0 && a.yaw==0);
 /* The same landing sequence is bit-for-bit deterministic. */
 roughhouse_init(&a);roughhouse_init(&b);
 a.roll=b.roll=640;a.pitch=b.pitch=-400;a.yaw=b.yaw=300;
 a.moving=b.moving=1;
 for(i=0;i<100;i++){
  roughhouse_step(&a);roughhouse_step(&b);
  assert(memcmp(&a,&b,sizeof(a))==0);
 }
 assert(!a.moving);
 /* A advances one film act; B/START are the only immediate full skip.
  * Strictly monotonic checkpoints avoid stuck/cycled cinematics. */
 assert(roughhouse_egg_next_act(0u)==EGG_CRACK_FRAME);
 assert(roughhouse_egg_next_act(EGG_CRACK_FRAME-1u)==EGG_CRACK_FRAME);
 assert(roughhouse_egg_next_act(EGG_CRACK_FRAME)==EGG_PUSH_FRAME);
 assert(roughhouse_egg_next_act(EGG_PUSH_FRAME-1u)==EGG_PUSH_FRAME);
 assert(roughhouse_egg_next_act(EGG_PUSH_FRAME)==EGG_SCARE_FRAME);
 assert(roughhouse_egg_next_act(EGG_SCARE_FRAME-1u)==EGG_SCARE_FRAME);
 assert(roughhouse_egg_next_act(EGG_SCARE_FRAME)==EGG_TOTAL_FRAMES);
 assert(roughhouse_egg_next_act(EGG_TOTAL_FRAMES)==EGG_TOTAL_FRAMES);
 /* Golden endings still have exactly the old meaning. */
 game_init(&state);
 state.ending_mask=(uint16_t)(0x01e0u);
 assert(game_good_ending_count(&state)==4);
 assert(!game_reset_unlocked(&state));
 puts("PASS: tap-aimed synthetic ragdoll, stable inertia, wall bounces, six-hit secret and once-only egg save");
 return 0;
}
