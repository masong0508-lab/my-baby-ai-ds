#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "arcade_feedback.h"
static void start(Game *g,Arcade *a,ArcadeFeedback *f,int id){
 game_init(g);g->unlocked=0xffffu;
 game_arcade_start(g,a,id);
 assert(a->active);
 arcade_feedback_begin(f,a);
 assert(arcade_feedback_current(f)==ARCADE_FEEDBACK_NONE);
 assert(arcade_feedback_observe(f,a)==ARCADE_FEEDBACK_NONE);
}
static ArcadeFeedbackEvent observe(Game *g,Arcade *a,ArcadeFeedback *f){
 Game before_game=*g;
 Arcade before_arcade=*a;
 ArcadeFeedbackEvent event=arcade_feedback_observe(f,a);
 unsigned remaining=f->remaining;
 assert(memcmp(g,&before_game,sizeof(*g))==0);
 assert(memcmp(a,&before_arcade,sizeof(*a))==0);
 /* Seeing the same simulation result twice neither emits another cue
  * nor extends its display lifetime. */
 assert(arcade_feedback_observe(f,a)==ARCADE_FEEDBACK_NONE);
 assert(f->remaining==remaining);
 assert(memcmp(g,&before_game,sizeof(*g))==0);
 assert(memcmp(a,&before_arcade,sizeof(*a))==0);
 return event;
}
/* Use each cabinet's real input/tick path. This deliberately includes
 * continuous steering/watering and WATCH frames that should stay silent. */
