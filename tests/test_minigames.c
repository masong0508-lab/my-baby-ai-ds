#include <assert.h>
#include <stdio.h>
#include <string.h>
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
 assert(a.active&&a.stage==0&&a.timer==360);
 for(i=0;i<4;i++){
  assert(a.stage==i&&a.target==tasks[i]);
  game_arcade_input(&g,&a,game_arcade_expected(&a));
 }
 assert(a.finished==1&&g.wins==1);
 game_arcade_start(&g,&a,1);
 game_arcade_input(&g,&a,0);
 assert(a.mistakes==1&&a.stage==0&&a.active);
 g.corruption=65;game_arcade_start(&g,&a,1);
 assert(a.timer==240);
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
static void tax_receipts(void){
 Game g;Arcade a;int i;
 game_init(&g);game_arcade_start(&g,&a,2);
 assert(a.active&&a.id==2&&a.timer==240);
 /* Drop outside both trays should not consume a life. */
 game_arcade_teether_touch(&g,&a,1,126,132);
 game_arcade_teether_touch(&g,&a,1,125,86);
 game_arcade_teether_touch(&g,&a,0,0,0);
 assert(a.mistakes==0&&a.round==0);
 for(i=0;i<MBAI_ROUNDS;i++){
  int x;
  assert(a.round==i);
  x=a.target==3?47:203;
  game_arcade_teether_touch(&g,&a,1,126,132);
  game_arcade_teether_touch(&g,&a,1,x,150);
  game_arcade_teether_touch(&g,&a,0,0,0);
 }
 assert(a.finished==1&&g.wins==1);
 game_arcade_start(&g,&a,2);
 game_arcade_input(&g,&a,a.target==3?1:3);
 assert(a.mistakes==1&&a.round==0);
 for(i=1;i<3&&a.active;i++){
  while(a.active&&a.mistakes==(unsigned)i)game_arcade_tick(&g,&a);
 }
 assert(a.finished==2);
 g.corruption=65;game_arcade_start(&g,&a,2);
 assert(a.timer==180);
}
static void baby_beatbox(void){
 Game g;Arcade a;int i,j;
 game_init(&g);game_arcade_start(&g,&a,4);
 assert(a.active&&a.id==4&&a.timer==90&&a.hold_frames==90&&!a.phase);
 game_arcade_input(&g,&a,a.notes[0]);
 assert(a.stage==0&&a.round==0); /* Cannot skip the demonstration. */
 for(i=0;i<3;i++){
  while(a.active&&!a.phase)game_arcade_tick(&g,&a);
  assert(a.active&&a.phase==1&&a.timer==240);
  for(j=0;j<3;j++){
   assert(a.stage==j);
   game_arcade_input(&g,&a,a.notes[j]);
  }
 }
 assert(a.finished==1&&g.wins==1);
 game_arcade_start(&g,&a,4);
 while(!a.phase)game_arcade_tick(&g,&a);
 game_arcade_input(&g,&a,(a.notes[0]+1)&3);
 assert(a.mistakes==1&&!a.phase);
 for(i=1;i<3&&a.active;i++){
  while(a.active&&!a.phase)game_arcade_tick(&g,&a);
  game_arcade_input(&g,&a,(a.notes[0]+1)&3);
 }
 assert(a.finished==2);
 g.corruption=65;game_arcade_start(&g,&a,4);
 assert(a.timer==72);
}
static void crib_kart(void){
 Game g;Arcade a;int i;
 game_init(&g);game_arcade_start(&g,&a,3);
 assert(a.active&&a.id==3&&a.stage==1&&a.hold_frames==90);
 for(i=0;i<MBAI_ROUNDS;i++){
  int lane=game_arcade_expected(&a);
  assert(lane>=0&&lane<3&&lane!=a.target);
  game_arcade_input(&g,&a,lane);
  while(a.active&&a.round==(unsigned)i)game_arcade_tick(&g,&a);
 }
 assert(a.finished==1&&g.wins==1);
 game_arcade_start(&g,&a,3);
 for(i=0;i<3&&a.active;i++){
  game_arcade_input(&g,&a,a.target);
  while(a.active&&a.mistakes==(unsigned)i)game_arcade_tick(&g,&a);
 }
 assert(a.finished==2&&a.round==0&&g.losses==1);
 g.corruption=65;game_arcade_start(&g,&a,3);
 assert(a.timer==66&&a.hold_frames==66);
}
static void sock_market(void){
 Game g;Arcade a;int i;
 game_init(&g);game_arcade_start(&g,&a,5);
 assert(a.active&&a.id==5&&a.timer==210);
 for(i=0;i<MBAI_ROUNDS;i++){
  int target_index=game_arcade_expected(&a);
  assert(target_index>=0&&target_index<3);
  assert(a.notes[target_index]==a.target);
  assert(a.notes[0]!=a.notes[1]&&a.notes[1]!=a.notes[2]&&
         a.notes[0]!=a.notes[2]);
  game_arcade_input(&g,&a,target_index);
  assert(a.round==(unsigned)(i+1));
 }
 assert(a.finished==1&&g.wins==1);
 game_arcade_start(&g,&a,5);
 for(i=0;i<3&&a.active;i++){
  int wrong=(game_arcade_expected(&a)+1)%3;
  game_arcade_input(&g,&a,wrong);
 }
 assert(a.finished==2&&a.mistakes==3);
 g.corruption=65;game_arcade_start(&g,&a,5);
 assert(a.timer==150);
}
static void naptime(void){
 Game g;Arcade a;int i;
 game_init(&g);game_arcade_start(&g,&a,6);
 assert(a.active&&a.id==6&&a.timer==600&&a.hold_frames==100);
 game_arcade_input(&g,&a,0);
 assert(a.mistakes==1&&a.round==0); /* Early wake-up costs a strike. */
 for(i=0;i<4;i++){
  unsigned green_start=a.hold_frames*38u/100u;
  /* Advance at least once: subsequent successes need a NEW lullaby cycle. */
  do{game_arcade_tick(&g,&a);}
  while(a.active&&a.elapsed%a.hold_frames!=green_start);
  assert(a.active);
  game_arcade_input(&g,&a,0);
  assert(a.round==(unsigned)(i+1));
  if(a.active){
   game_arcade_input(&g,&a,0); /* Can't spam the same quiet window. */
   assert(a.round==(unsigned)(i+1));
  }
 }
 assert(a.finished==1&&g.wins==1);
 game_arcade_start(&g,&a,6);
 for(i=0;i<3&&a.active;i++)game_arcade_input(&g,&a,0);
 assert(a.finished==2&&a.mistakes==3);
 game_arcade_start(&g,&a,6);
 while(a.active)game_arcade_tick(&g,&a);
 assert(a.finished==2); /* Global calming-session timeout. */
 g.corruption=65;game_arcade_start(&g,&a,6);
 assert(a.hold_frames==80&&a.timer==420);
}

