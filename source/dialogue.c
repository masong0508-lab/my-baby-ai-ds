#include <stdint.h>
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
