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
/* One readable next-step hint instead of four unlabeled competing needs.
 * Exhaustion at critical levels wins first, then hunger, recoverable sleep
 * / chaos, and finally social connection. No RNG or mutation. */
int game_care_hint(const Game *s){
 if(!s)return -1;
 if(s->energy<=15)return NAP;
 if(s->hunger>=70)return FEED;
 if(s->energy<=35||s->chaos>=70)return NAP;
 if(s->mood<=40)return CHAT;
 return -1;
}
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
/* A meaningful, accessible bedroom interaction: when the player has
 * explicitly chosen moonlight AND switched the lamp off, using the crib
 * grants a small extra rest and calms the toy's chaotic mood. This is
 * conditional on the actual Room controls, not random cosmetic text.
 * The existing game_care(NAP) always runs first and keeps save semantics. */
int game_room_quiet_rest(Game *s,int daylight,int lamp_on){
 if(!s||daylight||lamp_on)return 0;
 s->energy=clamp(s->energy+8);
 s->chaos=clamp((int)s->chaos-4);
 /* A deliberately quiet night can gradually settle acquired glitches.
  * Without any repeatable recovery, CHAT and arcade mistakes push
  * corruption upward permanently over a long-lived save. This is
  * bounded, requires a real moonlit/lamp-off crib interaction and does
  * not erase an ending record or a player's discovered memories. */
 s->corruption=clamp((int)s->corruption-2);
 return 1;
}
int game_unlocked(const Game *s,int id){return id>=0&&id<MBAI_GAMES&&!!(s->unlocked&(1u<<id));}
int game_unlock_requirements(int id,unsigned *wins,
                             unsigned *dollars,unsigned *favors){
 unsigned level;
 if(id<8||id>=MBAI_GAMES)return 0;
 level=(unsigned)(id-8);
 if(wins)*wins=(level+1u)*2u;
 if(dollars)*dollars=10u+level*2u;
 if(favors)*favors=level/2u+1u;
 return 1;
}
int game_unlock(Game *s,int id){
 unsigned cost,favors,wins;
 if(!game_unlock_requirements(id,&wins,&cost,&favors)||
    game_unlocked(s,id))return 0;
 if(s->wins<wins||s->dollars<cost||s->favors<favors)return 0;
 s->dollars-=cost;s->favors-=favors;
 s->unlocked|=(uint16_t)(1u<<id);
 return 1;
}
/* Ending IDs 1..4 and secret 9 are bad, IDs 5..8 are good. The existing 16-bit
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
 /* The next day brings ordinary bodily needs even if the player only
  * presses END DAY. This turns the days counter into an actual life cycle,
  * rather than an exploit to skip to late endings without ever caring.
  * Post-ending recovery/rewards happen FIRST; attrition applies to every
  * newly started day, including one following a bad ending. */
 s->hunger=clamp((int)s->hunger+8);
 s->energy=clamp((int)s->energy-6);
 s->mood=clamp((int)s->mood-2);
 s->chaos=clamp((int)s->chaos+2);
 /* A pathological multi-billion-day save must never wrap to day zero:
  * zero is rejected by the same existing save integrity checker. */
 if(s->days<UINT32_MAX)s->days++;
 return e;
}
int game_secret_egg(Game *s){
 if(!s)return 0;
 if(s->ending_mask&(1u<<END_EGG))return 0;
 s->ending_mask|=(uint16_t)(1u<<END_EGG);
 if(s->bad_endings<65535u)s->bad_endings++;
 s->corruption=clamp(s->corruption+20);
 return 1;
}
int game_reset_unlocked(const Game *s){
 /* Count only distinct BAD endings. Good endings must not unlock a wipe. */
 uint16_t x=(uint16_t)(s->ending_mask&0x021eu);int unique=0;
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
 case END_EGG:return "THE INCUBATION WAS NEVER HUMAN";
 default:return "A NORMAL DAY. SUSPICIOUS.";
 }
}
/* The first eight cabinets have individual rules and artwork.
 * Remaining cabinets retain the six-round direction fallback.
 * Arcade state is intentionally not part of the persistent Game save.
 */