static void corporate_crawl(void){
 Game g;Arcade a;int i;
 game_init(&g);g.unlocked=0xffffu;game_arcade_start(&g,&a,8);
 assert(a.active&&a.timer==82);
 for(i=0;i<MBAI_ROUNDS;i++){
  int correct=a.target;
  assert(correct==0||correct==2);
  game_arcade_input(&g,&a,correct);
  while(a.active&&a.round==(unsigned)i)game_arcade_tick(&g,&a);
 }
 assert(a.finished==1&&g.wins==1);
 game_arcade_start(&g,&a,8);
 for(i=0;i<3&&a.active;i++){
  game_arcade_input(&g,&a,a.target==0?2:0);
  while(a.active&&a.mistakes==(unsigned)i)game_arcade_tick(&g,&a);
 }
 assert(a.finished==2&&a.mistakes==3);
 g.corruption=65;game_arcade_start(&g,&a,8);
 assert(a.timer==58);
}
static void stroller_drift(void){
 Game g;Arcade a;int i;
 game_init(&g);g.unlocked=0xffffu;game_arcade_start(&g,&a,9);
 assert(a.active&&a.timer==660);
 for(i=0;i<660&&a.active;i++){
  int drift=(((a.elapsed+1u)/38u)&1u)?-2:2;
  int steer=a.balance>0?-1:a.balance<0?1:(drift>0?-1:1);
  game_arcade_steer(&a,steer);
  game_arcade_tick(&g,&a);
 }
 assert(a.finished==1&&g.wins==1&&a.round==5);
 game_arcade_start(&g,&a,9);
 for(i=0;i<660&&a.active;i++){
  game_arcade_steer(&a,1);
  game_arcade_tick(&g,&a);
 }
 assert(a.finished==2&&a.mistakes==3);
 g.corruption=65;game_arcade_start(&g,&a,9);
 assert(a.timer==510);
}
static void cereal_cipher(void){
 Game g;Arcade a;int i;
 game_init(&g);g.unlocked=0xffffu;game_arcade_start(&g,&a,10);
 for(i=0;i<MBAI_ROUNDS;i++){
  int correct=a.stage+a.direction;
  assert(a.timer==180&&a.target<3);
  assert(a.notes[a.target]==correct);
  assert(a.notes[1]==a.notes[0]+1&&a.notes[2]==a.notes[1]+1);
  game_arcade_input(&g,&a,a.target);
 }
 assert(a.finished==1&&g.wins==1);
 game_arcade_start(&g,&a,10);
 for(i=0;i<3&&a.active;i++)
  game_arcade_input(&g,&a,(a.target+1)%3);
 assert(a.finished==2);
 g.corruption=65;game_arcade_start(&g,&a,10);
 assert(a.timer==120);
}
static void tantrum_blocks(void){
 Game g;Arcade a;int i,frames;
 game_init(&g);g.unlocked=0xffffu;game_arcade_start(&g,&a,11);
 for(i=0;i<MBAI_ROUNDS;i++){
  frames=0;
  while(a.active&&a.direction!=a.target&&frames++<180)
   game_arcade_tick(&g,&a);
  assert(a.active&&a.direction==a.target);
  game_arcade_input(&g,&a,0);
 }
 assert(a.finished==1&&g.wins==1);
 game_arcade_start(&g,&a,11);
 for(i=0;i<3&&a.active;i++){
  while(a.active&&a.direction==a.target)game_arcade_tick(&g,&a);
  game_arcade_input(&g,&a,0);
 }
 assert(a.finished==2&&a.mistakes==3);
 g.corruption=65;game_arcade_start(&g,&a,11);
 assert(a.hold_frames==11&&a.timer==130);
}
static void first_audit(void){
 Game g;Arcade a;int i;
 game_init(&g);g.unlocked=0xffffu;game_arcade_start(&g,&a,12);
 for(i=0;i<MBAI_ROUNDS;i++){
  assert((int)a.target==(a.stage!=a.direction));
  assert(a.timer==180);
  game_arcade_input(&g,&a,a.target);
 }
 assert(a.finished==1&&g.wins==1);
 game_arcade_start(&g,&a,12);
 for(i=0;i<3&&a.active;i++)
  game_arcade_input(&g,&a,1-a.target);
 assert(a.finished==2);
 g.corruption=65;game_arcade_start(&g,&a,12);
 assert(a.timer==125);
}
static void glitch_gardening(void){
 Game g;Arcade a;int i,j;
 game_init(&g);g.unlocked=0xffffu;game_arcade_start(&g,&a,13);
 assert(a.active&&a.timer==540);
 game_arcade_input(&g,&a,(a.target+1)%3);
 assert(a.mistakes==1&&a.progress==0);
 for(i=0;i<4;i++){
  int x=46+82*a.target;
  for(j=0;j<50&&a.active&&a.round==(unsigned)i;j++){
   int dirty=game_arcade_garden_touch(&g,&a,1,x,140);
   if(j==0)assert(!dirty&&a.progress==2);
   if(j==3)assert(dirty&&a.progress==8);
  }
  assert(a.round==(unsigned)(i+1));
 }
 assert(a.finished==1&&g.wins==1);
 game_arcade_start(&g,&a,13);
 while(a.active)game_arcade_tick(&g,&a);
 assert(a.finished==2);
 g.corruption=65;game_arcade_start(&g,&a,13);
 assert(a.timer==390);
}
static void courtroom_crayons(void){
 static const int xs[3]={46,128,210},ys[3]={134,92,134};
 Game g;Arcade a;int i,j;
 game_init(&g);g.unlocked=0xffffu;game_arcade_start(&g,&a,14);
 game_arcade_court_touch(&g,&a,1,xs[a.notes[0]],ys[a.notes[0]]);
 assert(a.stage==1);
 game_arcade_court_touch(&g,&a,0,0,0);
 assert(a.stage==0); /* Releasing an incomplete stroke restarts. */
 for(i=0;i<4;i++){
  for(j=0;j<3;j++){
   int node=a.notes[j];
   game_arcade_court_touch(&g,&a,1,xs[node],ys[node]);
  }
  assert(a.round==(unsigned)(i+1));
 }
 assert(a.finished==1&&g.wins==1);
 game_arcade_start(&g,&a,14);
 for(i=0;i<3&&a.active;i++)
  game_arcade_input(&g,&a,(a.notes[0]+1)%3);
 assert(a.finished==2);
 g.corruption=65;game_arcade_start(&g,&a,14);
 assert(a.timer==180);
}
/* A timed-out question must still agree with its visible answer choices.
 * These cabinets once fell into the retired direction-game retry path,
 * leaving stale answers/evidence behind a newly changed prompt. */
