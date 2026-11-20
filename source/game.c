#include "game.h"
#include "dialogue.h"
#include <string.h>
const char *const game_names[MBAI_GAMES]={
 "Bottle Balance","Diaper Dash","Tax Return Teether","Crib Kart",
 "Baby Beatbox","Sock Market","Naptime Negotiator","Rattle Rescue",
 "Corporate Crawl","Stroller Drift","Cereal Cipher","Tantrum Blocks",
 "Baby's First Audit","Glitch Gardening","Courtroom Crayons","CEO of Daycare"
};
static uint8_t clamp(int n){return n<0?0:n>100?100:(uint8_t)n;}
static uint32_t next(Game *s){uint32_t x=s->rng?s->rng:0xA17u;x^=x<<13;x^=x>>17;x^=x<<5;return s->rng=x;}
void game_init(Game *s){memset(s,0,sizeof(*s));s->rng=0xBADA551u;s->days=1;s->dollars=8;s->favors=1;s->unlocked=255;s->hunger=35;s->mood=75;s->energy=75;s->chaos=12;s->corruption=2;}
unsigned game_glitch_tier(const Game *s){return s->corruption>=80?4:s->corruption>=60?3:s->corruption>=40?2:s->corruption>=20?1:0;}
const char *game_care(Game *s,Care action){
 switch(action){
 case FEED:s->hunger=clamp(s->hunger-30);s->mood=clamp(s->mood+3);break;
 case CHAT:s->mood=clamp(s->mood+7);s->energy=clamp(s->energy-3);s->corruption=clamp(s->corruption+2);break;
 case PLAY:s->mood=clamp(s->mood+10);s->hunger=clamp(s->hunger+12);s->energy=clamp(s->energy-12);s->chaos=clamp(s->chaos+12);break;
 case NAP:s->energy=clamp(s->energy+25);s->hunger=clamp(s->hunger+8);s->chaos=clamp(s->chaos-5);break;
 default:return "I CANNOT PROCESS THIS.";
 }
 if(s->hunger>80||s->energy<15)s->corruption=clamp(s->corruption+5);
 if(s->chaos>75)s->corruption=clamp(s->corruption+3);
 return dialogue_reply(s,action);
}
int game_unlocked(const Game *s,int id){return id>=0&&id<MBAI_GAMES&&!!(s->unlocked&(1u<<id));}
int game_unlock(Game *s,int id){
 int level=id-8;unsigned cost,favors;
 if(level<0||id>=MBAI_GAMES||game_unlocked(s,id))return 0;
 cost=10u+level*2u;favors=(unsigned)(level/2)+1u;
 if(s->wins<(unsigned)((level+1)*2)||s->dollars<cost||s->favors<favors)return 0;
 s->dollars-=cost;s->favors-=favors;s->unlocked|=(uint16_t)(1u<<id);return 1;
}
/* Ending IDs 1..4 are bad, IDs 5..8 are good. The existing 16-bit
 * ending_mask records both categories without changing on-disk save layout. */
