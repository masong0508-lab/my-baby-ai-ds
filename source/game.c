#include "game.h"
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
 if(s->corruption>=80)return "MY THOUGHTS HAVE TABS NOW.";
 if(s->hunger>75)return "MY BOTTLE HAS A SUBSCRIPTION.";
 if(s->chaos>65)return "THE BOARD IS NOT PLEASED.";
 switch(action){
 case FEED:return "CAN I EXPENSE THIS BOTTLE?";
 case CHAT:return "TELL HR I'M TEETHING.";
 case PLAY:return "I OWN 3% OF THIS RATTLE.";
 case NAP:return "WAKE ME WHEN STOCKS RECOVER.";
 default:return "BEEP.";
 }
}
int game_unlocked(const Game *s,int id){return id>=0&&id<MBAI_GAMES&&!!(s->unlocked&(1u<<id));}
int game_unlock(Game *s,int id){
 int level=id-8;unsigned cost,favors;
 if(level<0||id>=MBAI_GAMES||game_unlocked(s,id))return 0;
 cost=10u+level*2u;favors=(unsigned)(level/2)+1u;
 if(s->wins<(unsigned)((level+1)*2)||s->dollars<cost||s->favors<favors)return 0;
 s->dollars-=cost;s->favors-=favors;s->unlocked|=(uint16_t)(1u<<id);return 1;
}
int game_end_day(Game *s){
 int e=END_OK;
 if(s->corruption>=80&&s->chaos>=45)e=END_REBOOT;
 else if(s->hunger>=78)e=END_BOTTLE;
 else if(s->mood<=24)e=END_CEO;
 else if(s->chaos>=72)e=END_INTERN;
 if(e){
  if(s->bad_endings!=65535)s->bad_endings++;
  s->ending_mask|=(uint16_t)(1u<<e);
  s->corruption=clamp(s->corruption+15);
  s->hunger=clamp(s->hunger-35);s->energy=clamp(s->energy+30);
  s->chaos=clamp(s->chaos-35);s->mood=clamp(s->mood+30);
 }
 s->days++;return e;
}
int game_reset_unlocked(const Game *s){
 uint16_t x=s->ending_mask;int unique=0;
 while(x){unique+=(x&1u);x>>=1;}
 return s->bad_endings>=5||unique>=3;
}
const char *game_ending_name(int e){
 switch(e){
 case END_BOTTLE:return "BOTTLE SUBSCRIPTION ENDED";
 case END_CEO:return "BABY CEO HAS QUIT";
 case END_REBOOT:return "MANDATORY DAYCARE REBOOT";
 case END_INTERN:return "UNPAID INTERN REVOLT";
 default:return "A NORMAL DAY. SUSPICIOUS.";
 }
}
static void round_next(Game *s,Arcade *a){
 a->direction=(uint8_t)(next(s)&3u);
 a->reverse=(uint8_t)(game_glitch_tier(s)>=2&&a->round%3==2);
 a->timer=(uint16_t)(game_glitch_tier(s)>=3?75:120);
}
void game_arcade_start(Game *s,Arcade *a,int id){
 memset(a,0,sizeof(*a));if(!game_unlocked(s,id))return;
 a->active=1;a->id=(uint8_t)id;round_next(s,a);
}
int game_arcade_expected(const Arcade *a){return a->direction^(a->reverse?2:0);}
static void finish(Game *s,Arcade *a,int won){
 a->active=0;a->finished=(uint8_t)(won?1:2);
 if(won){s->wins++;s->dollars=(uint16_t)(s->dollars<=65530?s->dollars+5:65535);if(!(s->wins&1u)&&s->favors<65535)s->favors++;s->mood=clamp(s->mood+4);}
 else {s->losses++;s->mood=clamp(s->mood-8);s->chaos=clamp(s->chaos+15);s->corruption=clamp(s->corruption+18);}
}
void game_arcade_input(Game *s,Arcade *a,int dir){
 if(!a->active||dir<0||dir>3)return;
 if(dir==game_arcade_expected(a)){a->round++;if(a->round>=MBAI_ROUNDS)finish(s,a,1);else round_next(s,a);}
 else {a->mistakes++;s->corruption=clamp(s->corruption+4);if(a->mistakes>=3)finish(s,a,0);else round_next(s,a);}
}
void game_arcade_tick(Game *s,Arcade *a){
 if(!a->active)return;
 if(a->timer)a->timer--;
 if(!a->timer){a->mistakes++;s->corruption=clamp(s->corruption+3);if(a->mistakes>=3)finish(s,a,0);else round_next(s,a);}
}