static void play_step(Game *g,Arcade *a,int winning){
 int dir=-1;
 switch(a->id){
 case 0:case 9:{
  int steer=0;
  if(winning){
   unsigned period=a->id==0?34u:38u;
   int drift=(((a->elapsed+1u)/period)&1u)?-2:2;
   steer=a->balance>0?-1:a->balance<0?1:(drift>0?-1:1);
   if(a->reverse)steer=-steer;
  }else if(a->id==9)steer=1;
  game_arcade_steer(a,steer);
  break;
 }
 case 1:case 2:
  dir=game_arcade_expected(a);
  if(!winning)dir=(dir+1)%4;
  break;
 case 3:dir=winning?game_arcade_expected(a):a->target;break;
 case 4:
  if(a->phase){
   dir=game_arcade_expected(a);
   if(!winning)dir=(dir+1)%4;
  }
  break;
 case 5:case 10:case 12:
  dir=game_arcade_expected(a);
  if(!winning)dir=(dir+1)%(a->id==12?2:3);
  break;
 case 6:{
  unsigned phase=a->elapsed%a->hold_frames;
  if(!winning||(!a->taped&&phase>=a->hold_frames*38u/100u&&
     phase<=a->hold_frames*70u/100u))dir=0;
  break;
 }
 case 7:
  if(!winning)dir=(a->target+1)%3;
  else if(a->timer<=a->hold_frames/3u&&a->timer>=8u)dir=a->target;
  break;
 case 8:dir=winning?a->target:(a->target==0?2:0);break;
 case 11:
  if((a->direction==a->target)==winning)dir=0;
  break;
 case 13:
  if(winning)(void)game_arcade_garden_touch(g,a,1,46+82*a->target,140);
  else dir=(a->target+1)%3;
  break;
 case 14:
  if(winning){
   int node=game_arcade_expected(a);
   (void)game_arcade_court_touch(g,a,1,node==0?46:node==1?128:210,
                                node==1?92:134);
  }else dir=(game_arcade_expected(a)+1)%3;
  break;
 case 15:
  /* Three FUND then three PLAY decisions cause one actual budget strike
   * and a final out-of-bounds budget, rather than fabricating a result. */
  dir=winning?game_arcade_expected(a):(a->round<3?0:1);
  break;
 default:assert(0);break;
 }
 if(dir>=0)game_arcade_input(g,a,dir);
 if(a->active)game_arcade_tick(g,a);
}
static void all_cabinets(void){
 static const unsigned progress[16]={5,3,5,5,8,5,3,5,5,4,5,5,5,3,11,5};
 Game g;Arcade a;ArcadeFeedback f;
 unsigned id,frames,count[5];
 assert(sizeof(ArcadeFeedback)<=20u);
 for(id=0;id<MBAI_GAMES;id++){
  start(&g,&a,&f,(int)id);memset(count,0,sizeof(count));
  for(frames=0;frames<2000u&&a.active;frames++){
   (void)arcade_feedback_tick(&f);
   play_step(&g,&a,1);
   count[observe(&g,&a,&f)]++;
  }
  assert(!a.active&&a.finished==1&&g.wins==1&&g.losses==0);
  assert(count[ARCADE_FEEDBACK_PROGRESS]==progress[id]);
  assert(count[ARCADE_FEEDBACK_MISS]==0&&count[ARCADE_FEEDBACK_WIN]==1);
  assert(count[ARCADE_FEEDBACK_LOSS]==0);
  assert(arcade_feedback_current(&f)==ARCADE_FEEDBACK_WIN);
  /* The final note must replace, not accompany, the last progress cue. */
  for(unsigned i=0;i<10u;i++){
   game_arcade_tick(&g,&a);
   assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE);
  }
  assert(g.wins==1);
  start(&g,&a,&f,(int)id);memset(count,0,sizeof(count));
  for(frames=0;frames<2000u&&a.active;frames++){
   (void)arcade_feedback_tick(&f);
   play_step(&g,&a,0);
   count[observe(&g,&a,&f)]++;
  }
  assert(!a.active&&a.finished==2&&g.losses==1&&g.wins==0);
  assert(count[ARCADE_FEEDBACK_MISS]>0&&count[ARCADE_FEEDBACK_LOSS]==1);
  assert(count[ARCADE_FEEDBACK_WIN]==0);
  assert(arcade_feedback_current(&f)==ARCADE_FEEDBACK_LOSS);
  assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE);
 }
}
static void ignored_inputs_and_substeps(void){
 Game g;Arcade a;ArcadeFeedback f;
 start(&g,&a,&f,4);
 /* WATCH->REPEAT is an instruction transition, not a successful input. */
 while(!a.phase){
  game_arcade_input(&g,&a,a.notes[0]);
  game_arcade_tick(&g,&a);
  assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE);
 }
 game_arcade_input(&g,&a,game_arcade_expected(&a));
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_PROGRESS);
 game_arcade_input(&g,&a,-1);
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE);
 start(&g,&a,&f,3);
 game_arcade_input(&g,&a,game_arcade_expected(&a));
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE); /* choosing a lane */
 start(&g,&a,&f,8);
 game_arcade_input(&g,&a,a.target);
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE); /* choosing a posture */
 start(&g,&a,&f,2);
 (void)game_arcade_teether_touch(&g,&a,1,126,132);
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE);
 (void)game_arcade_teether_touch(&g,&a,1,125,86);
 (void)game_arcade_teether_touch(&g,&a,0,0,0);
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE); /* outside both trays */
 start(&g,&a,&f,14);
 game_arcade_input(&g,&a,a.notes[0]);
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_PROGRESS);
 a.touch_active=1;
 (void)game_arcade_court_touch(&g,&a,0,0,0);
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE); /* release restarts trace */
 start(&g,&a,&f,1);
 (void)game_arcade_diaper_touch(&g,&a,1,110,128);
 (void)game_arcade_diaper_touch(&g,&a,1,115,128);
 assert(a.progress>0&&a.stage==0);
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE); /* partial wipe */
 for(unsigned i=0;i<3u;i++){
  game_arcade_input(&g,&a,game_arcade_expected(&a));
  assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_PROGRESS);
 }
 (void)game_arcade_diaper_touch(&g,&a,0,0,0);
 (void)game_arcade_diaper_touch(&g,&a,1,83,128);
 assert(a.stage==3&&a.taped==1);
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_PROGRESS);
 (void)game_arcade_diaper_touch(&g,&a,1,83,128);
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE); /* holding the first tape */
 (void)game_arcade_diaper_touch(&g,&a,0,0,0);
 (void)game_arcade_diaper_touch(&g,&a,1,169,128);
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_WIN);
}
static void lifetime_and_high_water(void){
 Game g;Arcade a;ArcadeFeedback f;
 start(&g,&a,&f,0);
 while(a.round<1){play_step(&g,&a,1);(void)observe(&g,&a,&f);}
 assert(arcade_feedback_current(&f)==ARCADE_FEEDBACK_PROGRESS);
 /* Move outside the safe band without spilling, then steer back. The
  * already-earned 25-frame milestone must not chirp again on recovery. */
 while(a.round>0){
  game_arcade_steer(&a,1);game_arcade_tick(&g,&a);
  assert(a.mistakes==0);
  assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE);
 }
 while(a.round<1){
  play_step(&g,&a,1);
  assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE);
 }
 while(a.round<2){
  play_step(&g,&a,1);
  ArcadeFeedbackEvent event=observe(&g,&a,&f);
  assert(event==(a.round==2?ARCADE_FEEDBACK_PROGRESS:ARCADE_FEEDBACK_NONE));
 }
 for(unsigned i=1;i<ARCADE_FEEDBACK_TICKS;i++){
  assert(!arcade_feedback_tick(&f));
  assert(arcade_feedback_current(&f)==ARCADE_FEEDBACK_PROGRESS);
 }
 assert(arcade_feedback_tick(&f));
 assert(arcade_feedback_current(&f)==ARCADE_FEEDBACK_NONE);
 for(unsigned i=0;i<1000u;i++)assert(!arcade_feedback_tick(&f));
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE);
 /* A real event after expiry reactivates the footer; manual exit clears
  * it without announcing a loss. Beginning another session is silent. */
 while(a.round<3){play_step(&g,&a,1);(void)observe(&g,&a,&f);}
 assert(arcade_feedback_current(&f)==ARCADE_FEEDBACK_PROGRESS);
 a.active=0;
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE);
 assert(arcade_feedback_current(&f)==ARCADE_FEEDBACK_NONE);
 start(&g,&a,&f,5);
 game_arcade_input(&g,&a,game_arcade_expected(&a));
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_PROGRESS);
 game_arcade_start(&g,&a,6); /* changed cabinet cannot inherit snapshots */
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE);
 assert(arcade_feedback_current(&f)==ARCADE_FEEDBACK_NONE);
 a.active=0;a.finished=1;
 arcade_feedback_begin(&f,&a);
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE); /* no historical result */
 a.id=255;
 assert(observe(&g,&a,&f)==ARCADE_FEEDBACK_NONE);
 arcade_feedback_reset(NULL);arcade_feedback_begin(NULL,&a);
 assert(!arcade_feedback_tick(NULL));
 assert(arcade_feedback_current(NULL)==ARCADE_FEEDBACK_NONE);
 assert(arcade_feedback_observe(NULL,&a)==ARCADE_FEEDBACK_NONE);
 assert(arcade_feedback_observe(&f,NULL)==ARCADE_FEEDBACK_NONE);
}
int main(void){
 all_cabinets();
 ignored_inputs_and_substeps();
 lifetime_and_high_water();
 puts("PASS: 16 cabinets, real progress/miss/win/loss, pure snapshots, silent ignored inputs and finite feedback");
 return 0;
}
