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
 /* Golden endings still have exactly the old meaning. */
 game_init(&state);
 state.ending_mask=(uint16_t)(0x01e0u);
 assert(game_good_ending_count(&state)==4);
 assert(!game_reset_unlocked(&state));
 puts("PASS: deterministic bounded synthetic ragdoll, six-hit secret, one-shot persistent alien egg");
 return 0;
}