static void check_retry_prompt(const Game *g,const Arcade *a){
 unsigned fast=game_glitch_tier(g)>=3;
 int i,matches=0;
 assert(a->active&&!a->finished);
 if(a->id==10){
  assert(a->timer==(fast?120:180));
  assert(a->stage>=1&&a->stage<=4);
  assert(a->direction>=1&&a->direction<=4);
  for(i=0;i<3;i++)if(a->notes[i]==a->stage+a->direction){
   matches++;assert(i==a->target);
  }
  assert(matches==1);
 }else if(a->id==12){
  assert(a->timer==(fast?125:180));
  assert(a->stage>=20&&a->stage<=69);
  assert(a->direction==a->stage||a->direction==a->stage+5);
  assert(a->target==(a->stage==a->direction?0:1));
 }else{
  unsigned mask=0;
  assert(a->id==14&&a->timer==(fast?180:270));
  assert(a->stage==0&&!a->touch_active);
  for(i=0;i<3;i++){
   assert(a->notes[i]<3);
   mask|=1u<<a->notes[i];
  }
  assert(mask==7u);
 }
}
static void answer_retry_round(Game *g,Arcade *a){
 if(a->id==14){
  int i;
  for(i=0;i<3;i++)game_arcade_input(g,a,a->notes[a->stage]);
 }else game_arcade_input(g,a,a->target);
}
static void timed_question_recovery(void){
 static const int ids[3]={10,12,14};
 static const unsigned corruption[4]={2,58,65,98};
 static const int xs[3]={46,128,210},ys[3]={134,92,134};
 Game g;Arcade a;
 int cabinet,tier,seed,strike,i;
 for(cabinet=0;cabinet<3;cabinet++)for(tier=0;tier<4;tier++)
 for(seed=1;seed<=16;seed++){
  game_init(&g);g.unlocked=0xffffu;g.rng=(uint32_t)seed;
  g.corruption=(uint8_t)corruption[tier];
  game_arcade_start(&g,&a,ids[cabinet]);
  answer_retry_round(&g,&a);
  assert(a.round==1);
  for(strike=1;strike<=2;strike++){
   unsigned duration=a.timer,round=a.round;
   if(a.id==14){
    /* Time out halfway through a real stylus evidence gesture. */
    int node=a.notes[0];
    game_arcade_court_touch(&g,&a,1,xs[node],ys[node]);
    assert(a.stage==1&&a.touch_active);
   }
   for(i=0;i<(int)duration;i++)game_arcade_tick(&g,&a);
   assert(a.mistakes==strike&&a.round==round);
   assert(g.wins==0&&g.losses==0&&g.dollars==8&&g.favors==1);
   check_retry_prompt(&g,&a);
   answer_retry_round(&g,&a);
   assert(a.round==round+1);
  }
  while(a.active)answer_retry_round(&g,&a);
  assert(a.finished==1&&g.wins==1&&g.losses==0);
  assert(g.dollars==13&&g.favors==1);
  for(i=0;i<5;i++)game_arcade_tick(&g,&a);
  assert(g.wins==1&&g.dollars==13);

  /* The third timeout still ends the game exactly once, with no reward. */
  game_init(&g);g.unlocked=0xffffu;g.rng=(uint32_t)seed;
  g.corruption=(uint8_t)corruption[tier];
  game_arcade_start(&g,&a,ids[cabinet]);
  for(strike=1;strike<=3;strike++){
   unsigned duration=a.timer;
   for(i=0;i<(int)duration;i++)game_arcade_tick(&g,&a);
   assert(a.mistakes==strike&&a.round==0);
   if(strike<3)check_retry_prompt(&g,&a);
  }
  assert(!a.active&&a.finished==2&&g.losses==1&&g.wins==0);
  assert(g.dollars==8&&g.favors==1);
  for(i=0;i<5;i++)game_arcade_tick(&g,&a);
  assert(g.losses==1&&g.wins==0&&g.dollars==8);
 }
}
static void ceo_of_daycare(void){
 Game g;Arcade a;int i;
 game_init(&g);g.unlocked=0xffffu;game_arcade_start(&g,&a,15);
 assert(a.balance==55&&a.progress==45&&a.timer==540);
 for(i=0;i<6;i++)game_arcade_input(&g,&a,(i&1)?1:0);
 assert(a.finished==1&&g.wins==1&&a.balance>=35&&a.progress>=35);
 game_arcade_start(&g,&a,15);
 for(i=0;i<6&&a.active;i++)game_arcade_input(&g,&a,2);
 assert(a.finished==2); /* Nap-only business spends all the cash. */
 g.corruption=65;game_arcade_start(&g,&a,15);
 assert(a.timer==390);
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
 while(a.active&&a.mistakes==0)game_arcade_tick(&g,&a);
 assert(a.active&&a.mistakes==1&&a.timer==a.hold_frames);
 for(i=1;i<3&&a.active;i++){
  while(a.active&&a.mistakes==(unsigned)i)game_arcade_tick(&g,&a);
 }
 assert(a.finished==2);
 g.corruption=65;game_arcade_start(&g,&a,7);
 assert(a.hold_frames==108);
}
/* Practice uses the real cabinet engine but a separate state. Cover
 * every paid/free cabinet, its normal difficulty, timers and live-save
 * isolation even after repeated mistakes/timeouts. */
