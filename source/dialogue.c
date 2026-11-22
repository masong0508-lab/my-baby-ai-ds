#include <stdint.h>
#include <string.h>
#include "dialogue.h"
/*
 * Compact, offline Q0-ish integer neural classifier. Hand-tuned fixed
 * weights, NOT a pretrained large language model. Six inputs -> 8 ReLU
 * neurons -> 6 output classes, with 8 original responses per class.
 * < 1KB of weights, about 100 integer multiply-adds per care action.
 */
#define INPUTS 6
#define HIDDEN 8
#define OUTPUTS 6
#define VARIANTS 8
static const int8_t first[HIDDEN][INPUTS]={
 {0,2,0,-1,-1,5},
 {9,-2,0,1,0,-2},
 {0,-1,-9,0,0,-2},
 {-2,9,5,-1,-1,2},
 {0,-1,0,3,9,0},
 {-1,-4,-1,9,3,1},
 {4,1,0,1,0,2},
 {0,1,2,0,-2,2}
};
static const int8_t hidden_bias[HIDDEN]={12,0,0,0,0,0,4,3};
static const int8_t second[OUTPUTS][HIDDEN]={
 {3,0,0,0,0,0,1,1},
 {0,4,0,0,0,0,2,0},
 {0,0,4,0,0,0,0,2},
 {0,0,0,4,0,0,0,2},
 {0,0,0,0,4,1,0,0},
 {0,0,0,0,1,4,0,0}
};
static const int16_t final_bias[OUTPUTS]={15,0,0,0,0,0};
static const char *const lines[OUTPUTS][VARIANTS]={
 /* Balanced / independent. */
 {"I OWN THREE PERCENT OF THIS CRIB.","THE SHAREHOLDERS DEMAND JUICE.",
  "MY DIAPER HAS A NONCOMPETE.","I'LL FILE A COMPLAINT WITH HR.",
  "MY NAP SCHEDULE HAS A BOARD MEETING.","THE RATTLE NEEDS A REVIEW.",
  "CAN I GET A RAISE IN PACIFIERS?","I'M PUTTING TEDDY ON PAYROLL."},
 /* Hunger / bottle requests. */
 {"THIS BOTTLE HAS MICROTRANSACTIONS.","WHERE ARE MY LUNCH BENEFITS?",
  "MY MILK HAS AN EXPIRATION POLICY.","I'M SUING THE SNACK CABINET.",
  "THE BOTTLE LINE HAS A WAITLIST.","SNACK TIME IS A CORE BENEFIT.",
  "THIS FORMULA IS OUT OF STOCK.","THE HIGH CHAIR IS A FOOD COURT."},
 /* Sleep / low energy. */
 {"MY BEDTIME HAS TERMS OF SERVICE.","I NEED A FIVE-MINUTE SABBATICAL.",
  "WAKE ME WHEN THE MARKET RECOVERS.","I SLEEP IN A TAX-DEDUCTIBLE CRIB.",
  "PLEASE LOWER THE OFFICE LIGHTS.","I CLOCKED OUT BEFORE BEDTIME.",
  "THE PILLOW REQUESTS PAID LEAVE.","MY DREAMS HAVE A LOADING SCREEN."},
 /* Play / happy social baby. */
 {"I AM THE MAYOR OF PLAYTIME.","THE TEDDY BEAR IS MY COFOUNDER.",
  "I MADE A CHART ABOUT HUGS.","EVERY RATTLE IS AN OPPORTUNITY.",
  "TEDDY AND I JUST WON THE RAFFLE.","THE RATTLE HAS ITS OWN PARADE.",
  "PLAYTIME IS OUR BEST QUARTER.","I APPOINTED A CHIEF OF GIGGLES."},
 /* Glitch / surreal. */
 {"ERROR: BABY HAS TOO MANY TABS.","MY SPOON JUST TURNED INTO A JPEG.",
  "THE CRIB IS IN DEBUG MODE.","WHY IS THE FLOOR IN LOWERCASE?",
  "MY TOES JUST FAILED A UNIT TEST.","THE MOBILE IS IN DEBUG MODE.",
  "WHO MOVED THE CRIB'S HITBOX?","THIS NIGHTLIGHT NEEDS A PATCH."},
 /* Chaotic / rebellious. */
 {"WE HAVE UNIONIZED THE TOYS.","THIS NURSERY NEEDS A MANAGER.",
  "MY TANTRUM HAS A PRESS RELEASE.","I AM FILING A NOISE COMPLAINT.",
  "THE PLUSHIES VOTED FOR RECESS.","THIS TANTRUM IS A GROUP PROJECT.",
  "I'M AUDITING THE COOKIE JAR.","THE CRIB HAS DECLARED STRIKE."}
};
static void features(const Game *s,Care action,int8_t x[INPUTS]){
 x[0]=(int8_t)(((int)s->hunger-50)/5);
 x[1]=(int8_t)(((int)s->mood-50)/5);
 x[2]=(int8_t)(((int)s->energy-50)/5);
 x[3]=(int8_t)(((int)s->chaos-50)/5);
 x[4]=(int8_t)(((int)s->corruption-50)/5);
 x[5]=(int8_t)(action==CHAT?10:action==PLAY?5:action==FEED?-4:-6);
}
int dialogue_intent(const Game *s,Care action){
 int8_t input[INPUTS];int16_t hidden[HIDDEN];
 int i,j,best=0,score=-32767;
 features(s,action,input);
 for(i=0;i<HIDDEN;i++){
  int sum=hidden_bias[i]*4;
  for(j=0;j<INPUTS;j++)sum+=first[i][j]*input[j];
  if(sum<0)sum=0;
  if(sum>127)sum=127;
  hidden[i]=(int16_t)sum;
 }
 for(i=0;i<OUTPUTS;i++){
  int value=final_bias[i];
  for(j=0;j<HIDDEN;j++)value+=second[i][j]*hidden[j];
  /* Slight care-action bias makes changes feel immediately reactive. */
  if(i==1&&action==FEED)value+=40;
  if(i==2&&action==NAP)value+=40;
  if(i==3&&action==PLAY)value+=40;
  /* Care remains a 6->8->6 offline integer classifier, but its original
   * CHAT-specific play bias drowned out critical biological needs. These
   * small, bounded output priors keep ALL six authored moods reachable:
   * urgent hunger, sleep, glitches and chaos must not sound generically
   * playful when a player talks to a distressed infant. The positive
   * CHAT baseline keeps an otherwise healthy infant conversational. */
  if(i==0&&action==CHAT)value+=125;
  if(i==1&&s->hunger>=65)value+=((int)s->hunger-60)*8;
  if(i==2&&s->energy<=40)value+=(45-(int)s->energy)*7;
  if(i==4&&s->corruption>=65)value+=((int)s->corruption-60)*10;
  if(i==5&&s->chaos>=65)value+=((int)s->chaos-60)*11;
  if(value>score){score=value;best=i;}
 }
 return best;
}
const char *dialogue_reply(Game *state,Care action){
 uint32_t x=state->rng;
 int intent=dialogue_intent(state,action);
 if(!x)x=0xBADA551u;
 x^=x<<13;x^=x>>17;x^=x<<5;
 state->rng=x;
 /* One extra bit of the EXISTING save-compatible RNG doubles variety.
  * No new Game field, transcript buffer or runtime text allocation. */
 return lines[intent][(x>>12)&(VARIANTS-1u)];
}
/* Twelve authored recollections, selected only after a corresponding
 * event really happened. Static ROM text, not a generated transcript.
 * Every line fits one 36-character nursery speech row. */
