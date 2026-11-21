#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "game.h"
#include "save.h"

/* Deterministic long-run simulation: exercises all care actions, endings,
 * corruption tiers, all eight distinct initial cabinets, legacy challenges and saves.
 * The ROM has no heap allocations in these state transitions. */
int main(void){
 const char *base="/tmp/my_baby_ai_stress";
 Game g,loaded;
 Arcade a;
 uint32_t seed=0x734c93adu;
 int i,arcades=0,saves=0;
 assert(save_erase(base));
 game_init(&g);
 for(i=0;i<25000;i++){
  seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
  if(i%23==0){
   unsigned prev_day=g.days;
   int ending=game_end_day(&g);
   assert(ending>=END_OK&&ending<=END_SECOND_CHANCE);
   assert(g.days==prev_day+1u);
  }else if(i%17==0){
   int n=0,id=(int)((seed>>12)&15u);
    g.unlocked|=(uint16_t)(1u<<id);
   game_arcade_start(&g,&a,id);
   assert(a.active);
   while(a.active&&n<720){
    if(id==0||id==9)game_arcade_steer(&a,(n/9)&1?1:-1);
    else if(n%8==0)game_arcade_input(&g,&a,game_arcade_expected(&a));
    game_arcade_tick(&g,&a);
    n++;
   }
   assert(!a.active&&a.finished>=1&&a.finished<=2);
   arcades++;
  }else{
   (void)game_care(&g,(Care)(seed&3u));
  }
  if(i%97==0)(void)game_unlock(&g,8+(i/97)%8);
  assert(g.days>=1);
  assert(g.hunger<=100&&g.mood<=100&&g.energy<=100);
  assert(g.chaos<=100&&g.corruption<=100);
  assert((g.unlocked&255u)==255u);
  assert(g.safe_visuals<=1);
  if(i%127==0){
   assert(save_write(base,&g));
   assert(save_load(base,&loaded));
   assert(loaded.sequence==g.sequence);
   assert(loaded.days==g.days&&loaded.dollars==g.dollars);
   assert(loaded.wins==g.wins&&loaded.losses==g.losses);
   assert(loaded.corruption==g.corruption);
   saves++;
  }
 }
 assert(arcades>1000&&saves>150);
 assert(save_erase(base));
 printf("PASS: 25000 seeded gameplay transitions, %d arcades, %d save round-trips\\n",arcades,saves);
 return 0;
}