static const uint8_t diaper_tasks[4]={3,1,2,0}; /* left,right,down,up */
static void diaper_next(Game *s,Arcade *a){
 a->target=diaper_tasks[a->stage];
 a->progress=0;a->taped=0;
 a->reverse=(uint8_t)(game_glitch_tier(s)>=2&&a->stage==2);
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?240:360);
}
static void teether_next(Game *s,Arcade *a){
 /* File the refund (LEFT) or shred the bogus bill (RIGHT). */
 a->target=(uint8_t)((next(s)&1u)?1:3);
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?180:240);
 a->touch_active=0;
}
static void kart_next(Game *s,Arcade *a){
 /* The roadblock chooses a lane; steering must happen before it arrives. */
 a->target=(uint8_t)(next(s)%3u);
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?66:90);
 a->hold_frames=a->timer;
}
static void sock_next(Game *s,Arcade *a){
 int i;
 /* Find the one sock matching the sample; shuffle all three shop stalls. */
 a->target=(uint8_t)(next(s)%3u);
 for(i=0;i<3;i++)a->notes[i]=(uint8_t)i;
 for(i=2;i>0;i--){
  int j=(int)(next(s)%(unsigned)(i+1));
  uint8_t t=a->notes[i];a->notes[i]=a->notes[j];a->notes[j]=t;
 }
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?150:210);
}
static void nap_begin(Game *s,Arcade *a){
 /* The green lullaby window is broad enough for touchscreen response. */
 a->elapsed=0;a->taped=0;
 a->hold_frames=(uint16_t)(game_glitch_tier(s)>=3?80:100);
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?420:600);
}
static void beatbox_next(Game *s,Arcade *a){
 int i;
 a->stage=0;
 a->phase=0;  /* Watch three notes before repeating the pattern. */
 a->elapsed=0;
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?72:90);
 a->hold_frames=a->timer; /* Fix animated preview: never divide by zero. */
 for(i=0;i<3;i++)a->notes[i]=(uint8_t)(next(s)&3u);
}
static void rattle_next(Game *s,Arcade *a){
 a->target=(uint8_t)(next(s)%3u);
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?108:144);
 a->hold_frames=a->timer; /* original fall duration for drawing */
 a->elapsed=0;
}
static void crawl_next(Game *s,Arcade *a){
 /* A rolling office scanner needs a duck; a floor crate needs a jump. */
 a->target=(uint8_t)((next(s)&1u)?0:2);
 a->stage=1;
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?58:82);
 a->hold_frames=a->timer;
}
static void stroller_begin(Game *s,Arcade *a){
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?510:660);
 a->balance=0;a->elapsed=0;a->hold_frames=0;a->steering=0;
}
static void cereal_next(Game *s,Arcade *a){
 int i,answer;
 a->stage=(uint8_t)(1u+next(s)%4u);
 a->direction=(uint8_t)(1u+next(s)%4u);
 answer=a->stage+a->direction;
 a->target=(uint8_t)(next(s)%3u);
 for(i=0;i<3;i++)a->notes[i]=(uint8_t)(answer+i-a->target);
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?120:180);
}
static void blocks_next(Game *s,Arcade *a){
 a->target=(uint8_t)(next(s)%3u);
 a->elapsed=0;a->direction=0;
 a->hold_frames=(uint16_t)(game_glitch_tier(s)>=3?11:16);
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?130:190);
}
static void audit_next(Game *s,Arcade *a){
 a->stage=(uint8_t)(20u+next(s)%50u);
 a->direction=(uint8_t)(a->stage+((next(s)&1u)?0u:5u));
 a->target=(uint8_t)(a->stage==a->direction?0:1);
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?125:180);
}
static void garden_next(Game *s,Arcade *a){
 a->target=(uint8_t)(next(s)%3u);
 a->progress=0;
}
static void court_next(Game *s,Arcade *a){
 int i;
 a->stage=0;a->touch_active=0;
 for(i=0;i<3;i++)a->notes[i]=(uint8_t)i;
 for(i=2;i>0;i--){
  int j=(int)(next(s)%(unsigned)(i+1));
  uint8_t t=a->notes[i];a->notes[i]=a->notes[j];a->notes[j]=t;
 }
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?180:270);
}
static void ceo_begin(Game *s,Arcade *a){
 a->balance=55;a->progress=45;
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?390:540);
}
static void finish(Game *s,Arcade *a,int won);
static void garden_water(Game *s,Arcade *a,unsigned amount){
 int count;
 if(!a->active||a->id!=13)return;
 count=a->progress+(int)amount;
 if(count>=100){
  a->round++;
  if(a->round>=4)finish(s,a,1);
  else garden_next(s,a);
 }else a->progress=(uint8_t)count;
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
 else if(id==2)teether_next(s,a);
 else if(id==3){a->stage=1;kart_next(s,a);}
 else if(id==4)beatbox_next(s,a);
 else if(id==5)sock_next(s,a);
 else if(id==6)nap_begin(s,a);
 else if(id==7)rattle_next(s,a);
 else if(id==8)crawl_next(s,a);
 else if(id==9)stroller_begin(s,a);
 else if(id==10)cereal_next(s,a);
 else if(id==11)blocks_next(s,a);
 else if(id==12)audit_next(s,a);
 else if(id==13){
  a->timer=(uint16_t)(game_glitch_tier(s)>=3?390:540);
  garden_next(s,a);
 }
 else if(id==14)court_next(s,a);
 else if(id==15)ceo_begin(s,a);
 else round_next(s,a);
}
int game_arcade_expected(const Arcade *a){
 if(a->id==1)return a->target^(a->reverse?2:0);
 if(a->id==7||a->id==2)return a->target;
 if(a->id==4)return a->notes[a->stage<3?a->stage:2];
 if(a->id==3)return (a->target+1u)%3u; /* a safe lane */
 if(a->id==5){int i;for(i=0;i<3;i++)if(a->notes[i]==a->target)return i;}
 if(a->id==6||a->id==9||a->id==11)return 0;
 if(a->id==8)return a->target;
 if(a->id==10||a->id==12||a->id==13)return a->target;
 if(a->id==14)return a->notes[a->stage<3?a->stage:2];
 if(a->id==15)return a->balance<a->progress?0:1;
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
 if(!a->active||(a->id!=0&&a->id!=9))return;
 force=direction<0?-4:direction>0?4:0;
 a->steering=(int8_t)(a->reverse?-force:force);
}
void game_arcade_input(Game *s,Arcade *a,int dir){
 if(!a->active||a->id==0||a->id==9||dir<0||dir>3)return;
 if(a->id==8){
  if(dir==0||dir==2)a->stage=(uint8_t)dir;
  return; /* Choose a posture before the office obstacle arrives. */
 }
 if(a->id==10){
  if(dir>2)return;
  if(dir==a->target){
   a->round++;
   if(a->round>=MBAI_ROUNDS)finish(s,a,1);
   else cereal_next(s,a);
  }else{
   a->mistakes++;s->corruption=clamp(s->corruption+3);
   if(a->mistakes>=3)finish(s,a,0);
   else cereal_next(s,a);
  }
  return;
 }
 if(a->id==12){
  if(dir>1)return;
  if(dir==a->target){
   a->round++;
   if(a->round>=MBAI_ROUNDS)finish(s,a,1);
   else audit_next(s,a);
  }else{
   a->mistakes++;s->corruption=clamp(s->corruption+3);
   if(a->mistakes>=3)finish(s,a,0);
   else audit_next(s,a);
  }
  return;
 }
 if(a->id==13){
  if(dir>2)return;
  if(dir==a->target)garden_water(s,a,25);
  else{
   a->mistakes++;s->corruption=clamp(s->corruption+3);
   if(a->mistakes>=3)finish(s,a,0);
  }
  return;
 }
 if(a->id==14){
  if(dir>2)return;
  if(dir==a->notes[a->stage]){
   a->stage++;
   if(a->stage>=3){
    a->round++;
    if(a->round>=4)finish(s,a,1);
    else court_next(s,a);
   }
  }else{
   a->mistakes++;s->corruption=clamp(s->corruption+3);
   if(a->mistakes>=3)finish(s,a,0);
   else court_next(s,a);
  }
  return;
 }
 if(a->id==15){
  int budget=a->balance,morale=a->progress;
  if(dir>2)return;
  if(dir==0){budget+=12;morale-=8;}
  else if(dir==1){budget-=10;morale+=13;}
  else{budget-=5;morale+=4;}
  if(budget<20||budget>85||morale<20||morale>85){
   a->mistakes++;s->corruption=clamp(s->corruption+3);
   budget=55;morale=45;
   if(a->mistakes>=3){finish(s,a,0);return;}
  }
  a->balance=(int16_t)budget;a->progress=(uint8_t)morale;
  a->round++;
  if(a->round>=MBAI_ROUNDS)
   finish(s,a,budget>=35&&morale>=35&&budget<=80&&morale<=80);
  return;
 }
 if(a->id==11){
  if(dir!=0)return; /* Drop the active block at its current position. */
  if(a->direction==a->target){
   a->round++;
   if(a->round>=MBAI_ROUNDS)finish(s,a,1);
   else blocks_next(s,a);
  }else{
   a->mistakes++;s->corruption=clamp(s->corruption+3);
   if(a->mistakes>=3)finish(s,a,0);
   else blocks_next(s,a);
  }
  return;
 }
 if(a->id==3){
  if(dir<=2)a->stage=(uint8_t)dir;
  return; /* Steering is not itself a hit or a score. */
 }
 if(a->id==5){
  if(dir>2)return;
  if(a->notes[dir]==a->target){
   a->round++;
   if(a->round>=MBAI_ROUNDS)finish(s,a,1);
   else sock_next(s,a);
  }else{
   a->mistakes++;s->corruption=clamp(s->corruption+3);
   if(a->mistakes>=3)finish(s,a,0);
   else sock_next(s,a);
  }
  return;
 }
 if(a->id==6){
  unsigned phase;
  if(dir!=0||a->taped)return;
  phase=a->elapsed%a->hold_frames;
  if(phase>=a->hold_frames*38u/100u&&
     phase<=a->hold_frames*70u/100u){
   a->round++;
   a->taped=1; /* One success per lullaby cycle. */
   if(a->round>=4)finish(s,a,1);
  }else{
   a->mistakes++;s->corruption=clamp(s->corruption+3);
   if(a->mistakes>=3)finish(s,a,0);
  }
  return;
 }
 if(a->id==4){
  int correct;
  if(!a->phase)return; /* Sounds/visuals are demonstrating the tune. */
  correct=dir==a->notes[a->stage];
  if(correct){
   a->stage++;
   if(a->stage>=3){
    a->round++;
    if(a->round>=3)finish(s,a,1);
    else beatbox_next(s,a);
   }
  }else{
   a->mistakes++;s->corruption=clamp(s->corruption+3);
   if(a->mistakes>=3)finish(s,a,0);
   else beatbox_next(s,a);
  }
  return;
 }
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
   else if(a->id==2)teether_next(s,a);
   else round_next(s,a);
  }
 }else{
  a->mistakes++;s->corruption=clamp(s->corruption+4);
  if(a->mistakes>=3)finish(s,a,0);
  else if(a->id==1)diaper_next(s,a);
  else if(a->id==2)teether_next(s,a);
  else if(a->id==5)sock_next(s,a);
  else if(a->id==7)rattle_next(s,a);
  else if(a->id==10)cereal_next(s,a);
  else if(a->id==12)audit_next(s,a);
  else if(a->id==14)court_next(s,a);
  else round_next(s,a);
 }
}
static int inside(int x,int y,int x0,int y0,int x1,int y1){
 return x>=x0&&x<x1&&y>=y0&&y<y1;
}
/* Gesture-based receipt sorting: source -> matching destination, on release.
 * Holding the paper while dragging updates its position in the illustrated
 * UI, but releasing outside a tray never causes an undeserved mistake. */