#define MEMORY_KINDS 6
#define MEMORY_VARIANTS 2
#define MEMORY_CAPACITY 4u
#define MEMORY_MAX_AGE 8u
#define NO_REPLY UINT8_MAX
static const char *const memories[MEMORY_KINDS][MEMORY_VARIANTS]={
 {"THAT BOTTLE HIT THE SPOT. THANK YOU.",
  "THE LAST BOTTLE EARNED FIVE STARS."},
 {"OUR PLAYTIME DESERVES AN ENCORE.",
  "YOU MADE PLAYTIME FEEL SPECIAL."},
 {"THAT NAP FIXED MY WHOLE AFTERNOON.",
  "I AM BACK FROM MY PAID NAP. THANKS."},
 {"THAT ARCADE WIN GOES ON YOUR REVIEW.",
  "OUR WIN EARNED AN EXTRA GIGGLE."},
 {"THAT ARCADE LOSS IS OFF THE RECORD.",
  "ONE LOSS WILL NOT CANCEL PLAYTIME."},
 {"THAT QUIET CRIB NAP WAS FIRST CLASS.",
  "THE DARK, QUIET CRIB HELPED ME REST."}
};
void dialogue_memory_reset(DialogueMemory *memory,uint32_t day){
 if(!memory)return;
 memset(memory,0,sizeof(*memory));
 memory->day=day;memory->last_reply=NO_REPLY;
}
static void memory_sync_day(DialogueMemory *memory,uint32_t day){
 if(memory->day!=day)dialogue_memory_reset(memory,day);
}
static void memory_remove(DialogueMemory *memory,unsigned index){
 unsigned i;
 for(i=index+1u;i<memory->count;i++)
  memory->recent[i-1u]=memory->recent[i];
 if(memory->count)memory->count--;
}
static void memory_age(DialogueMemory *memory){
 unsigned i=0;
 while(i<memory->count){
  memory->recent[i].age++;
  if(memory->recent[i].age>=MEMORY_MAX_AGE)memory_remove(memory,i);
  else i++;
 }
}
static void memory_record(DialogueMemory *memory,DialogueEvent event){
 unsigned i=0;
 while(i<memory->count){
  if(memory->recent[i].kind==(unsigned)event||
     (event==DIALOGUE_EVENT_QUIET_REST&&
      memory->recent[i].kind==DIALOGUE_EVENT_NAP))memory_remove(memory,i);
  else i++;
 }
 if(memory->count>=MEMORY_CAPACITY)memory->count=MEMORY_CAPACITY-1u;
 for(i=memory->count;i>0u;i--)memory->recent[i]=memory->recent[i-1u];
 memory->recent[0].kind=(uint8_t)event;
 memory->recent[0].age=0;memory->recent[0].spoken=0;
 memory->count++;
}
void dialogue_memory_event(DialogueMemory *memory,DialogueEvent event,
                           uint32_t day){
 if(!memory||event<DIALOGUE_EVENT_FEED||event>DIALOGUE_EVENT_QUIET_REST)
  return;
 memory_sync_day(memory,day);
 /* Successful quiet rest completes the NAP just recorded by care. It
  * upgrades that one interaction, not a second tick of every memory. */
 if(!(event==DIALOGUE_EVENT_QUIET_REST&&memory->count&&
      memory->recent[0].kind==DIALOGUE_EVENT_NAP&&
      memory->recent[0].age==0))memory_age(memory);
 memory_record(memory,event);
}
static unsigned known_reply(const char *reply){
 unsigned i,j;
 if(!reply)return NO_REPLY;
 for(i=0;i<OUTPUTS;i++)for(j=0;j<VARIANTS;j++)
  if(reply==lines[i][j])return i*VARIANTS+j;
 return NO_REPLY;
}
const char *dialogue_contextual_reply(DialogueMemory *memory,
                                      const Game *state,Care action,
                                      const char *fallback){
 unsigned reply,i;
 if(!memory||!state||action<FEED||action>NAP)return fallback;
 memory_sync_day(memory,state->days);
 memory_age(memory);
 /* Current needs take precedence over a cheerful reminiscence even if
  * the intent classifier happens to prefer a different authored mood. */
 if(action==CHAT&&state->hunger<65&&state->energy>40&&
    state->mood>40&&state->corruption<60&&state->chaos<65){
  int intent=dialogue_intent(state,CHAT);
  if(intent==0||intent==3){
   for(i=0;i<memory->count;i++){
    DialogueMemoryEntry *entry=&memory->recent[i];
    if(!entry->spoken){
     unsigned kind=(unsigned)entry->kind-1u;
     unsigned variant=(state->rng>>16)&1u;
     reply=OUTPUTS*VARIANTS+kind*MEMORY_VARIANTS+variant;
     if(reply==memory->last_reply){variant^=1u;reply^=1u;}
     entry->spoken=1;memory->last_reply=(uint8_t)reply;
     return memories[kind][variant];
    }
   }
  }
 }
 /* game_care already advanced the shared RNG exactly once. If it chose
  * the preceding line again, take another variant of that SAME intent
  * using existing bits, so neither game randomness nor needs change. */
 reply=known_reply(fallback);
 if(reply!=NO_REPLY&&reply==memory->last_reply){
  unsigned intent=reply/VARIANTS;
  unsigned variant=reply%VARIANTS;
  variant=(variant+1u+(state->rng>>20)%(VARIANTS-1u))%VARIANTS;
  reply=intent*VARIANTS+variant;
  fallback=lines[intent][variant];
 }
 memory->last_reply=(uint8_t)reply;
 if(action==FEED)memory_record(memory,DIALOGUE_EVENT_FEED);
 else if(action==PLAY)memory_record(memory,DIALOGUE_EVENT_PLAY);
 else if(action==NAP)memory_record(memory,DIALOGUE_EVENT_NAP);
 return fallback;
}