static uint16_t add_currency(uint16_t value,unsigned amount){
 return (uint16_t)(value>65535u-amount?65535u:value+amount);
}
int game_is_good_ending(int ending){
 return ending>=END_GRADUATION && ending<=END_SECOND_CHANCE;
}
int game_good_ending_count(const Game *s){
 uint16_t x=(uint16_t)((s->ending_mask>>END_GRADUATION)&15u);
 int n=0;while(x){n+=(x&1u);x>>=1;}return n;
}
int game_end_day(Game *s){
 int e=END_OK;
 /* A real bad ending takes priority over a positive outcome. */
 if(s->corruption>=80&&s->chaos>=45)e=END_REBOOT;
 else if(s->hunger>=78)e=END_BOTTLE;
 else if(s->mood<=24)e=END_CEO;
 else if(s->chaos>=72)e=END_INTERN;
 if(e!=END_OK){
  if(s->bad_endings<65535u)s->bad_endings++;
  s->ending_mask|=(uint16_t)(1u<<e);
  s->corruption=clamp(s->corruption+15);
  s->hunger=clamp(s->hunger-35);s->energy=clamp(s->energy+30);
  s->chaos=clamp(s->chaos-35);s->mood=clamp(s->mood+30);
 }else{
  /* Each positive story finale occurs only once per save. All return to
   * nursery; a finale is never a forced exit or an implicit save wipe. */
  if(s->days>=10&&s->bad_endings>0&&s->hunger<=35&&s->mood>=85&&
     s->energy>=55&&s->chaos<=40&&!(s->ending_mask&(1u<<END_SECOND_CHANCE)))
   e=END_SECOND_CHANCE;
  else if(s->wins>=6&&s->dollars>=25&&s->mood>=65&&s->hunger<=65&&
          !(s->ending_mask&(1u<<END_ARCADE_CHAMP)))
   e=END_ARCADE_CHAMP;
  else if(s->days>=5&&s->corruption>=40&&s->corruption<80&&
          s->chaos<=35&&s->energy>=50&&s->mood>=60&&
          !(s->ending_mask&(1u<<END_GLITCH_TAMER)))
   e=END_GLITCH_TAMER;
  else if(s->days>=3&&s->hunger<=30&&s->mood>=80&&s->energy>=65&&
          s->chaos<=35&&!(s->ending_mask&(1u<<END_GRADUATION)))
   e=END_GRADUATION;
  if(game_is_good_ending(e)){
   s->ending_mask|=(uint16_t)(1u<<e);
   s->dollars=add_currency(s->dollars,8);
   s->favors=add_currency(s->favors,1);
   s->corruption=clamp(s->corruption-8);
  }
 }
 s->days++;
 return e;
}
int game_reset_unlocked(const Game *s){
 /* Count only distinct BAD endings. Good endings must not unlock a wipe. */
 uint16_t x=(uint16_t)(s->ending_mask&0x001eu);int unique=0;
 while(x){unique+=(x&1u);x>>=1;}
 return s->bad_endings>=5||unique>=3;
}
const char *game_ending_name(int e){
 switch(e){
 case END_BOTTLE:return "BOTTLE SUBSCRIPTION ENDED";
 case END_CEO:return "BABY CEO HAS QUIT";
 case END_REBOOT:return "MANDATORY DAYCARE REBOOT";
 case END_INTERN:return "UNPAID INTERN REVOLT";
 case END_GRADUATION:return "TINY DIPLOMA CEREMONY";
 case END_ARCADE_CHAMP:return "RATTLE ARCADE CHAMPION";
 case END_GLITCH_TAMER:return "BUGS BECOME FEATURES";
 case END_SECOND_CHANCE:return "SECOND-CHANCE DAYCARE";
 default:return "A NORMAL DAY. SUSPICIOUS.";
 }
}
/* Cabinets 0, 1 and 7 have independent real-time mechanics.
 * The other cabinets retain the six-round directional game for now.
 * Arcade state is intentionally not part of the persistent Game save.
 */
static const uint8_t diaper_tasks[4]={3,1,2,0}; /* left,right,down,up */
static void diaper_next(Game *s,Arcade *a){
 a->target=diaper_tasks[a->stage];
 a->progress=0;a->taped=0;
 a->reverse=(uint8_t)(game_glitch_tier(s)>=2&&a->stage==2);
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?240:360);
}
static void rattle_next(Game *s,Arcade *a){
 a->target=(uint8_t)(next(s)%3u);
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?108:144);
 a->hold_frames=a->timer; /* original fall duration for drawing */
 a->elapsed=0;
}
static void round_next(Game *s,Arcade *a){
 a->direction=(uint8_t)(next(s)&3u);
 a->reverse=(uint8_t)(game_glitch_tier(s)>=2&&a->round%3==2);
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?75:120);
}
void game_arcade_start(Game *s,Arcade *a,int id){
 memset(a,0,sizeof(*a));
 if(!game_unlocked(s,id))return;
 a->active=1;a->id=(uint8_t)id;
 if(id==0){
  a->timer=600;
  a->reverse=(uint8_t)(game_glitch_tier(s)>=2);
 }else if(id==1)diaper_next(s,a);
 else if(id==7)rattle_next(s,a);
 else round_next(s,a);
}
int game_arcade_expected(const Arcade *a){
 if(a->id==1)return a->target^(a->reverse?2:0);
 if(a->id==7)return a->target;
 return a->direction^(a->reverse?2:0);
}
static void finish(Game *s,Arcade *a,int won){
 a->active=0;a->finished=(uint8_t)(won?1:2);
 if(won){
  s->wins++;
  s->dollars=(uint16_t)(s->dollars<=65530?s->dollars+5:65535);
  if(!(s->wins&1u)&&s->favors<65535)s->favors++;
  s->mood=clamp(s->mood+4);
 }else{
  s->losses++;s->mood=clamp(s->mood-8);
  s->chaos=clamp(s->chaos+15);s->corruption=clamp(s->corruption+18);
 }
}
void game_arcade_steer(Arcade *a,int direction){
 int force;
 if(!a->active||a->id!=0)return;
 force=direction<0?-4:direction>0?4:0;
 a->steering=(int8_t)(a->reverse?-force:force);
}
void game_arcade_input(Game *s,Arcade *a,int dir){
 if(!a->active||a->id==0||dir<0||dir>3)return;
 if(a->id==7){
  /* Match the lane only when the falling rattle enters the catch zone.
   * Tapping immediately on spawn is not a valid catch. */
  if(dir==a->target&&a->timer<=a->hold_frames/3u&&a->timer>=8){
   a->round++;
   if(a->round>=MBAI_ROUNDS)finish(s,a,1);
   else rattle_next(s,a);
  }else{
   a->mistakes++;s->corruption=clamp(s->corruption+3);
   if(a->mistakes>=3)finish(s,a,0);
   else rattle_next(s,a);
  }
  return;
 }
 if(dir==game_arcade_expected(a)){
  if(a->id==1){
   a->stage++;a->round=a->stage;
   if(a->stage>=4)finish(s,a,1);
   else diaper_next(s,a);
  }else{
   a->round++;
   if(a->round>=MBAI_ROUNDS)finish(s,a,1);
   else round_next(s,a);
  }
 }else{
  a->mistakes++;s->corruption=clamp(s->corruption+4);
  if(a->mistakes>=3)finish(s,a,0);
  else if(a->id==1)diaper_next(s,a);
  else if(a->id==7)rattle_next(s,a);
  else round_next(s,a);
 }
}
static int inside(int x,int y,int x0,int y0,int x1,int y1){
 return x>=x0&&x<x1&&y>=y0&&y<y1;
}
static int diaper_advance(Game *s,Arcade *a,int still_touching){
 a->stage++;a->round=a->stage;
 if(a->stage>=4)finish(s,a,1);
 else diaper_next(s,a);
 a->touch_active=(uint8_t)still_touching;
 return 1;
}
/* Continuous drag/swipe/tap gestures. The four tasks are intentionally
 * different, rather than four reskinned directional prompts.
 * Return nonzero only when the visible state needs repainting. */
