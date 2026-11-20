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
int main(void){
 bottle();diaper();
 puts("PASS: distinct bottle physics and four-phase diaper mechanics");
 return 0;
}
