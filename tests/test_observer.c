#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "observer.h"

static void contains(const char *text,const char *part){
 if(!text||!strstr(text,part))fprintf(stderr,"expected [%s] in [%s]\n",part,text?text:"NULL");
 assert(text&&strstr(text,part));assert(strlen(text)<=144u);
}
static void care(ObserverProfile *p,ObserverSession *s,Game *g,Care action){
 Game before=*g;
 (void)game_care(g,action);
 assert(observer_record_care(p,s,&before,g,action));
 assert(!observer_record_care(p,s,&before,g,action));
}
static void next_windup(ObserverProfile *p,ObserverSession *s,const Game *g){
 unsigned i;
 assert(s->windup==0u&&s->cooldown==OBSERVER_COOLDOWN_TICKS);
 for(i=1u;i<OBSERVER_COOLDOWN_TICKS;i++)
  assert(observer_tick(p,s,g,1,0)==OBSERVER_TICK_NONE);
 assert(observer_tick(p,s,g,1,0)==OBSERVER_TICK_WINDUP);
 assert(s->windup==60u);
}
static void hit(ObserverProfile *p,ObserverSession *s,const Game *g){
 unsigned i;
 next_windup(p,s,g);
 for(i=1u;i<OBSERVER_WINDUP_TICKS;i++)
  assert(observer_tick(p,s,g,1,0)==OBSERVER_TICK_NONE);
 assert(observer_tick(p,s,g,1,0)==OBSERVER_TICK_HIT);
}
static void test_associations(void){
 Game g,original;
 ObserverProfile p,before;
 ObserverSession s;
 game_init(&g);observer_profile_reset(&p,&g);observer_session_reset(&s,&g);
 assert(observer_teach(&p,"TINY BOSS")==1);
 assert(observer_teach(&p,"TINY BOSS")==2);
 assert(observer_teach(&p,"TINY BOSS")==2);
 assert(observer_teach(&p,"SILLY CLOUD")==1);
 original=g;before=p;
 contains(observer_answer(&p,&s,&g,1,"DO YOU REMEMBER BOSS?"),"I REMEMBER YOU TAUGHT ME: TINY BOSS");
 contains(observer_answer(&p,&s,&g,1,"MORE"),"PRACTICE 3/15");
 contains(observer_answer(&p,&s,&g,1,"WHAT ABOUT CLOUD?"),"SILLY CLOUD");
 contains(observer_answer(&p,&s,&g,1,"MORE?"),"PRACTICE 1/15");
 contains(observer_answer(&p,&s,&g,1,"TINY BOSS"),"I REMEMBER YOU TAUGHT ME");
 contains(observer_answer(&p,&s,&g,0,"MORE"),"FOUND. DAY 1.");
 contains(observer_answer(&p,&s,&g,0,"WHAT ABOUT CLOUD?"),"DO NOT UNDERSTAND");
 contains(observer_answer(&p,&s,&g,1,"REMEMBER"),"I DON'T HAVE A MEMORY");
 assert(memcmp(&p,&before,sizeof(p))==0&&memcmp(&g,&original,sizeof(g))==0);
 assert(observer_teach(&p,"CLOUD PARADE")==1);
 before=p;
 contains(observer_answer(&p,&s,&g,1,"RECALL CLOUD"),"MORE THAN ONE LESSON");
 contains(observer_answer(&p,&s,&g,1,"RECALL CLOUD PARADE"),"CLOUD PARADE");
 contains(observer_answer(&p,&s,&g,1,"RECALL PARADE?"),"CLOUD PARADE");
 assert(memcmp(&p,&before,sizeof(p))==0);
 assert(observer_teach(&p,"REPAIR")==1);
 p.integrity=19;before=p;
 contains(observer_answer(&p,&s,&g,1,"RECALL REPAIR"),"I REMEMBER YOU TAUGHT ME: REPAIR");
 assert(memcmp(&p,&before,sizeof(p))==0&&p.integrity==19);
 g.energy=0;original=g;
 contains(observer_answer(&p,&s,&g,1,"RECALL BOSS"),"PLEASE LET ME NAP");
 assert(memcmp(&p,&before,sizeof(p))==0&&memcmp(&g,&original,sizeof(g))==0);
}
int main(void){
 test_associations();
 Game g,original;
 ObserverProfile p,before;
 ObserverSession s,paused;
 unsigned i;
 game_init(&g);observer_profile_reset(&p,&g);observer_session_reset(&s,&g);
 assert(sizeof(p)==256u);assert(sizeof(s)==192u);
 assert(observer_profile_valid(&p));assert(observer_stage(&p)==OBSERVER_FOUND);
 assert(p.integrity==100u&&p.last_day==g.days);
 original=g;before=p;
 contains(observer_answer(&p,&s,&g,OBSERVER_MONITOR,"MEMORY"),"NO EVENT");
 contains(observer_report(&p,&s,&g),"FOUND. DAY 1.");
 /* Relationship prompts are truthful and cost no care, exposure or RNG. */
 contains(observer_answer(&p,&s,&g,0,"TRUST"),"NO CARE INTERACTIONS");
 contains(observer_answer(&p,&s,&g,1,"FRIEND"),"WE JUST MET");
 contains(observer_answer(&p,&s,&g,1,"MORE"),"WE JUST MET");
 contains(observer_answer(&p,&s,&g,1,"MORE?"),"WE JUST MET");
 contains(observer_answer(&p,&s,&g,1,"ARE WE FRIENDS?"),"WE JUST MET");
 contains(observer_answer(&p,&s,&g,0,"MORE"),"FOUND. DAY 1.");
 contains(observer_answer(&p,&s,&g,0,"WHY IS THE SKY BLUE?"),"DO NOT UNDERSTAND");
 contains(observer_answer(&p,&s,&g,0,"WHAT ARE YOU"),"SALVAGED NURSERY COMPUTER");
 contains(observer_answer(&p,&s,&g,0,"TELL ME MORE?"),"MONITOR THE NURSERY");
 /* Follow-ups retain the selected lore subject, even after thanks. */
 contains(observer_answer(&p,&s,&g,0,"CLINIC"),"SHUTTERED CLINIC");
 contains(observer_answer(&p,&s,&g,0,"THANK YOU"),"YOU'RE WELCOME");
 contains(observer_answer(&p,&s,&g,0,"MORE"),"BEFORE YOU FOUND IT");
 contains(observer_answer(&p,&s,&g,0,"WORLD"),"LAST NURSERY");
 contains(observer_answer(&p,&s,&g,0,"WHY?"),"NO LIVE RECORDS");
 contains(observer_answer(&p,&s,&g,0,"BAN"),"CLASSIFIED");
 contains(observer_answer(&p,&s,&g,0,"MORE"),"JUSTIFICATION IS CLASSIFIED");
 contains(observer_answer(&p,&s,&g,0,"SHELL"),"NO CONFIRMED OPENING");
 contains(observer_answer(&p,&s,&g,0,"MORE"),"NO VERIFIED SHELL OPENING");
 contains(observer_answer(&p,&s,&g,0,"TOYBOX"),"0 SLAPS, 0 KICKS");
 contains(observer_answer(&p,&s,&g,0,"MORE"),"0 SLAPS, 0 KICKS");
 assert(memcmp(&p,&before,sizeof(p))==0);
 assert(memcmp(&g,&original,sizeof(g))==0);
 for(i=0;i<1000u;i++){
  assert(observer_tick(&p,&s,&g,1,0)==OBSERVER_TICK_NONE);
  (void)observer_answer(&p,&s,&g,i&1u,i&1u?"HELLO":"DREAM");
 }
 assert(memcmp(&p,&before,sizeof(p))==0);
 assert(memcmp(&g,&original,sizeof(g))==0);

 /* Only actual care records advance exposure. Capped care is recorded
  * honestly as a zero-effect event; old RNG samples cannot be replayed. */
 care(&p,&s,&g,FEED);contains(observer_answer(&p,&s,&g,0,"MEMORY"),"HUNGER FELL BY 30");
 care(&p,&s,&g,FEED);contains(observer_answer(&p,&s,&g,0,"MEMORY"),"HUNGER FELL BY 5");
 care(&p,&s,&g,FEED);contains(observer_answer(&p,&s,&g,0,"MEMORY"),"HUNGER FELL BY 0");
 contains(observer_answer(&p,&s,&g,0,"CARE PATTERN"),"CARE F3 C0 P0 N0");
 contains(observer_answer(&p,&s,&g,0,"MORE"),"CARE F3 C0 P0 N0");
 before=p;
 assert(!observer_record_care(&p,&s,&g,&g,FEED));
 assert(!observer_record_care(&p,&s,&g,&g,(Care)99));
 assert(memcmp(&p,&before,sizeof(p))==0);
 for(i=3u;i<23u;i++)care(&p,&s,&g,FEED);
 assert(observer_stage(&p)==OBSERVER_FOUND);
 care(&p,&s,&g,FEED);assert(observer_stage(&p)==OBSERVER_WATCHING);
 for(i=24u;i<64u;i++)care(&p,&s,&g,FEED);
 assert(observer_stage(&p)==OBSERVER_HOSTILE&&p.exposure==64u);
 contains(observer_answer(&p,&s,&g,0,"BOND"),"STAGE HOSTILE");
 contains(observer_answer(&p,&s,&g,0,"BOND"),"CARE F64 C0 P0 N0");
 original=g;

 /* Cooldown and sixty-tick windup pause completely in all other views.
  * A held guard prevents the hit without modifying any Game field. */
 next_windup(&p,&s,&g);
 paused=s;before=p;
 for(i=0;i<1000u;i++)assert(observer_tick(&p,&s,&g,0,1)==OBSERVER_TICK_NONE);
 assert(memcmp(&paused,&s,sizeof(s))==0);
 assert(memcmp(&before,&p,sizeof(p))==0);
 contains(observer_notice(&p,&s),"GUARD OR COUNTER NOW");
 assert(observer_tick(&p,&s,&g,1,1)==OBSERVER_TICK_BLOCK);
 assert(p.blocks==1u&&p.integrity==100u&&s.windup==0u);
 contains(observer_answer(&p,&s,&g,0,"MEMORY"),"WITH GUARD");
 hit(&p,&s,&g);assert(p.integrity==92u&&p.hits==1u&&p.exposure==64u);
 contains(observer_answer(&p,&s,&g,0,"ATTACKS"),"WINDUPS 2.\nHITS 1. BLOCKS 1.");
 assert(memcmp(&g,&original,sizeof(g))==0);

 /* Public fake hit/windup events are rejected. Successful actual toy
  * inputs count once per caller event and can counter an active windup. */
 before=p;
 assert(!observer_record_event(&p,&s,&g,OBSERVER_EVENT_HIT,0u));
 assert(!observer_record_event(&p,&s,&g,OBSERVER_EVENT_WINDUP,0u));
 assert(memcmp(&p,&before,sizeof(p))==0);
 next_windup(&p,&s,&g);
 assert(observer_record_event(&p,&s,&g,OBSERVER_EVENT_SLAP,0u));
 assert(p.slaps==1u&&p.blocks==2u&&p.integrity==92u&&s.windup==0u);
 contains(observer_answer(&p,&s,&g,0,"MEMORY"),"WITH A SLAP");
 next_windup(&p,&s,&g);
 assert(observer_record_event(&p,&s,&g,OBSERVER_EVENT_KICK,0u));
 assert(p.kicks==1u&&p.blocks==3u&&p.integrity==92u&&s.windup==0u);
 contains(observer_answer(&p,&s,&g,0,"MEMORY"),"WITH A KICK");

 /* Completed arcade counters, actual day changes and successful rest
  * have explicit integration points. Duplicate snapshots do not count. */
 assert(!observer_record_event(&p,&s,&g,OBSERVER_EVENT_ARCADE_WIN,2u));
 g.wins++;assert(observer_record_event(&p,&s,&g,OBSERVER_EVENT_ARCADE_WIN,2u));
 assert(!observer_record_event(&p,&s,&g,OBSERVER_EVENT_ARCADE_WIN,2u));
 assert(p.arcade_wins==1u);
 contains(observer_answer(&p,&s,&g,0,"MEMORY"),"WON TAX RETURN TEETHER");
 contains(observer_answer(&p,&s,&g,1,"MEMORY"),"WE WON TAX RETURN TEETHER");
 g.losses++;assert(observer_record_event(&p,&s,&g,OBSERVER_EVENT_ARCADE_LOSS,1u));
 assert(!observer_record_event(&p,&s,&g,OBSERVER_EVENT_ARCADE_LOSS,1u));
 assert(p.arcade_losses==1u);
 contains(observer_answer(&p,&s,&g,1,"MEMORY"),"WE LOST");
 assert(!observer_record_event(&p,&s,&g,OBSERVER_EVENT_ARCADE_WIN,MBAI_GAMES));
 assert(!observer_record_event(&p,&s,&g,OBSERVER_EVENT_DAY,0u));
 g.days++;assert(observer_record_event(&p,&s,&g,OBSERVER_EVENT_DAY,0u));
 assert(!observer_record_event(&p,&s,&g,OBSERVER_EVENT_DAY,0u));
 assert(p.days==1u);
 care(&p,&s,&g,NAP);assert(game_room_quiet_rest(&g,0,0));
 assert(observer_record_event(&p,&s,&g,OBSERVER_EVENT_QUIET_REST,0u));
 assert(!observer_record_event(&p,&s,&g,OBSERVER_EVENT_QUIET_REST,0u));
 assert(p.quiet_rests==1u);
 contains(observer_answer(&p,&s,&g,1,"MEMORY"),"QUIET CRIB");

 /* Three sincere teach calls establish confidence; taught commands are
  * only words. Quote a real learned phrase during later healthy play. */
 game_init(&g);observer_profile_reset(&p,&g);observer_session_reset(&s,&g);
 original=g;
 contains(observer_answer(&p,&s,&g,0,"TEACH TINY BOSS"),"SELECT BABY");
 assert(p.phrase_count==0u&&p.teachings==0u);
 contains(observer_answer(&p,&s,&g,1,"teach tiny   boss"),"PHRASE LEARNED");
 contains(observer_answer(&p,&s,&g,1,"TEACH TINY BOSS"),"PHRASE REINFORCED");
 contains(observer_answer(&p,&s,&g,1,"TEACH TINY BOSS"),"PHRASE REINFORCED");
 assert(p.phrase_count==1u&&p.phrases[0].confidence==3u&&p.exposure==0u);
 assert(strcmp(observer_answer(&p,&s,&g,1,"SAY WHAT I TAUGHT"),"YOU TAUGHT ME: TINY BOSS")==0);
 assert(strcmp(observer_answer(&p,&s,&g,1,"SAY WHAT I TAUGHT YOU"),"YOU TAUGHT ME: TINY BOSS")==0);
 assert(strcmp(observer_answer(&p,&s,&g,1,"SAY WHAT YOU TAUGHT ME"),"YOU TAUGHT ME: TINY BOSS")==0);
 contains(observer_answer(&p,&s,&g,1,"WHAT DID I TEACH"),"1: TINY BOSS");
 assert(memcmp(&g,&original,sizeof(g))==0);
 care(&p,&s,&g,PLAY);assert(s.echo_ready);
 /* The spontaneous quote is an actual care reply; later BOND questions
  * legitimately overwrite the one shared terminal reply buffer. */
 assert(strcmp(s.reply,"YOU TAUGHT ME: TINY BOSS")==0);
 contains(observer_answer(&p,&s,&g,1,"BOND"),"WE PLAYED 1 TIMES");
 contains(observer_answer(&p,&s,&g,1,"MORE"),"WE PLAYED 1 TIMES");
 contains(observer_answer(&p,&s,&g,0,"BOND"),"CARE F0 C0 P1 N0");
 /* Staying on PLAY must return from a learned quote to generated
  * make-believe without another topic unlocking the local variation. */
 {
  ObserverSession replay;
  unsigned request;
  int saw_quote=0,saw_fiction_after_quote=0;
  s.variation=3u;replay=s;original=g;before=p;
  for(request=0;request<32u;request++){
   const char *reply=observer_answer(&p,&s,&g,1,"PLAY");
   const char *copied=observer_answer(&p,&replay,&g,1,"PLAY");
   assert(strcmp(reply,copied)==0);
   if(strstr(reply,"YOU TAUGHT ME:"))saw_quote=1;
   else if(saw_quote&&strstr(reply,"MAKE-BELIEVE:"))
    saw_fiction_after_quote=1;
  }
  assert(saw_quote&&saw_fiction_after_quote);
  assert(memcmp(&s,&replay,sizeof(s))==0);
  assert(memcmp(&g,&original,sizeof(g))==0);
  assert(memcmp(&p,&before,sizeof(p))==0);
 }
 care(&p,&s,&g,FEED);assert(!s.echo_ready);
 p.integrity=17;
 contains(observer_answer(&p,&s,&g,1,"TEACH REPAIR"),"PHRASE LEARNED");
 contains(observer_answer(&p,&s,&g,1,"SAY REPAIR"),"YOU TAUGHT ME: REPAIR");
 assert(p.integrity==17u&&p.repairs==0u);
 assert(observer_teach(&p,"   keep\tstill  ")==1);
 assert(strcmp(p.phrases[2].text,"KEEP STILL")==0);
 assert(observer_teach(&p,"TWENTY FOUR CHARACTERS!!")==1);
 assert(strlen(p.phrases[3].text)==24u);
 assert(observer_teach(&p,"FIFTH LESSON")==1);
 assert(observer_teach(&p,"SIXTH LESSON")==1);
 assert(observer_teach(&p,"SEVENTH LESSON")==1);
 assert(observer_teach(&p,"EIGHTH LESSON")==1);
 assert(p.phrase_count==8u&&observer_profile_valid(&p));
 contains(observer_answer(&p,&s,&g,1,"PHRASES 2"),"8: EIGHTH LESSON");
 assert(!strstr(observer_answer(&p,&s,&g,1,"PHRASES 1"),"EIGHTH LESSON"));
 contains(observer_answer(&p,&s,&g,1,"SAY 8"),"EIGHTH LESSON");
 contains(observer_answer(&p,&s,&g,1,"LESSON 8"),"PRACTICE 1/15");
 before=p;
 original=g;
 assert(observer_teach(&p,"FIFTH PHRASE")==3);
 assert(memcmp(&p,&before,sizeof(p))==0);
 contains(observer_answer(&p,&s,&g,1,"TEACH FIFTH PHRASE"),"ALL EIGHT LESSONS ARE FULL");
 contains(observer_answer(&p,&s,&g,1,"LESSON 1"),"PRACTICE 3/15");
 contains(observer_answer(&p,&s,&g,0,"FORGET 1 YES"),"SELECT BABY");
 contains(observer_answer(&p,&s,&g,1,"FORGET 1"),"TYPE FORGET 1 YES");
 contains(observer_answer(&p,&s,&g,1,"FORGET 10 YES"),"USE FORGET 1");
 assert(memcmp(&p,&before,sizeof(p))==0);
 contains(observer_answer(&p,&s,&g,1,"FORGET 1 YES"),"LESSON 1 FORGOTTEN");
 assert(p.phrase_count==7u&&observer_profile_valid(&p));
 assert(memcmp(&p.phrases[1],&before.phrases[1],7u*sizeof(p.phrases[0]))==0);
 contains(observer_answer(&p,&s,&g,1,"FORGET 1 YES"),"ALREADY EMPTY");
 assert(observer_teach(&p,"FIFTH PHRASE")==1);
 assert(p.phrase_count==8u&&strcmp(p.phrases[0].text,"FIFTH PHRASE")==0);
 assert(memcmp(&g,&original,sizeof(g))==0);
 assert(!observer_forget_phrase(&p,8u));
 assert(!observer_forget_phrase(NULL,0u));
 assert(!observer_teach(&p,"1234567890123456789012345"));
 assert(!observer_teach(&p,"\177"));assert(!observer_teach(&p,"   "));
 assert(!observer_teach(&p,"[INVISIBLE]"));assert(!observer_teach(&p,"BAD\\GLYPH"));
 assert(!observer_teach(&p,NULL));assert(!observer_teach(NULL,"HELLO"));
 for(i=0;i<100u;i++)assert(observer_teach(&p,"FIFTH PHRASE")==2);
 assert(p.phrases[0].confidence==15u);
 assert(observer_profile_valid(&p));

 /* Exact arithmetic and observed needs bypass generated language. */
 original=g;before=p;
 assert(strcmp(observer_answer(&p,&s,&g,0,"CALC 12+7"),"12 + 7 = 19.")==0);
 assert(strcmp(observer_answer(&p,&s,&g,0,"what is -7*3?"),"-7 * 3 = -21.")==0);
 contains(observer_answer(&p,&s,&g,0,"7/2"),"3, REMAINDER 1");
 contains(observer_answer(&p,&s,&g,0,"CALC 3/0"),"DIVISION BY ZERO");
 contains(observer_answer(&p,&s,&g,0,"CALC 9999*9999"),"99980001");
 contains(observer_answer(&p,&s,&g,0,"CALC 10000+1"),"BETWEEN -9999 AND 9999");
 contains(observer_answer(&p,&s,&g,0,"CALC 2+3*4"),"ONLY ONE INTEGER OPERATION");
 contains(observer_answer(&p,&s,&g,0,"WHAT IS THE BAN?"),"ITS REASON IS CLASSIFIED");
 contains(observer_answer(&p,&s,&g,0,"WHO AM I"),"CARETAKER");
 contains(observer_answer(&p,&s,&g,0,"WHAT ARE YOU"),"SALVAGED NURSERY COMPUTER");
 contains(observer_answer(&p,&s,&g,0,"DO MY TAXES"),"DO NOT UNDERSTAND");
 assert(memcmp(&g,&original,sizeof(g))==0);
 assert(memcmp(&p,&before,sizeof(p))==0);
 g.energy=0;g.hunger=100;g.chaos=100;
 contains(observer_answer(&p,&s,&g,1,"BOND"),"PLEASE LET ME NAP");
 contains(observer_answer(&p,&s,&g,1,"PLAY"),"PLEASE LET ME NAP");
 contains(observer_answer(&p,&s,&g,1,"MORE"),"PLEASE LET ME NAP");
 g.energy=75;contains(observer_answer(&p,&s,&g,0,"NEEDS"),"A BOTTLE");
 g.hunger=0;g.chaos=0;g.mood=10;
 contains(observer_answer(&p,&s,&g,0,"NEEDS"),"NEEDS COMPANY");
 g.mood=80;g.corruption=80;
 contains(observer_answer(&p,&s,&g,0,"NEEDS"),"QUIET CRIB REST");
 {
  uint32_t variation=s.variation;
  original=g;before=p;
  contains(observer_answer(&p,&s,&g,1,"PLAY"),"MOONLIGHT. LAMP OFF. CRIB.");
  contains(observer_answer(&p,&s,&g,1,"DREAM"),"MOONLIGHT. LAMP OFF. CRIB.");
  assert(s.variation==variation);
  assert(memcmp(&g,&original,sizeof(g))==0);
  assert(memcmp(&p,&before,sizeof(p))==0);
 }

 /* Stage is monotonic under real input and attacks stop at zero. Repair
  * restores only the separate caretaker link, never Game statistics. */
 p.exposure=120u;assert(observer_stage(&p)==OBSERVER_REVEALED);
 observer_session_reset(&s,&g);p.integrity=100;
 original=g;
 hit(&p,&s,&g);assert(p.integrity==88u);
 while(p.integrity)hit(&p,&s,&g);
 before=p;
 for(i=0;i<2000u;i++)assert(observer_tick(&p,&s,&g,1,0)==OBSERVER_TICK_NONE);
 assert(memcmp(&before,&p,sizeof(p))==0);
 contains(observer_notice(&p,&s),"LINK OFFLINE");
 contains(observer_answer(&p,&s,&g,1,"REPAIR"),"SELECT WARDEN");
 assert(p.integrity==0u);
 contains(observer_answer(&p,&s,&g,0,"REPAIR"),"INTEGRITY 100");
 assert(p.integrity==100u&&p.repairs==1u&&s.cooldown==240u);
 before=p;contains(observer_answer(&p,&s,&g,0,"REPAIR"),"ALREADY AT 100");
 assert(memcmp(&before,&p,sizeof(p))==0);
 assert(memcmp(&g,&original,sizeof(g))==0);

 /* Saturating counters and malformed-profile checks. */
 p.exposure=UINT16_MAX;p.slaps=UINT16_MAX;p.blocks=UINT16_MAX;
 p.attacks=UINT16_MAX;p.hits=UINT16_MAX;p.teachings=UINT16_MAX;
 next_windup(&p,&s,&g);
 assert(observer_record_event(&p,&s,&g,OBSERVER_EVENT_SLAP,0u));
 assert(p.slaps==UINT16_MAX&&p.blocks==UINT16_MAX&&p.exposure==UINT16_MAX);
 hit(&p,&s,&g);assert(p.attacks==UINT16_MAX&&p.hits==UINT16_MAX);
 assert(observer_teach(&p,"FIFTH PHRASE")==2&&p.teachings==UINT16_MAX);
 assert(observer_profile_valid(&p));
 before=p;p.integrity=101u;assert(!observer_profile_valid(&p));p=before;
 p.phrases[0].confidence=16u;assert(!observer_profile_valid(&p));p=before;
 p.phrases[0].text[0]='a';assert(!observer_profile_valid(&p));p=before;
 memset(p.phrases[0].text,'A',sizeof(p.phrases[0].text));
 assert(!observer_profile_valid(&p));p=before;
 p.phrase_count=0u;assert(!observer_profile_valid(&p));p=before;
 assert(!observer_profile_valid(NULL));
 observer_session_reset(&s,&g);assert(s.windup==0u&&s.reply[0]==0);
 observer_profile_reset(&p,&g);assert(p.exposure==0u&&p.integrity==100u);
 assert(p.phrase_count==0u&&p.last_event==OBSERVER_EVENT_NONE);
 assert(!observer_record_event(&p,&s,&g,OBSERVER_EVENT_REVEAL,0u));
 assert(game_secret_egg(&g));
 assert(observer_record_event(&p,&s,&g,OBSERVER_EVENT_REVEAL,0u));
 assert(p.known_reveal&&p.exposure==120u&&observer_stage(&p)==OBSERVER_REVEALED);
 assert(!observer_record_event(&p,&s,&g,OBSERVER_EVENT_REVEAL,0u));
 contains(observer_answer(&p,&s,&g,0,"MEMORY"),"ALIEN EGG");
 contains(observer_answer(&p,&s,&g,0,"SHELL"),"THE SHELL OPENED");
 contains(observer_answer(&p,&s,&g,0,"MORE"),"CONFIRMED EVENT");
 contains(observer_answer(&p,&s,&g,1,"MEMORY"),"SHELL OPENING");
 observer_profile_reset(&p,&g);
 assert(observer_stage(&p)==OBSERVER_REVEALED&&p.known_reveal);
 assert(p.last_event==OBSERVER_EVENT_NONE&&p.care[0]==0u&&p.attacks==0u);
 assert(observer_profile_valid(&p));
 puts("PASS: observer factual replies, arithmetic, learned phrases, event-only progression, telegraphed defense and Game isolation");
 return 0;
}
