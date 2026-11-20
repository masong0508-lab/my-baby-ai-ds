#include <assert.h>
#include <stdio.h>
#include "game.h"
static void bottle(void){
 Game g;Arcade a;int i;
 game_init(&g);game_arcade_start(&g,&a,0);
 assert(a.active&&a.id==0&&a.timer==600);
 for(i=0;i<600&&a.active;i++){
  int drift=(((a.elapsed+1u)/34u)&1u)?-2:2;
  int steer=a.balance>1?-1:a.balance< -1?1:(drift>0?-1:1);
  if(a.reverse)steer=-steer;
  game_arcade_steer(&a,steer);
  game_arcade_tick(&g,&a);
 }
 assert(a.finished==1&&g.wins==1&&a.hold_frames==150);
 game_arcade_start(&g,&a,0);
 for(i=0;i<600&&a.active;i++)game_arcade_tick(&g,&a);
 assert(a.finished==2&&g.losses==1);
}
static void diaper(void){
 Game g;Arcade a;int i;
 const int tasks[4]={3,1,2,0};
 game_init(&g);game_arcade_start(&g,&a,1);
 assert(a.active&&a.stage==0&&a.timer==140);
 for(i=0;i<4;i++){
  assert(a.stage==i&&a.target==tasks[i]);
  game_arcade_input(&g,&a,game_arcade_expected(&a));
 }
 assert(a.finished==1&&g.wins==1);
 game_arcade_start(&g,&a,1);
 game_arcade_input(&g,&a,0);
 assert(a.mistakes==1&&a.stage==0&&a.active);
 g.corruption=65;game_arcade_start(&g,&a,1);
 assert(a.timer==95);
 game_arcade_input(&g,&a,3);
 game_arcade_input(&g,&a,1);
 assert(a.stage==2&&a.reverse&&game_arcade_expected(&a)==0);
 game_arcade_input(&g,&a,0);game_arcade_input(&g,&a,0);
 assert(a.finished==1);
 game_arcade_start(&g,&a,1);
 for(i=0;i<3&&a.active;i++){
  while(a.active&&a.mistakes==(unsigned)i)game_arcade_tick(&g,&a);
 }
 assert(a.finished==2);
}
static void diaper_gestures(void){
 Game g;Arcade a;int i;
 game_init(&g);game_arcade_start(&g,&a,1);
 /* Wipe requires continuous rubbing, not a single tap. */
 game_arcade_diaper_touch(&g,&a,1,110,128);
 for(i=0;i<12&&a.stage==0;i++)
  game_arcade_diaper_touch(&g,&a,1,(i&1)?116:145,128);
 assert(a.stage==1&&a.active);
 game_arcade_diaper_touch(&g,&a,0,0,0);
 /* Lift a fresh diaper from the tray onto the baby. */
 game_arcade_diaper_touch(&g,&a,1,35,155);
 game_arcade_diaper_touch(&g,&a,1,127,135);
 game_arcade_diaper_touch(&g,&a,0,0,0);
 assert(a.stage==2);
 /* Fold with a distinct upward stroke. */
 game_arcade_diaper_touch(&g,&a,1,126,157);
 game_arcade_diaper_touch(&g,&a,1,126,110);
 game_arcade_diaper_touch(&g,&a,0,0,0);
 assert(a.stage==3);
 game_arcade_diaper_touch(&g,&a,1,83,128);
 assert(a.taped==1&&a.active);
 game_arcade_diaper_touch(&g,&a,0,0,0);
 game_arcade_diaper_touch(&g,&a,1,169,128);
 assert(a.finished==1&&g.wins==1);
}
static void rattle(void){
 Game g;Arcade a;int i;
 game_init(&g);game_arcade_start(&g,&a,7);
 assert(a.active&&a.id==7&&a.hold_frames==144);
 /* A correct lane is not enough: the toy must reach the catch window. */
 game_arcade_input(&g,&a,a.target);
 assert(a.mistakes==1&&a.round==0);
 for(i=0;i<MBAI_ROUNDS;i++){
  while(a.active&&a.timer>a.hold_frames/3u)game_arcade_tick(&g,&a);
  assert(a.active);
  game_arcade_input(&g,&a,a.target);
 }
 assert(a.finished==1&&g.wins==1);
 game_arcade_start(&g,&a,7);
 for(i=0;i<3&&a.active;i++){
  while(a.active&&a.mistakes==(unsigned)i)game_arcade_tick(&g,&a);
 }
 assert(a.finished==2);
 g.corruption=65;game_arcade_start(&g,&a,7);
 assert(a.hold_frames==108);
}
int main(void){
 bottle();diaper();diaper_gestures();rattle();
 puts("PASS: bottle physics, four diaper gestures and timed Rattle Rescue");
 return 0;
}