int game_arcade_teether_touch(Game *s,Arcade *a,int touching,int x,int y){
 int dir=-1;
 if(!a->active||a->id!=2)return 0;
 if(!touching){
  if(a->touch_active&&inside(a->touch_start_x,a->touch_start_y,99,91,160,150)){
   if(inside(a->touch_last_x,a->touch_last_y,11,128,96,174))dir=3;
   else if(inside(a->touch_last_x,a->touch_last_y,159,128,245,174))dir=1;
  }
  a->touch_active=0;
  if(dir>=0){game_arcade_input(s,a,dir);return 1;}
  return 0;
 }
 if(x<0||x>=256||y<27||y>=176)return 0;
 if(!a->touch_active){
  a->touch_active=1;
  a->touch_start_x=a->touch_last_x=(int16_t)x;
  a->touch_start_y=a->touch_last_y=(int16_t)y;
  return 1;
 }
 a->touch_last_x=(int16_t)x;
 a->touch_last_y=(int16_t)y;
 return 0;
}
/* Continuous irrigation of the correct bed: only a held stylus adds water.
 * Releasing cannot accidentally water an unselected garden. */
int game_arcade_garden_touch(Game *s,Arcade *a,int touching,int x,int y){
 int lane=-1;
 if(!a->active||a->id!=13||!touching)return 0;
 if(y<105||y>=175)return 0;
 if(x>=13&&x<79)lane=0;
 else if(x>=95&&x<161)lane=1;
 else if(x>=177&&x<243)lane=2;
 if(lane==a->target){
  garden_water(s,a,2);
  /* Repaint at 8%-progress intervals, not each held-stylus frame.
   * Watering simulation still advances every frame. */
  return !a->active||(a->progress%8u)==0u;
 }
 return 0;
}
/* Courtroom evidence must be connected along a continuous gesture.
 * Button accessibility can still select individual numbered clues. */