static void practice_all(void){
 Game live,original,sandbox;Arcade a;
 static const unsigned goal[16]={6,4,6,6,3,6,4,6,6,5,6,6,6,4,4,6};
 game_init(&live);live.corruption=95;live.wins=7;live.losses=3;
 original=live;
 for(int id=0;id<MBAI_GAMES;id++){
  assert(game_arcade_goal((unsigned)id)==goal[id]);
  assert(game_arcade_practice_start(&live,&sandbox,&a,id));
  assert(a.active&&a.practice&&a.id==id&&sandbox.corruption==0);
  assert(a.timer_limit==a.timer&&a.timer>0);
  assert(game_unlocked(&sandbox,id));
  assert(memcmp(&live,&original,sizeof(live))==0);
  for(unsigned tick=0;tick<2500u&&a.active;tick++)
   game_arcade_tick(&sandbox,&a);
  assert(!a.active&&a.finished);
  assert(memcmp(&live,&original,sizeof(live))==0);
 }
 assert(!game_arcade_goal(16u));
 assert(!game_arcade_practice_start(&live,&live,&a,0));
 assert(!game_arcade_practice_start(&live,&sandbox,&a,16));
 assert(memcmp(&live,&original,sizeof(live))==0);
 assert(game_arcade_practice_start(&live,&sandbox,&a,4));
 assert(game_arcade_beat_slot(&a)==0);
 while(a.elapsed<30u)game_arcade_tick(&sandbox,&a);
 assert(game_arcade_beat_slot(&a)==1);
 while(a.elapsed<60u)game_arcade_tick(&sandbox,&a);
 assert(game_arcade_beat_slot(&a)==2);
 while(!a.phase)game_arcade_tick(&sandbox,&a);
 assert(game_arcade_beat_slot(&a)==-1&&a.timer_limit==240u&&a.timer==240u);
 /* Normal play is still locked and never inherits practice mode. */
 game_arcade_start(&live,&a,8);assert(!a.active&&!a.practice);
 game_arcade_start(&live,&a,0);assert(a.active&&!a.practice);
 assert(memcmp(&live,&original,sizeof(live))==0);
}
static void arcade_counter_limits(void){
 Game g;Arcade a;
 game_init(&g);g.unlocked=UINT16_MAX;g.wins=UINT32_MAX-1u;
 game_arcade_start(&g,&a,10);
 for(unsigned i=0;i<6u;i++)game_arcade_input(&g,&a,game_arcade_expected(&a));
 assert(g.wins==UINT32_MAX);
 game_arcade_start(&g,&a,10);
 for(unsigned i=0;i<6u;i++)game_arcade_input(&g,&a,game_arcade_expected(&a));
 assert(g.wins==UINT32_MAX);
 g.losses=UINT32_MAX;
 game_arcade_start(&g,&a,10);
 for(unsigned i=0;i<3u;i++)game_arcade_input(&g,&a,(game_arcade_expected(&a)+1)%3);
 assert(g.losses==UINT32_MAX&&a.finished==2);
 /* A released stylus must not constantly redraw a button-only trace. */
 game_arcade_start(&g,&a,14);
 game_arcade_input(&g,&a,game_arcade_expected(&a));
 assert(a.stage==1&&!game_arcade_court_touch(&g,&a,0,0,0)&&a.stage==1);
}
int main(void){
 practice_all();arcade_counter_limits();
 bottle();diaper();diaper_gestures();tax_receipts();
 crib_kart();baby_beatbox();sock_market();naptime();rattle();
 corporate_crawl();stroller_drift();cereal_cipher();tantrum_blocks();
 first_audit();glitch_gardening();courtroom_crayons();ceo_of_daycare();
 timed_question_recovery();
 puts("PASS: all 16 arcade mechanics, touch gestures, collision, memory, strategy, timers");
 return 0;
}
