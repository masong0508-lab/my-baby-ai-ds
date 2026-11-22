#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "dialogue.h"
static const char *remembered_care(Game *state,DialogueMemory *memory,Care action){
 const char *fallback=game_care(state,action);
 Game after=*state;
 const char *reply=dialogue_contextual_reply(memory,state,action,fallback);
 unsigned i;
 assert(memcmp(state,&after,sizeof(after))==0);
 assert(reply&&strlen(reply)>10&&strlen(reply)<40);
 for(i=0;reply[i];i++)assert(reply[i]>=32&&reply[i]<=126);
 return reply;
}
static void healthy(Game *state){
 state->hunger=25;state->energy=85;state->mood=80;
 state->chaos=12;state->corruption=2;
}
static void test_recent_care_memory(void){
 Game s;DialogueMemory memory;
 const char *fallback,*reply;
 assert(sizeof(DialogueMemory)<=24u);
 game_init(&s);dialogue_memory_reset(&memory,s.days);
 (void)remembered_care(&s,&memory,FEED);
 (void)remembered_care(&s,&memory,PLAY);
 (void)remembered_care(&s,&memory,NAP);
 /* Three real prior care actions, recalled in recency order. CHAT does
  * not manufacture a new care memory or repeat the same recollection. */
 assert(strstr(remembered_care(&s,&memory,CHAT),"NAP"));
 assert(strstr(remembered_care(&s,&memory,CHAT),"PLAYTIME"));
 assert(strstr(remembered_care(&s,&memory,CHAT),"BOTTLE"));
 fallback=game_care(&s,CHAT);
 reply=dialogue_contextual_reply(&memory,&s,CHAT,fallback);
 assert(reply==fallback); /* all three memories have been spoken */
 /* Repeated feeds refresh one actual event rather than filling all four
  * slots and then telling four versions of the same bottle anecdote. */
 dialogue_memory_reset(&memory,s.days);healthy(&s);
 for(int i=0;i<40;i++)(void)remembered_care(&s,&memory,FEED);
 assert(strstr(remembered_care(&s,&memory,CHAT),"BOTTLE"));
 fallback=game_care(&s,CHAT);
 assert(dialogue_contextual_reply(&memory,&s,CHAT,fallback)==fallback);
 /* NAP plus its successful quiet-rest result counts as one interaction.
  * The older arcade event survives seven later inputs; double aging the
  * crib visit would wrongly expire it before this final conversation. */
 healthy(&s);dialogue_memory_reset(&memory,s.days);
 dialogue_memory_event(&memory,DIALOGUE_EVENT_ARCADE_WIN,s.days);
 (void)remembered_care(&s,&memory,NAP);
 assert(game_room_quiet_rest(&s,0,0));
 dialogue_memory_event(&memory,DIALOGUE_EVENT_QUIET_REST,s.days);
 assert(strstr(remembered_care(&s,&memory,CHAT),"QUIET"));
 for(unsigned i=0;i<4u;i++){
  s.hunger=80;
  (void)remembered_care(&s,&memory,CHAT);
 }
 healthy(&s);
 assert(strstr(remembered_care(&s,&memory,CHAT),"WIN"));
}
static void test_external_memory_and_capacity(void){
 static const DialogueEvent events[]={
  DIALOGUE_EVENT_FEED,DIALOGUE_EVENT_PLAY,DIALOGUE_EVENT_NAP,
  DIALOGUE_EVENT_ARCADE_WIN,DIALOGUE_EVENT_ARCADE_LOSS
 };
 static const char *const expected[]={"LOSS","WIN","NAP","PLAYTIME"};
 Game s;DialogueMemory memory;
 const char *fallback;
 unsigned i;
 game_init(&s);dialogue_memory_reset(&memory,s.days);
 /* An old cumulative win counter alone cannot invent a recent match. */
 s.wins=100;fallback=game_care(&s,CHAT);
 assert(dialogue_contextual_reply(&memory,&s,CHAT,fallback)==fallback);
 for(i=0;i<5u;i++)dialogue_memory_event(&memory,events[i],s.days);
 for(i=0;i<4u;i++){
  const char *reply=remembered_care(&s,&memory,CHAT);
  assert(strstr(reply,expected[i]));
  assert(strlen(reply)<=36u); /* fits native speech row without truncation */
 }
 fallback=game_care(&s,CHAT);
 assert(dialogue_contextual_reply(&memory,&s,CHAT,fallback)==fallback);
 /* The conditional quiet rest is the same nap, not two memories. */
 healthy(&s);dialogue_memory_reset(&memory,s.days);
 (void)remembered_care(&s,&memory,NAP);
 assert(game_room_quiet_rest(&s,0,0));
 dialogue_memory_event(&memory,DIALOGUE_EVENT_QUIET_REST,s.days);
 assert(strstr(remembered_care(&s,&memory,CHAT),"QUIET"));
 fallback=game_care(&s,CHAT);
 assert(dialogue_contextual_reply(&memory,&s,CHAT,fallback)==fallback);
}
static void test_memory_need_priority_and_expiry(void){
 Game s;DialogueMemory memory;
 const char *fallback;
 int need;
 for(need=0;need<5;need++){
  game_init(&s);dialogue_memory_reset(&memory,s.days);
  dialogue_memory_event(&memory,DIALOGUE_EVENT_ARCADE_WIN,s.days);
  if(need==0)s.hunger=80;
  if(need==1)s.energy=20;
  if(need==2)s.corruption=75;
  if(need==3)s.chaos=80;
  if(need==4)s.mood=25;
  fallback=game_care(&s,CHAT);
  assert(dialogue_contextual_reply(&memory,&s,CHAT,fallback)==fallback);
  healthy(&s);
  assert(strstr(remembered_care(&s,&memory,CHAT),"WIN"));
 }
 game_init(&s);dialogue_memory_reset(&memory,s.days);
 dialogue_memory_event(&memory,DIALOGUE_EVENT_ARCADE_WIN,s.days);
 /* Unspoken memories still expire: urgency must not preserve an old
  * event forever and recall it as soon as the baby becomes comfortable. */
 for(int i=0;i<7;i++){
  s.hunger=80;
  (void)remembered_care(&s,&memory,CHAT);
 }
 healthy(&s);fallback=game_care(&s,CHAT);
 assert(dialogue_contextual_reply(&memory,&s,CHAT,fallback)==fallback);
 /* Changing days expires memory before the next interaction; no room
  * callback is needed to prevent last-day references after END DAY. */
 dialogue_memory_event(&memory,DIALOGUE_EVENT_ARCADE_LOSS,s.days);
 (void)game_end_day(&s);healthy(&s);
 fallback=game_care(&s,CHAT);
 assert(dialogue_contextual_reply(&memory,&s,CHAT,fallback)==fallback);
 dialogue_memory_event(&memory,DIALOGUE_EVENT_ARCADE_WIN,s.days);
 dialogue_memory_event(&memory,DIALOGUE_EVENT_PLAY,s.days+1u);
 s.days++;healthy(&s);
 assert(strstr(remembered_care(&s,&memory,CHAT),"PLAYTIME"));
 fallback=game_care(&s,CHAT);
 assert(dialogue_contextual_reply(&memory,&s,CHAT,fallback)==fallback);
}
static void test_memory_repetition_and_isolation(void){
 Game a,b;DialogueMemory ma,mb;
 const char *last=NULL;
 unsigned i,avoided=0;
 game_init(&a);a.corruption=85;b=a;
 dialogue_memory_reset(&ma,a.days);dialogue_memory_reset(&mb,b.days);
 for(i=0;i<1024u;i++){
  const char *fallback=game_care(&a,CHAT);
  const char *baseline=game_care(&b,CHAT);
  const char *reply=dialogue_contextual_reply(&ma,&a,CHAT,fallback);
  assert(strcmp(fallback,baseline)==0);
  assert(memcmp(&a,&b,sizeof(a))==0); /* identical shared RNG trajectory */
  if(last){
   if(strcmp(last,fallback)==0)avoided++;
   assert(strcmp(last,reply)!=0);
  }
  last=reply;
 }
 assert(avoided>10u);
 game_init(&a);b=a;
 dialogue_memory_reset(&ma,a.days);dialogue_memory_reset(&mb,b.days);
 for(i=0;i<1024u;i++){
  Care action=(Care)((i*7u+i/9u)%4u);
  const char *ra=remembered_care(&a,&ma,action);
  const char *rb=remembered_care(&b,&mb,action);
  assert(strcmp(ra,rb)==0&&memcmp(&a,&b,sizeof(a))==0);
  assert(memcmp(&ma,&mb,sizeof(ma))==0);
 }
 /* A second session, or loading a different baby on the same day, must
  * not inherit the first baby's memories or its last dialogue choice. */
 healthy(&a);b=a;
 dialogue_memory_reset(&ma,a.days);dialogue_memory_reset(&mb,b.days);
 dialogue_memory_event(&ma,DIALOGUE_EVENT_ARCADE_WIN,a.days);
 const char *empty_fallback=game_care(&b,CHAT);
 assert(dialogue_contextual_reply(&mb,&b,CHAT,empty_fallback)==empty_fallback);
 assert(strstr(remembered_care(&a,&ma,CHAT),"WIN"));
 dialogue_memory_event(&ma,DIALOGUE_EVENT_ARCADE_LOSS,a.days);
 dialogue_memory_reset(&ma,a.days);
 const char *reset_fallback=game_care(&a,CHAT);
 assert(dialogue_contextual_reply(&ma,&a,CHAT,reset_fallback)==reset_fallback);
 /* Optional callers and invalid actions retain their existing response. */
 assert(dialogue_contextual_reply(NULL,&a,CHAT,reset_fallback)==reset_fallback);
 assert(dialogue_contextual_reply(&ma,NULL,CHAT,reset_fallback)==reset_fallback);
 DialogueMemory before=ma;
 const char *invalid="INVALID";
 assert(dialogue_contextual_reply(&ma,&a,(Care)7,invalid)==invalid);
 dialogue_memory_event(&ma,(DialogueEvent)0,a.days);
 assert(memcmp(&before,&ma,sizeof(ma))==0);
}
static void test_all_memory_variants(void){
 Game s;DialogueMemory memory;
 const char *seen[12];
 unsigned kind,variant,n=0,i;
 game_init(&s);healthy(&s);
 for(kind=DIALOGUE_EVENT_FEED;kind<=DIALOGUE_EVENT_QUIET_REST;kind++){
  for(variant=0;variant<2u;variant++){
   dialogue_memory_reset(&memory,s.days);
   dialogue_memory_event(&memory,(DialogueEvent)kind,s.days);
   /* Sample both deterministic branches without consuming or seeding
    * the Game's RNG through an extra dialogue_reply call. */
   s.rng=variant<<16;
   Game after=s;
   const char *reply=dialogue_contextual_reply(&memory,&s,CHAT,"FALLBACK");
   assert(memcmp(&s,&after,sizeof(s))==0);
   assert(strlen(reply)>10u&&strlen(reply)<=36u);
   for(i=0;reply[i];i++)assert(reply[i]>=32&&reply[i]<=126);
   for(i=0;i<n;i++)assert(strcmp(reply,seen[i])!=0);
   seen[n++]=reply;
   /* Another real event of the same kind may be recalled again, but
    * cannot repeat the immediately preceding line even with same RNG. */
   dialogue_memory_event(&memory,(DialogueEvent)kind,s.days);
   const char *again=dialogue_contextual_reply(&memory,&s,CHAT,"FALLBACK");
   assert(strcmp(reply,again)!=0);
   assert(memcmp(&s,&after,sizeof(s))==0);
  }
 }
 assert(n==12u);
}
int main(void){
 Game s;int i,j,counts[6]={0};
 game_init(&s);
 for(i=0;i<6;i++)for(j=0;j<20;j++){
  const char *line;
  s.hunger=(uint8_t)((i==1)?99:10+j);
  s.energy=(uint8_t)((i==2)?1:76);
  s.mood=(uint8_t)((i==3)?98:50);
  s.corruption=(uint8_t)((i==4)?98:3);
  s.chaos=(uint8_t)((i==5)?99:2);
  int intent=dialogue_intent(&s,CHAT);
  assert(intent>=0&&intent<6);
  /* Every archetypal mood must survive a normal CHAT action. The old
   * classifier returned only Balanced or Playful for these 120 samples. */
  assert(intent==i);
  counts[intent]++;
  line=dialogue_reply(&s,(Care)(j%4));
  assert(line&&strlen(line)>10&&strlen(line)<40);
 }
 for(i=0;i<6;i++)assert(counts[i]==20);
 /* Interaction-based checks: inspect classifier AFTER ordinary care has
  * modified need meters, the moment dialogue_reply() actually runs. */
 {
  Game a;
  game_init(&a);
  (void)game_care(&a,CHAT);
  assert(dialogue_intent(&a,CHAT)==0); /* healthy conversation */
  game_init(&a);
  a.hunger=99;a.mood=50;a.energy=76;a.chaos=2;a.corruption=3;
  (void)game_care(&a,FEED);
  assert(a.hunger==69&&dialogue_intent(&a,FEED)==1);
  game_init(&a);
  a.energy=1;
  (void)game_care(&a,NAP);
  assert(a.energy==26&&dialogue_intent(&a,NAP)==2);
  game_init(&a);
  (void)game_care(&a,PLAY);
  assert(dialogue_intent(&a,PLAY)==3);
  game_init(&a);
  a.corruption=98;
  (void)game_care(&a,CHAT);
  assert(dialogue_intent(&a,CHAT)==4);
  game_init(&a);
  a.chaos=98;
  (void)game_care(&a,CHAT);
  assert(dialogue_intent(&a,CHAT)==5);
 }
 /* Eight distinct replies can all appear for an unchanged emotional
  * state; replaying identical saved RNG yields identical lines. This
  * detects both an unfilled response slot and accidental RNG changes. */
 {
  Game a,b;
  const char *seen[8]={0};
  int count=0;
  game_init(&a);
  a.hunger=98;a.energy=30;a.mood=45;
  b=a;
  for(i=0;i<512;i++){
   const char *line=dialogue_reply(&a,CHAT);
   const char *copy=dialogue_reply(&b,CHAT);
   int found=0;
   assert(strcmp(line,copy)==0);
   assert(a.rng==b.rng);
   assert(strlen(line)>10&&strlen(line)<40);
   for(j=0;j<count;j++)if(strcmp(seen[j],line)==0){found=1;break;}
   if(!found){
    assert(count<8);
    seen[count++]=line;
   }
  }
  assert(count==8);
 }
 for(i=0;i<6;i++)printf("classifier class %d: %d observations\n",i,counts[i]);
 test_recent_care_memory();
 test_external_memory_and_capacity();
 test_memory_need_priority_and_expiry();
 test_memory_repetition_and_isolation();
 test_all_memory_variants();
 puts("PASS: bounded, deterministic offline integer dialogue engine");
 puts("PASS: truthful recent-event recall, urgent needs, expiry, isolation and no-repeat replies");
 return 0;
}