int game_arcade_court_touch(Game *s,Arcade *a,int touching,int x,int y){
 static const int node_x[3]={46,128,210};
 static const int node_y[3]={134,92,134};
 int i;
 if(!a->active||a->id!=14)return 0;
 if(!touching){
  int changed=a->stage!=0;
  if(a->touch_active&&a->stage>0)a->stage=0;
  a->touch_active=0;
  return changed;
 }
 for(i=0;i<3;i++){
  int dx=x-node_x[i],dy=y-node_y[i];
  if(dx*dx+dy*dy<=20*20&&i==a->notes[a->stage]){
   a->touch_active=1;
   game_arcade_input(s,a,i);
   return 1;
  }
 }
 return 0;
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
 if(a->balance>=-MBAI_BOTTLE_SAFE_BALANCE&&
    a->balance<=MBAI_BOTTLE_SAFE_BALANCE){
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
 if(a->id==8){
  if(a->timer)a->timer--;
  if(!a->timer){
   if(a->stage==a->target){
    a->round++;
    if(a->round>=MBAI_ROUNDS){finish(s,a,1);return;}
   }else{
    a->mistakes++;s->corruption=clamp(s->corruption+4);
    if(a->mistakes>=3){finish(s,a,0);return;}
   }
   crawl_next(s,a);
  }
  return;
 }
 if(a->id==9){
  int drift;
  a->elapsed++;
  drift=((a->elapsed/38u)&1u)?-2:2;
  a->balance+=(int16_t)(drift+a->steering);
  a->steering=0;
  if(a->balance>60||a->balance< -60){
   a->mistakes++;s->corruption=clamp(s->corruption+3);
   a->balance=0;a->hold_frames=0;
   if(a->mistakes>=3){finish(s,a,0);return;}
  }
  if(a->balance>=-25&&a->balance<=25){
   a->hold_frames++;
   if(a->hold_frames>=70){
    a->hold_frames=0;a->round++;
    if(a->round>=5){finish(s,a,1);return;}
   }
  }else a->hold_frames=0;
  if(a->timer)a->timer--;
  if(!a->timer)finish(s,a,0);
  return;
 }
 if(a->id==13||a->id==15){
  if(a->timer)a->timer--;
  if(!a->timer)finish(s,a,0);
  return;
 }
 if(a->id==11){
  unsigned i;
  a->elapsed++;
  i=(unsigned)(a->elapsed/a->hold_frames)%4u;
  a->direction=(uint8_t)(i==3u?1u:i);
  if(a->timer)a->timer--;
  if(!a->timer){
   a->mistakes++;s->corruption=clamp(s->corruption+3);
   if(a->mistakes>=3)finish(s,a,0);
   else blocks_next(s,a);
  }
  return;
 }
 if(a->id==3){
  if(a->timer)a->timer--;
  if(!a->timer){
   if(a->stage==a->target){
    a->mistakes++;s->corruption=clamp(s->corruption+4);
    if(a->mistakes>=3){finish(s,a,0);return;}
   }else{
    a->round++;
    if(a->round>=MBAI_ROUNDS){finish(s,a,1);return;}
   }
   kart_next(s,a);
  }
  return;
 }
 if(a->id==6){
  unsigned phase;
  a->elapsed++;
  phase=a->elapsed%a->hold_frames;
  if(phase<a->hold_frames*38u/100u||
     phase>a->hold_frames*70u/100u)a->taped=0;
  if(a->timer)a->timer--;
  if(!a->timer)finish(s,a,0);
  return;
 }
 if(a->id==4){
  if(!a->phase)a->elapsed++;
  if(a->timer)a->timer--;
  if(!a->timer){
   if(!a->phase){
    a->phase=1;a->timer=(uint16_t)(game_glitch_tier(s)>=3?180:240);
   }else{
    a->mistakes++;s->corruption=clamp(s->corruption+3);
    if(a->mistakes>=3)finish(s,a,0);
    else beatbox_next(s,a);
   }
  }
  return;
 }
 if(a->timer)a->timer--;
 if(!a->timer){
  a->mistakes++;s->corruption=clamp(s->corruption+3);
  if(a->mistakes>=3)finish(s,a,0);
  else if(a->id==1)diaper_next(s,a);
  else if(a->id==2)teether_next(s,a);
  else if(a->id==5)sock_next(s,a);
  else if(a->id==7)rattle_next(s,a);
  /* Rebuild the whole cabinet-specific question after a timeout. The
   * old direction fallback changed visible numbers but kept stale
   * answers, and failed to clear an unfinished courtroom gesture. */
  else if(a->id==10)cereal_next(s,a);
  else if(a->id==12)audit_next(s,a);
  else if(a->id==14)court_next(s,a);
  else round_next(s,a);
 }
}