int game_arcade_diaper_touch(Game *s,Arcade *a,int touching,int x,int y){
 int dx,dy,step;
 if(!a->active||a->id!=1)return 0;
 if(!touching){
  int success=0;
  if(a->touch_active&&a->stage==1)
   success=inside(a->touch_start_x,a->touch_start_y,12,143,77,175)&&
           inside(a->touch_last_x,a->touch_last_y,102,113,166,158);
  else if(a->touch_active&&a->stage==2)
   success=inside(a->touch_start_x,a->touch_start_y,95,141,164,175)&&
           inside(a->touch_last_x,a->touch_last_y,95,93,164,125);
  a->touch_active=0;
  if(success)return diaper_advance(s,a,0);
  return 0;
 }
 if(x<0||x>=256||y<27||y>=176)return 0;
 if(!a->touch_active){
  a->touch_active=1;
  a->touch_start_x=a->touch_last_x=(int16_t)x;
  a->touch_start_y=a->touch_last_y=(int16_t)y;
  if(a->stage==3){
   if(a->taped==0&&inside(x,y,66,115,113,150)){
    a->taped=1;return 1;
   }
   if(a->taped==1&&inside(x,y,143,115,191,150))
    return diaper_advance(s,a,1);
  }
  return 0;
 }
 if(a->stage==0&&inside(x,y,89,103,169,151)){
  dx=x-a->touch_last_x;dy=y-a->touch_last_y;
  if(dx<0)dx=-dx;
  if(dy<0)dy=-dy;
  step=dx+dy;
  if(step>=3&&step<=48){
   int total=a->progress+(step>18?18:step);
   a->progress=(uint8_t)(total>100?100:total);
   if(a->progress>=90)return diaper_advance(s,a,1);
   a->touch_last_x=(int16_t)x;a->touch_last_y=(int16_t)y;
   return 0; /* periodic UI repaint shows progress without VRAM tearing */
  }
 }
 a->touch_last_x=(int16_t)x;a->touch_last_y=(int16_t)y;
 return 0;
}
static void bottle_tick(Game *s,Arcade *a){
 int drift;
 a->elapsed++;
 drift=((a->elapsed/34u)&1u)?-2:2;
 a->balance+=(int16_t)(drift+a->steering);
 a->steering=0;
 if(a->balance>62||a->balance< -62){
  a->mistakes++;
  a->balance=0;a->hold_frames=0;
  s->corruption=clamp(s->corruption+3);
  if(a->mistakes>=3){finish(s,a,0);return;}
 }
 if(a->balance>=-22&&a->balance<=22){
  if(a->hold_frames<150)a->hold_frames++;
 }else{
  a->hold_frames=(uint16_t)(a->hold_frames>2?a->hold_frames-2:0);
 }
 a->round=(uint8_t)((a->hold_frames*6u)/150u);
 if(a->hold_frames>=150){finish(s,a,1);return;}
 if(a->timer)a->timer--;
 if(!a->timer)finish(s,a,0);
}
void game_arcade_tick(Game *s,Arcade *a){
 if(!a->active)return;
 if(a->id==0){bottle_tick(s,a);return;}
 if(a->timer)a->timer--;
 if(!a->timer){
  a->mistakes++;s->corruption=clamp(s->corruption+3);
  if(a->mistakes>=3)finish(s,a,0);
  else if(a->id==1)diaper_next(s,a);
  else if(a->id==7)rattle_next(s,a);
  else round_next(s,a);
 }
}
