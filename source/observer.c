#include "observer.h"
#include "narrative_lm.h"
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(ObserverProfile)<=256u,"observer profile budget");
_Static_assert(sizeof(ObserverSession)<=192u,"observer session budget");
enum {FOCUS_STATUS=0,FOCUS_NEEDS,FOCUS_MEMORY,FOCUS_WORLD,FOCUS_SELF,
      FOCUS_PLAY,FOCUS_DREAM,FOCUS_THREAT,FOCUS_TEACH,FOCUS_CALC,FOCUS_BOND,
      FOCUS_BAN,FOCUS_CLINIC,FOCUS_SHELL,FOCUS_TOYBOX};
enum {NOTICE_NONE=0,NOTICE_WINDUP,NOTICE_HIT,NOTICE_BLOCK,NOTICE_REPAIR};

static uint16_t sat_add(uint16_t value,uint32_t amount){
 return (uint16_t)(amount>UINT16_MAX-(uint32_t)value?UINT16_MAX:value+amount);
}
static const char *say(ObserverSession *s,const char *format,...){
 va_list args;
 size_t i;
 if(!s)return "WARDEN UNAVAILABLE.";
 va_start(args,format);
 (void)vsnprintf(s->reply,145u,format,args);
 va_end(args);
 for(i=0;s->reply[i];i++)
  if(s->reply[i]>='a'&&s->reply[i]<='z')s->reply[i]=(char)(s->reply[i]-32);
 return s->reply;
}
/* Bounded ASCII normalization. Teaching accepts punctuation as inert
 * data, never as a command to run later. No locale or allocation. */
static int normalize(char *out,size_t cap,const char *in){
 size_t i=0u,n=0u;
 int pending=0;
 if(!out||cap<1u||!in)return 0;
 out[0]=0;
 for(i=0u;i<160u&&in[i];i++){
  unsigned c=(unsigned char)in[i];
  if(c==' '||c=='\t'||c=='\r'||c=='\n'){
   if(n)pending=1;
   continue;
  }
  if(c<32u||c>126u)return 0;
  if(pending){if(n+1u>=cap)return 0;out[n++]=' ';pending=0;}
  if(n+1u>=cap)return 0;
  if(c>='a'&&c<='z')c-=32u;
  out[n++]=(char)c;
 }
 if(i==160u)return 0;
 out[n]=0;
 return 1;
}
static int has(const char *text,const char *word){
 size_t size=strlen(word);
 const char *p=text;
 while((p=strstr(p,word))!=NULL){
  unsigned char left=p==text?' ':(unsigned char)p[-1];
  unsigned char right=(unsigned char)p[size];
  if(!(left>='A'&&left<='Z')&&!(left>='0'&&left<='9')&&
     !(right>='A'&&right<='Z')&&!(right>='0'&&right<='9'))return 1;
  p++;
 }
 return 0;
}
/* The terminal's compact bitmap font has these printable punctuation
 * glyphs. Reject unsupported glyphs instead of silently losing a taught
 * character. Normalized letters, digits and spaces are always accepted. */
static int phrase_character(unsigned c){
 return (c>='A'&&c<='Z')||(c>='0'&&c<='9')||
        (c&&strchr(" .,!?:;-/+*='\"()%$<>#",(int)c)!=NULL);
}
static void record(ObserverProfile *p,const Game *g,unsigned event,
                    unsigned detail,unsigned effect){
 p->last_event=(uint8_t)event;p->last_detail=(uint8_t)detail;
 p->last_effect=(uint8_t)effect;p->last_event_day=g?g->days:p->last_day;
}
void observer_profile_reset(ObserverProfile *p,const Game *g){
 if(!p)return;
 memset(p,0,sizeof(*p));p->integrity=100;
 p->last_day=g?g->days:0u;
 /* An old v1 save can already contain the real egg reveal. Preserve that
  * established knowledge without inventing dated observer events. */
 if(g&&(g->ending_mask&(1u<<END_EGG))){p->known_reveal=1;p->exposure=120u;}
}
int observer_profile_valid(const ObserverProfile *p){
 unsigned i,count=0;
 if(!p||p->integrity>100u||p->last_event>OBSERVER_EVENT_REVEAL||
    p->last_effect>100u||p->phrase_count>OBSERVER_PHRASES||
    p->phrase_next>=OBSERVER_PHRASES||p->known_reveal>1u||
    (p->known_reveal&&p->exposure<120u))return 0;
 if((p->last_event==OBSERVER_EVENT_ARCADE_WIN||
     p->last_event==OBSERVER_EVENT_ARCADE_LOSS)&&p->last_detail>=MBAI_GAMES)
  return 0;
 for(i=0;i<OBSERVER_PHRASES;i++){
  const ObserverPhrase *phrase=&p->phrases[i];
  size_t j;
  if(phrase->confidence>15u)return 0;
  for(j=0;j<=OBSERVER_PHRASE_CHARS&&phrase->text[j];j++){
   unsigned c=(unsigned char)phrase->text[j];
   if(!phrase_character(c))return 0;
   if(c==' '&&(j==0u||phrase->text[j-1u]==' '))return 0;
  }
  if(j>OBSERVER_PHRASE_CHARS)return 0;
  if((j==0u)!=(phrase->confidence==0u))return 0;
  if(j&&phrase->text[j-1u]==' ')return 0;
  if(j){
   unsigned previous;
   count++;
   for(previous=0;previous<i;previous++)
    if(p->phrases[previous].confidence&&
       strcmp(p->phrases[previous].text,phrase->text)==0)return 0;
  }
  for(;j<=OBSERVER_PHRASE_CHARS;j++)if(phrase->text[j])return 0;
 }
 return count==p->phrase_count;
}
void observer_session_reset(ObserverSession *s,const Game *g){
 if(!s)return;
 memset(s,0,sizeof(*s));
 s->cooldown=OBSERVER_COOLDOWN_TICKS;
 s->variation=(g?g->rng:0u)^0x57415244u;
 if(g){s->seen_care_rng=g->rng;s->seen_wins=g->wins;
       s->seen_losses=g->losses;s->seen_quiet_rng=g->rng;}
}
unsigned observer_stage(const ObserverProfile *p){
 unsigned value=p?p->exposure:0u;
 return value>=120u?OBSERVER_REVEALED:value>=64u?OBSERVER_HOSTILE:
        value>=24u?OBSERVER_WATCHING:OBSERVER_FOUND;
}
const char *observer_stage_name(unsigned stage){
 static const char *const names[]={"FOUND","WATCHING","HOSTILE","REVEALED"};
 return names[stage<=OBSERVER_REVEALED?stage:OBSERVER_FOUND];
}
static int learned_quote(const ObserverProfile *p,ObserverSession *s,
                          unsigned minimum){
 unsigned n;
 for(n=0;n<OBSERVER_PHRASES;n++){
  unsigned i=(s->quote_cursor+n)%OBSERVER_PHRASES;
  if(p->phrases[i].confidence>=minimum){
   say(s,"YOU TAUGHT ME: %s",p->phrases[i].text);
   s->quote_cursor=(uint8_t)((i+1u)%OBSERVER_PHRASES);
   return 1;
  }
 }
 return 0;
}
int observer_record_care(ObserverProfile *p,ObserverSession *s,
                         const Game *before,const Game *after,Care action){
 unsigned effect=0;
 if(!p||!s||!before||!after||(unsigned)action>NAP||
    before->rng==after->rng||s->seen_care_rng==after->rng)return 0;
 s->seen_care_rng=after->rng;s->echo_ready=0;
 p->care[action]=sat_add(p->care[action],1u);
 p->exposure=sat_add(p->exposure,action==PLAY?2u:1u);
 if(action==FEED&&before->hunger>after->hunger)
  effect=before->hunger-after->hunger;
 if(action==CHAT&&after->mood>before->mood)effect=after->mood-before->mood;
 if(action==PLAY&&before->energy>after->energy)effect=before->energy-after->energy;
 if(action==NAP&&after->energy>before->energy)effect=after->energy-before->energy;
 record(p,after,OBSERVER_EVENT_FEED+(unsigned)action,0u,effect);
 /* A phrase must be reinforced three times and actual healthy play must
  * occur. One of every three play events may quote it; menus cannot do so. */
 if(action==PLAY&&p->care[PLAY]%3u==1u&&game_care_hint(after)<0&&
    after->corruption<60u&&
    learned_quote(p,s,3u))s->echo_ready=1;
 return 1;
}
static void block_attack(ObserverProfile *p,ObserverSession *s,
                          const Game *g,unsigned method){
 s->windup=0;s->cooldown=OBSERVER_COOLDOWN_TICKS;
 s->notice=NOTICE_BLOCK;s->notice_ticks=90u;
 p->blocks=sat_add(p->blocks,1u);
 record(p,g,OBSERVER_EVENT_BLOCK,method,0u);
}
int observer_record_event(ObserverProfile *p,ObserverSession *s,
                          const Game *g,ObserverEvent event,unsigned detail){
 uint32_t exposure=0;
 if(!p||!s||!g)return 0;
 switch(event){
 case OBSERVER_EVENT_ARCADE_WIN:
  if(detail>=MBAI_GAMES||g->wins<=s->seen_wins)return 0;
  s->seen_wins=g->wins;p->arcade_wins=sat_add(p->arcade_wins,1u);
  exposure=3u;break;
 case OBSERVER_EVENT_ARCADE_LOSS:
  if(detail>=MBAI_GAMES||g->losses<=s->seen_losses)return 0;
  s->seen_losses=g->losses;p->arcade_losses=sat_add(p->arcade_losses,1u);
  exposure=3u;break;
 case OBSERVER_EVENT_QUIET_REST:
  if(s->seen_quiet_rng==g->rng)return 0;
  s->seen_quiet_rng=g->rng;p->quiet_rests=sat_add(p->quiet_rests,1u);
  exposure=1u;detail=0u;break;
 case OBSERVER_EVENT_DAY:
  if(g->days<=p->last_day)return 0;
  exposure=g->days-p->last_day;
  p->days=sat_add(p->days,exposure);p->last_day=g->days;
  exposure=exposure>UINT16_MAX/6u?UINT16_MAX:exposure*6u;
  detail=0u;break;
 case OBSERVER_EVENT_SLAP:
  p->slaps=sat_add(p->slaps,1u);exposure=2u;detail=0u;break;
 case OBSERVER_EVENT_KICK:
  p->kicks=sat_add(p->kicks,1u);exposure=3u;detail=0u;break;
 case OBSERVER_EVENT_REVEAL:
  if(!(g->ending_mask&(1u<<END_EGG))||p->known_reveal)return 0;
  p->known_reveal=1;
  if(p->exposure<120u)p->exposure=120u;
  detail=0u;break;
 default:return 0;
 }
 p->exposure=sat_add(p->exposure,exposure);
 record(p,g,(unsigned)event,detail,0u);
 if(s->windup&&(event==OBSERVER_EVENT_SLAP||event==OBSERVER_EVENT_KICK))
  block_attack(p,s,g,(unsigned)event);
 return 1;
}
ObserverTick observer_tick(ObserverProfile *p,ObserverSession *s,
                           const Game *g,int active,int guard){
 unsigned damage;
 if(!p||!s||!g||!active)return OBSERVER_TICK_NONE;
 if(s->notice_ticks)s->notice_ticks--;
 if(!p->integrity){s->windup=0;return OBSERVER_TICK_NONE;}
 if(observer_stage(p)<OBSERVER_HOSTILE){
  s->windup=0;s->cooldown=OBSERVER_COOLDOWN_TICKS;
  return OBSERVER_TICK_NONE;
 }
 if(s->windup){
  if(guard){block_attack(p,s,g,0u);return OBSERVER_TICK_BLOCK;}
  s->windup--;
  if(s->windup)return OBSERVER_TICK_NONE;
  damage=observer_stage(p)==OBSERVER_REVEALED?12u:8u;
  if(damage>p->integrity)damage=p->integrity;
  p->integrity=(uint8_t)(p->integrity-damage);
  p->hits=sat_add(p->hits,1u);
  s->cooldown=OBSERVER_COOLDOWN_TICKS;
  s->notice=NOTICE_HIT;s->notice_ticks=90u;
  record(p,g,OBSERVER_EVENT_HIT,0u,damage);
  return OBSERVER_TICK_HIT;
 }
 if(s->cooldown){s->cooldown--;if(s->cooldown)return OBSERVER_TICK_NONE;}
 s->windup=OBSERVER_WINDUP_TICKS;s->notice=NOTICE_WINDUP;
 s->notice_ticks=OBSERVER_WINDUP_TICKS;
 p->attacks=sat_add(p->attacks,1u);
 record(p,g,OBSERVER_EVENT_WINDUP,0u,0u);
 return OBSERVER_TICK_WINDUP;
}
const char *observer_notice(const ObserverProfile *p,const ObserverSession *s){
 if(!p||!s)return "";
 if(!p->integrity)return "BRACER LINK OFFLINE.\nTYPE REPAIR AT WARDEN.";
 if(s->windup)return "ATTACK WINDUP!\nGUARD OR COUNTER NOW.";
 if(!s->notice_ticks)return "";
 if(s->notice==NOTICE_HIT)return "YOUR BRACER TOOK A HIT!\nCHECK WARDEN FOR INTEGRITY.";
 if(s->notice==NOTICE_BLOCK)return "ATTACK BLOCKED.\nWARDEN LOGGED YOUR COUNTER.";
 if(s->notice==NOTICE_REPAIR)return "BRACER LINK RESTORED.\nINTEGRITY 100.";
 return "";
}
int observer_teach(ObserverProfile *p,const char *text){
 char clean[OBSERVER_PHRASE_CHARS+1u];
 unsigned i,slot;
 if(!observer_profile_valid(p)||!normalize(clean,sizeof(clean),text)||!clean[0])return 0;
 for(i=0;clean[i];i++)if(!phrase_character((unsigned char)clean[i]))return 0;
 for(i=0;i<OBSERVER_PHRASES;i++){
  if(p->phrases[i].confidence&&strcmp(clean,p->phrases[i].text)==0){
   if(p->phrases[i].confidence<15u)p->phrases[i].confidence++;
   p->teachings=sat_add(p->teachings,1u);return 2;
  }
 }
 /* A new lesson must never erase a familiar word. Reuse only an
  * explicitly vacant slot, preserving all other confidence values. */
 if(p->phrase_count==OBSERVER_PHRASES)return 3;
 slot=OBSERVER_PHRASES;
 for(i=0;i<OBSERVER_PHRASES;i++){
  unsigned candidate=(p->phrase_next+i)%OBSERVER_PHRASES;
  if(!p->phrases[candidate].confidence){slot=candidate;break;}
 }
 if(slot==OBSERVER_PHRASES)return 0;
 memset(&p->phrases[slot],0,sizeof(p->phrases[slot]));
 memcpy(p->phrases[slot].text,clean,strlen(clean)+1u);
 p->phrases[slot].confidence=1;
 p->phrase_count++;
 p->phrase_next=(uint8_t)((slot+1u)%OBSERVER_PHRASES);
 p->teachings=sat_add(p->teachings,1u);
 return 1;
}
int observer_forget_phrase(ObserverProfile *p,unsigned slot){
 if(!observer_profile_valid(p)||slot>=OBSERVER_PHRASES||
    !p->phrases[slot].confidence)return 0;
 memset(&p->phrases[slot],0,sizeof(p->phrases[slot]));
 p->phrase_count--;
 p->phrase_next=p->phrase_count?(uint8_t)slot:0u;
 return 1;
}
const char *observer_report(const ObserverProfile *p,ObserverSession *s,
                            const Game *g){
 if(!p||!s||!g)return "WARDEN UNAVAILABLE.";
 s->focus=FOCUS_STATUS;
 return say(s,"%s. DAY %lu.\nCARE F%u C%u P%u N%u.\nINTEGRITY %u. HITS %u.\nASK NEEDS, MEMORY OR HELP.",
  observer_stage_name(observer_stage(p)),(unsigned long)g->days,
  (unsigned)p->care[FEED],(unsigned)p->care[CHAT],(unsigned)p->care[PLAY],
  (unsigned)p->care[NAP],(unsigned)p->integrity,(unsigned)p->hits);
}
static const char *needs(ObserverSession *s,const Game *g){
 int hint=game_care_hint(g);
 s->focus=FOCUS_NEEDS;
 if(s->speaker==OBSERVER_BABY){
  if(hint==NAP)return say(s,"MY EYES FEEL HEAVY.\nPLEASE LET ME NAP.\nWARDEN: ENERGY %u, CHAOS %u.",
                          (unsigned)g->energy,(unsigned)g->chaos);
  if(hint==FEED)return say(s,"MY TUMMY IS EMPTY.\nMAY I HAVE A BOTTLE?\nWARDEN: HUNGER %u.",(unsigned)g->hunger);
  if(hint==CHAT)return say(s,"STAY AND TALK TO ME.\nIT'S QUIET WHEN YOU GO.\nWARDEN: MOOD %u.",(unsigned)g->mood);
  if(g->corruption>=60u)return say(s,"THE NOISE INSIDE ME IS LOUD.\nMOONLIGHT. LAMP OFF. CRIB.\nCAN WE TRY THAT?");
  return say(s,"I'M ALL RIGHT FOR NOW.\nYOU CAN STAY HERE WITH ME.\nWARDEN: H%u M%u E%u C%u.",
              (unsigned)g->hunger,(unsigned)g->mood,(unsigned)g->energy,(unsigned)g->chaos);
 }
 if(hint==NAP)return say(s,"ENERGY %u. CHAOS %u.\nA NAP IS THE CURRENT PRIORITY. USE NAP; TALK ALONE DOES NOT REST THE BABY.",
                          (unsigned)g->energy,(unsigned)g->chaos);
 if(hint==FEED)return say(s,"HUNGER %u.\nA BOTTLE IS THE CURRENT PRIORITY. USE FEED TO HELP THE BABY.",(unsigned)g->hunger);
 if(hint==CHAT)return say(s,"MOOD %u.\nTHE BABY NEEDS COMPANY. USE THE NURSERY CHAT CARE ACTION.",(unsigned)g->mood);
 if(g->corruption>=60u)return say(s,"CORRUPTION %u.\nTRY QUIET CRIB REST: MOONLIGHT, LAMP OFF, THEN CRIB.",(unsigned)g->corruption);
 return say(s,"NO URGENT CARE REQUEST.\nHUNGER %u, MOOD %u, ENERGY %u, CHAOS %u. THE NURSERY CONTROLS STILL PROVIDE CARE.",
              (unsigned)g->hunger,(unsigned)g->mood,(unsigned)g->energy,(unsigned)g->chaos);
}
/* A relationship question is grounded in OBSERVED interaction history.
 * No invented trust percentage, hidden meter or extra persistent bytes.
 * Needs still take priority for the baby. The monitor distinguishes
 * caregiving from guaranteed safety as the shelter mystery escalates. */
static const char *bond(const ObserverProfile *p,ObserverSession *s,
                         const Game *g){
 unsigned stage=observer_stage(p);
 int hint=game_care_hint(g);
 uint32_t total=(uint32_t)p->care[FEED]+(uint32_t)p->care[CHAT]+
                (uint32_t)p->care[PLAY]+(uint32_t)p->care[NAP];
 s->focus=FOCUS_BOND;
 if(s->speaker==OBSERVER_BABY){
  if(hint>=0||g->corruption>=60u)return needs(s,g);
  if(!total)return say(s,"WE JUST MET. CAN WE TALK OR PLAY? I'M STILL LEARNING YOUR VOICE.");
  if(stage>=OBSERVER_HOSTILE)
   return say(s,"WE TALKED %u TIMES AND PLAYED %u TIMES. I KNOW YOUR ROUTINE. NOW I WATCH YOUR HANDS.",
              (unsigned)p->care[CHAT],(unsigned)p->care[PLAY]);
  if(p->care[CHAT]&&p->care[PLAY])
   return say(s,"WE TALKED %u TIMES AND PLAYED %u TIMES. I KNOW HOW YOU SOUND. WILL YOU STAY?",
              (unsigned)p->care[CHAT],(unsigned)p->care[PLAY]);
  if(p->care[PLAY])
   return say(s,"WE PLAYED %u TIMES. I RECOGNIZE YOUR HANDS. CAN WE PLAY AGAIN?",
              (unsigned)p->care[PLAY]);
  return say(s,"YOU FED ME %u TIMES AND LET ME REST %u TIMES. I'M LEARNING YOUR ROUTINE.",
             (unsigned)p->care[FEED],(unsigned)p->care[NAP]);
 }
 if(!total)return say(s,"NO CARE INTERACTIONS RECORDED. TRUST IS NOT A MEASURED METER. TRY FEED, CHAT, PLAY OR NAP.");
 if(stage>=OBSERVER_HOSTILE)
  return say(s,"CARE F%u C%u P%u N%u. STAGE %s. BRACER %u. CARE IS RECORDED; SAFETY IS NOT GUARANTEED.",
             (unsigned)p->care[FEED],(unsigned)p->care[CHAT],
             (unsigned)p->care[PLAY],(unsigned)p->care[NAP],
             observer_stage_name(stage),(unsigned)p->integrity);
 return say(s,"CARE F%u C%u P%u N%u. STAGE %s. TRUST IS NOT A NUMBER. WARDEN RECORDS YOUR ROUTINE, NOT YOUR FEELINGS.",
            (unsigned)p->care[FEED],(unsigned)p->care[CHAT],
            (unsigned)p->care[PLAY],(unsigned)p->care[NAP],
            observer_stage_name(stage));
}

static const char *memory(const ObserverProfile *p,ObserverSession *s){
 const char *kind;
 s->focus=FOCUS_MEMORY;
 if(s->speaker==OBSERVER_BABY){
  switch(p->last_event){
  case OBSERVER_EVENT_FEED:return say(s,"YOU GAVE ME A BOTTLE ON DAY %lu.\nWARDEN: HUNGER -%u. FEEDS %u.",
   (unsigned long)p->last_event_day,(unsigned)p->last_effect,(unsigned)p->care[FEED]);
  case OBSERVER_EVENT_CHAT:return say(s,"YOU TALKED TO ME ON DAY %lu.\nWARDEN: MOOD +%u. CHATS %u.",
   (unsigned long)p->last_event_day,(unsigned)p->last_effect,(unsigned)p->care[CHAT]);
  case OBSERVER_EVENT_PLAY:return say(s,"WE PLAYED ON DAY %lu.\nI REMEMBER PLAYING.\nWARDEN: ENERGY -%u. PLAYS %u.",
   (unsigned long)p->last_event_day,(unsigned)p->last_effect,(unsigned)p->care[PLAY]);
  case OBSERVER_EVENT_NAP:return say(s,"YOU LET ME NAP ON DAY %lu.\nWARDEN: ENERGY +%u. NAPS %u.",
   (unsigned long)p->last_event_day,(unsigned)p->last_effect,(unsigned)p->care[NAP]);
  case OBSERVER_EVENT_ARCADE_WIN:
   return say(s,"WE WON %s! I REMEMBER THAT GAME. CAN WE TRY ANOTHER CABINET?",game_names[p->last_detail<MBAI_GAMES?p->last_detail:0u]);
  case OBSERVER_EVENT_ARCADE_LOSS:
   return say(s,"WE LOST %s, BUT WE CAN TRY AGAIN. WILL YOU HELP ME?",game_names[p->last_detail<MBAI_GAMES?p->last_detail:0u]);
  case OBSERVER_EVENT_QUIET_REST:return say(s,"I REMEMBER THE QUIET CRIB. THE DARK ROOM FELT CALMER. THANK YOU FOR STAYING.");
  case OBSERVER_EVENT_BLOCK:return say(s,"YOU STOPPED MY WINDUP. WARDEN COUNTED %u BLOCKS. YOUR BRACER IS AT %u.",(unsigned)p->blocks,(unsigned)p->integrity);
  case OBSERVER_EVENT_REPAIR:return say(s,"WARDEN FIXED YOUR BRACER. I REMEMBER THAT IT WAS DAMAGED. IT IS AT %u NOW.",(unsigned)p->integrity);
  case OBSERVER_EVENT_REVEAL:return say(s,"I REMEMBER THE SHELL OPENING. WARDEN SAW THE EGG TOO. ARE YOU STILL GOING TO STAY?");
  case OBSERVER_EVENT_NONE:return say(s,"I DON'T HAVE A MEMORY TO TELL YET. STAY A LITTLE WHILE. WARDEN IS WATCHING.");
  case OBSERVER_EVENT_HIT:return say(s,"THAT ONE REACHED YOUR BRACER.\nWARDEN: %u DAMAGE ON DAY %lu.\nYOUR INTEGRITY IS %u.",
   (unsigned)p->last_effect,(unsigned long)p->last_event_day,(unsigned)p->integrity);
  default:break;
  }
 }
 switch(p->last_event){
 case OBSERVER_EVENT_FEED:
  return say(s,"DAY %lu: YOU FED THE BABY. HUNGER FELL BY %u. TOTAL FEEDS OBSERVED: %u.",
   (unsigned long)p->last_event_day,(unsigned)p->last_effect,(unsigned)p->care[FEED]);
 case OBSERVER_EVENT_CHAT:
  return say(s,"DAY %lu: YOU USED CHAT. MOOD ROSE BY %u. TOTAL CHATS OBSERVED: %u.",
   (unsigned long)p->last_event_day,(unsigned)p->last_effect,(unsigned)p->care[CHAT]);
 case OBSERVER_EVENT_PLAY:
  return say(s,"DAY %lu: YOU PLAYED WITH THE BABY. IT USED %u ENERGY. TOTAL PLAY EVENTS: %u.",
   (unsigned long)p->last_event_day,(unsigned)p->last_effect,(unsigned)p->care[PLAY]);
 case OBSERVER_EVENT_NAP:
  return say(s,"DAY %lu: THE BABY NAPPED. ENERGY ROSE BY %u. TOTAL NAPS OBSERVED: %u.",
   (unsigned long)p->last_event_day,(unsigned)p->last_effect,(unsigned)p->care[NAP]);
 case OBSERVER_EVENT_ARCADE_WIN:case OBSERVER_EVENT_ARCADE_LOSS:
  return say(s,"DAY %lu: YOU %s %s. WARDEN HAS THE RESULT IN ITS LOG.",
   (unsigned long)p->last_event_day,p->last_event==OBSERVER_EVENT_ARCADE_WIN?"WON":"LOST",
   game_names[p->last_detail<MBAI_GAMES?p->last_detail:0u]);
 case OBSERVER_EVENT_BLOCK:
  return say(s,"DAY %lu: YOU BLOCKED AN ATTACK%s. TOTAL BLOCKS: %u. INTEGRITY %u.",
   (unsigned long)p->last_event_day,p->last_detail==OBSERVER_EVENT_KICK?" WITH A KICK":
   p->last_detail==OBSERVER_EVENT_SLAP?" WITH A SLAP":" WITH GUARD",
   (unsigned)p->blocks,(unsigned)p->integrity);
 case OBSERVER_EVENT_HIT:
  return say(s,"DAY %lu: A STRIKE LANDED FOR %u INTEGRITY. CONFIRMED HITS: %u. INTEGRITY NOW %u.",
   (unsigned long)p->last_event_day,(unsigned)p->last_effect,(unsigned)p->hits,(unsigned)p->integrity);
 case OBSERVER_EVENT_SLAP:kind="A TOYBOX SLAP THAT CONNECTED";break;
 case OBSERVER_EVENT_KICK:kind="A TOYBOX KICK THAT CONNECTED";break;
 case OBSERVER_EVENT_QUIET_REST:kind="SUCCESSFUL QUIET CRIB REST";break;
 case OBSERVER_EVENT_DAY:kind="A DAY ADVANCE";break;
 case OBSERVER_EVENT_WINDUP:kind="AN ATTACK WINDUP";break;
 case OBSERVER_EVENT_REPAIR:kind="A BRACER LINK REPAIR";break;
 case OBSERVER_EVENT_TEACH:kind="A PHRASE YOU TAUGHT";break;
 case OBSERVER_EVENT_REVEAL:kind="THE SHELL OPENING AND THE ALIEN EGG";break;
 default:return say(s,"NO EVENT HAS BEEN RECORDED IN THIS NURSERY LOG. THE ARCHIVE IS WAITING FOR ITS FIRST ENTRY.");
 }
 return say(s,"DAY %lu: WARDEN RECORDED %s. THE ENTRY IS STILL IN THE NURSERY LOG.",
             (unsigned long)p->last_event_day,kind);
}
static const char *threat(const ObserverProfile *p,ObserverSession *s){
 s->focus=FOCUS_THREAT;
 if(s->speaker==OBSERVER_BABY)
  return say(s,"WARDEN WATCHES MY HANDS.\n%u WINDUPS, %u HITS, %u BLOCKS.\nYOUR BRACER: %u INTEGRITY.",
   (unsigned)p->attacks,(unsigned)p->hits,(unsigned)p->blocks,(unsigned)p->integrity);
 return say(s,"STAGE %s. WINDUPS %u.\nHITS %u. BLOCKS %u.\nINTEGRITY %u. GUARD THE WINDUP.\nWARDEN CAN REPAIR YOUR BRACER.",
  observer_stage_name(observer_stage(p)),(unsigned)p->attacks,(unsigned)p->hits,
  (unsigned)p->blocks,(unsigned)p->integrity);
}
static const char *world(ObserverSession *s,const char *input){
 s->focus=has(input,"BAN")||has(input,"BUREAU")||has(input,"WHY")?FOCUS_BAN:
          has(input,"CLINIC")||has(input,"FOUND")?FOCUS_CLINIC:FOCUS_WORLD;
 if(s->speaker==OBSERVER_BABY){
  if(has(input,"BAN")||has(input,"BUREAU")||has(input,"WHY"))
   return say(s,"WARDEN SAYS THE BUREAU CALLS IT THE CRADLE BAN. IT SAYS THE REASON IS CLASSIFIED. I JUST KNOW YOU LET ME STAY.");
  if(has(input,"CLINIC")||has(input,"FOUND"))
   return say(s,"WARDEN SAYS YOU FOUND ME OUTSIDE THE SHUTTERED CLINIC. NOW I'M HERE WITH YOU. DON'T TELL THE BUREAU.");
  return say(s,"WARDEN CALLS THIS THE LAST NURSERY. IT DOESN'T KNOW HOW MANY OF US ARE LEFT. YOU'RE STILL HERE, THOUGH.");
 }
 if(has(input,"BAN")||has(input,"BUREAU")||has(input,"WHY"))
  return say(s,"THE CONTINUITY BUREAU ENFORCES THE CRADLE BAN. ITS REASON IS CLASSIFIED. I CANNOT VERIFY THE BUREAU'S CLAIMS.");
 if(has(input,"CLINIC")||has(input,"FOUND"))
  return say(s,"YOU FOUND THIS UNREGISTERED BABY OUTSIDE A SHUTTERED CLINIC. THIS HIDDEN NURSERY IS ITS SHELTER.");
 return say(s,"THIS IS THE LAST NURSERY. YOU HIDE A FORBIDDEN BABY FROM THE CONTINUITY BUREAU. MY RECORDS DO NOT SHOW HOW MANY SURVIVE.");
}
static const char *shell(const ObserverProfile *p,ObserverSession *s,const Game *g){
 s->focus=FOCUS_SHELL;
 if(g->ending_mask&(1u<<END_EGG))return say(s,"THE SHELL OPENED. THE EGG WAS REAL. I HAVE THE RECORD. THE NURSERY IS STILL HERE, AND SO IS THE BABY.");
 return say(s,observer_stage(p)>=OBSERVER_HOSTILE?
   "I AM WATCHING THE SEAMS OF ITS TOY-LIKE SHELL. SOMETHING IS HIDDEN INSIDE. NO CONFIRMED OPENING IS IN THIS ARCHIVE.":
   "THE BABY HAS A TOY-LIKE SHELL. MY ARCHIVE CANNOT EXPLAIN WHAT IS INSIDE. NO CONFIRMED OPENING IS IN THE LOG.");
}
static const char *toybox(const ObserverProfile *p,ObserverSession *s){
 s->focus=FOCUS_TOYBOX;
 return say(s,"CONNECTED IN TOYBOX: %u SLAPS, %u KICKS. A TIMED COUNTER CAN STOP ITS WINDUP. GUARD PROTECTS YOUR BRACER TOO.",
              (unsigned)p->slaps,(unsigned)p->kicks);
}
/* Follow-up details stay anchored to the previously selected subject. */
static const char *world_more(ObserverSession *s){
 if(s->focus==FOCUS_CLINIC)
  return say(s,s->speaker==OBSERVER_BABY?
    "I DON'T REMEMBER BEFORE THE CLINIC. YOU BROUGHT ME HERE. WHAT WILL WE DO TODAY?":
    "THE CLINIC WAS SHUTTERED. THE ARCHIVE HAS NO VERIFIED RECORD OF THE BABY BEFORE YOU FOUND IT.");
 if(s->focus==FOCUS_BAN)
  return say(s,s->speaker==OBSERVER_BABY?
    "I DON'T KNOW WHY THEY BANNED BABIES. WARDEN SAYS I'M NOT REGISTERED. IS IT SAFE TO STAY HERE?":
    "THE BAN IS ENFORCED BY THE BUREAU. ITS JUSTIFICATION IS CLASSIFIED; I CANNOT CONFIRM THE CLAIMS ABOUT INFANTS.");
 return say(s,s->speaker==OBSERVER_BABY?
   "THIS ROOM IS OUR HIDING PLACE. WARDEN WATCHES OVER IT. CAN WE STAY TOGETHER?":
   "THIS NURSERY IS THE SHELTER. I CAN DESCRIBE THE CLINIC OR CRADLE BAN, BUT I HAVE NO LIVE RECORDS OF THE OUTSIDE WORLD.");
}
static const char *phrases(const ObserverProfile *p,ObserverSession *s){
 unsigned i,n=0;
 size_t used=0;
 s->focus=FOCUS_TEACH;
 if(!p->phrase_count)return say(s,"NO TAUGHT PHRASES YET. TYPE TEACH FOLLOWED BY UP TO 24 CHARACTERS. REPEAT A PHRASE TO REINFORCE IT.");
 s->reply[0]=0;
 for(i=0;i<OBSERVER_PHRASES;i++)if(p->phrases[i].confidence){
  int written=snprintf(s->reply+used,145u-used,"%s%u: %s",n?"\n":"",i+1u,p->phrases[i].text);
  if(written<0||(size_t)written>=145u-used)break;
  used+=(size_t)written;n++;
 }
 return s->reply;
}
static int number(const char **at,int32_t *out){
 const char *p=*at;
 int sign=1;
 int32_t value=0;
 unsigned digits=0;
 while(*p==' ')p++;
 if(*p=='+'||*p=='-'){if(*p=='-')sign=-1;p++;}
 while(*p>='0'&&*p<='9'){
  if(++digits>4u)return 0;
  value=value*10+(*p-'0');p++;
 }
 if(!digits)return 0;
 *out=value*sign;*at=p;return 1;
}
static const char *calculate(ObserverSession *s,const char *input){
 int32_t a,b,result;
 const char *p=input;
 char op;
 s->focus=FOCUS_CALC;
 if(strncmp(p,"CALCULATE ",10u)==0)p+=10;
 else if(strncmp(p,"CALC ",5u)==0)p+=5;
 else if(strncmp(p,"WHAT IS ",8u)==0)p+=8;
 if(!number(&p,&a))return say(s,"USE CALC 12+7. ONE INTEGER OPERATION: + - * /. EACH NUMBER MUST BE BETWEEN -9999 AND 9999.");
 while(*p==' ')p++;
 op=*p;if(op)p++;
 if((op!='+'&&op!='-'&&op!='*'&&op!='/')||!number(&p,&b))
  return say(s,"I CAN CALCULATE ONE INTEGER OPERATION USING + - * /. TRY CALC 12+7.");
 while(*p==' ')p++;
 if(*p=='?')p++;
 if(*p)return say(s,"ONLY ONE INTEGER OPERATION IS SUPPORTED. I DID NOT EVALUATE THAT EXPRESSION.");
 if(op=='/'&&!b)return say(s,"DIVISION BY ZERO HAS NO DEFINED RESULT.");
 result=op=='+'?a+b:op=='-'?a-b:op=='*'?a*b:a/b;
 if(op=='/'&&a%b)return say(s,"%ld / %ld = %ld, REMAINDER %ld. INTEGER DIVISION.",
                            (long)a,(long)b,(long)result,(long)(a%b));
 return say(s,"%ld %c %ld = %ld.",(long)a,op,(long)b,(long)result);
}
static const char *fiction(ObserverProfile *p,ObserverSession *s,const Game *g,
                           unsigned speaker,unsigned topic){
 unsigned used;
 const char *prefix;
 size_t length;
 if(speaker==OBSERVER_BABY&&(game_care_hint(g)>=0||g->corruption>=60u))
  return needs(s,g);
 /* Every eligible fiction request advances exactly once, including a
  * learned quote. Otherwise a quote-producing seed could stay locked on
  * that same phrase until the player changed to another topic. */
 s->variation=s->variation*1664525u+1013904223u;
 if(speaker==OBSERVER_BABY&&topic==NARRATIVE_PLAY&&p->care[PLAY]&&
    (s->variation%3u)==0u&&learned_quote(p,s,3u))return s->reply;
 prefix=topic==NARRATIVE_DREAM?"IMAGINED DREAM: ":
        topic==NARRATIVE_PLAY?"MAKE-BELIEVE: ":"";
 length=strlen(prefix);memcpy(s->reply,prefix,length);
 used=narrative_lm_generate(s->reply+length,145u-length,speaker,topic,
                             observer_stage(p),s->variation);
 if(!used)return say(s,"I LOST MY WORDS. ASK STATUS, NEEDS, MEMORY, WORLD OR HELP.");
 return s->reply;
}
const char *observer_answer(ObserverProfile *p,ObserverSession *s,const Game *g,
                            unsigned speaker,const char *input){
 char clean[160];
 const char *at;
 unsigned topic;
 if(!p||!s||!g)return "WARDEN UNAVAILABLE.";
 if(speaker>OBSERVER_BABY)speaker=OBSERVER_MONITOR;
 if(s->speaker!=speaker)s->focus=FOCUS_STATUS;
 s->speaker=(uint8_t)speaker;
 if(!normalize(clean,sizeof(clean),input))
  return say(s,"USE A SHORT PRINTABLE ASCII QUESTION. TRY HELP.");
 if(!clean[0])return observer_report(p,s,g);
 if(strncmp(clean,"TEACH ",6u)==0||strncmp(clean,"TEACH:",6u)==0){
  int result;
  if(speaker!=OBSERVER_BABY)return say(s,"SELECT BABY TO TEACH A PHRASE. I WILL KEEP ITS LESSONS IN THE NURSERY LOG.");
  result=observer_teach(p,clean+6);
  s->focus=FOCUS_TEACH;
  if(!result)return say(s,"TEACH 1 TO 24 LETTERS, DIGITS OR KEYBOARD PUNCTUATION. SPACES ARE ALLOWED. I CANNOT READ THAT LESSON.");
  if(result==3)return say(s,"ALL FOUR LESSONS ARE FULL. I KEPT THEM ALL. ASK PHRASES TO REVIEW THEM OR FORGET 1 TO FREE A SLOT.");
  record(p,g,OBSERVER_EVENT_TEACH,0u,0u);
  return say(s,result==2?"PHRASE REINFORCED. ASK SAY TO HEAR IT. REPEATED TEACHING CAN MAKE THE BABY QUOTE IT DURING PLAY.":
   "PHRASE LEARNED. ASK SAY TO HEAR IT. TEACH IT AGAIN TO REINFORCE IT.");
 }
 if(strcmp(clean,"FORGET")==0||strncmp(clean,"FORGET ",7u)==0){
  unsigned slot;
  s->focus=FOCUS_TEACH;
  if(speaker!=OBSERVER_BABY)return say(s,"SELECT BABY TO MANAGE ITS TAUGHT PHRASES.");
  if(strlen(clean)<8u||clean[7]<'1'||clean[7]>'4')
   return say(s,"USE FORGET 1, 2, 3 OR 4. ASK PHRASES TO SEE THE SLOTS.");
  slot=(unsigned)(clean[7]-'1');
  if(clean[8]&&strcmp(clean+8," YES")!=0)
   return say(s,"USE FORGET 1 TO REVIEW, OR FORGET 1 YES TO CONFIRM. REPLACE 1 WITH THE SLOT NUMBER.");
  if(!p->phrases[slot].confidence)return say(s,"LESSON %u IS ALREADY EMPTY.",slot+1u);
  if(!clean[8])return say(s,"FORGET LESSON %u: %s? TYPE FORGET %u YES TO REMOVE ONLY THIS PHRASE.",slot+1u,p->phrases[slot].text,slot+1u);
  if(observer_forget_phrase(p,slot)){
   s->echo_ready=0;
   return say(s,"LESSON %u FORGOTTEN. THE OTHER LESSONS ARE SAFE. YOU CAN TEACH A NEW PHRASE.",slot+1u);
  }
  return say(s,"LESSON NOT CHANGED.");
 }
 if(strncmp(clean,"LESSON ",7u)==0&&strlen(clean)==8u&&clean[7]>='1'&&clean[7]<='4'){
  unsigned slot=(unsigned)(clean[7]-'1');
  s->focus=FOCUS_TEACH;
  if(!p->phrases[slot].confidence)return say(s,"LESSON %u IS EMPTY.",slot+1u);
  return say(s,"LESSON %u: %s. PRACTICE %u/15. %s",slot+1u,p->phrases[slot].text,(unsigned)p->phrases[slot].confidence,
   p->phrases[slot].confidence>=3u?"I MAY SAY IT DURING HEALTHY PLAY.":"TEACH IT AGAIN TO HELP ME REMEMBER.");
 }
 if(strcmp(clean,"REPAIR")==0||strcmp(clean,"REPAIR SHELTER")==0||strcmp(clean,"REPAIR WARDEN")==0){
  if(speaker!=OBSERVER_MONITOR)return say(s,"WARDEN HANDLES REPAIRS. SELECT WARDEN AND TYPE REPAIR.");
  if(p->integrity==100u&&!s->windup)return say(s,"BRACER ALREADY AT 100 INTEGRITY.");
  p->integrity=100;p->repairs=sat_add(p->repairs,1u);
  s->windup=0;s->cooldown=OBSERVER_COOLDOWN_TICKS;
  s->notice=NOTICE_REPAIR;s->notice_ticks=90u;
  record(p,g,OBSERVER_EVENT_REPAIR,0u,0u);
  return say(s,"BRACER REPAIRED. INTEGRITY 100. I KEPT THE NURSERY LOG AND YOUR BABY'S RECORDS.");
 }
 if(strcmp(clean,"SAY")==0||strcmp(clean,"REPEAT")==0||
    strcmp(clean,"SAY WHAT I TAUGHT")==0||strcmp(clean,"SAY WHAT I TAUGHT YOU")==0||
    strcmp(clean,"SAY WHAT YOU TAUGHT ME")==0){
  s->focus=FOCUS_TEACH;
  if(learned_quote(p,s,1u))return s->reply;
  return phrases(p,s);
 }
 if(strncmp(clean,"SAY ",4u)==0){
  unsigned i;
  s->focus=FOCUS_TEACH;
  at=clean+4;
  if(at[0]>='1'&&at[0]<='4'&&!at[1]){
   i=(unsigned)(at[0]-'1');
   if(p->phrases[i].confidence)return say(s,"YOU TAUGHT ME: %s",p->phrases[i].text);
  }else for(i=0;i<OBSERVER_PHRASES;i++)
   if(p->phrases[i].confidence&&strcmp(at,p->phrases[i].text)==0)
    return say(s,"YOU TAUGHT ME: %s",p->phrases[i].text);
  return say(s,"THAT PHRASE IS NOT IN A TAUGHT SLOT. TRY SAY OR WHAT DID I TEACH.");
 }
 if(has(clean,"TEACH")||has(clean,"PHRASES"))return phrases(p,s);
 if(has(clean,"HELP"))return say(s,"ASK STATUS, NEEDS, MEMORY, WORLD, ATTACKS, BOND, PLAY OR DREAM. TRY TEACH HELLO, SAY, PHRASES, FORGET 1, CALC 2+3.");
 at=clean;
 if(strncmp(at,"WHAT IS ",8u)==0)at+=8;
 if(has(clean,"CALC")||has(clean,"CALCULATE")||
    (*at>='0'&&*at<='9')||((*at=='-'||*at=='+')&&at[1]>='0'&&at[1]<='9'))
  return calculate(s,clean);
 if(strcmp(clean,"MORE")==0||strcmp(clean,"MORE?")==0||strcmp(clean,"TELL ME MORE")==0||strcmp(clean,"TELL ME MORE?")==0||strcmp(clean,"WHY?")==0||strcmp(clean,"WHY")==0){
  switch(s->focus){
  case FOCUS_NEEDS:return needs(s,g);
  case FOCUS_SELF:
   if(speaker==OBSERVER_BABY)return fiction(p,s,g,speaker,NARRATIVE_SELF);
   return say(s,"I MONITOR THE NURSERY AND KEEP OBSERVED CARE RECORDS. ASK MEMORY OR WORLD TO LEARN MORE.");
  case FOCUS_BOND:return bond(p,s,g);
  case FOCUS_MEMORY:return memory(p,s);
  case FOCUS_WORLD:case FOCUS_BAN:case FOCUS_CLINIC:return world_more(s);
  case FOCUS_SHELL:
   return say(s,(g->ending_mask&(1u<<END_EGG))?
    "THE OPENING IS A CONFIRMED EVENT. ITS CAUSE AND THE EGG'S ORIGIN REMAIN UNKNOWN.":
    "THERE IS NO VERIFIED SHELL OPENING YET. WARDEN CANNOT TELL YOU WHAT IS INSIDE FROM THE SEAMS ALONE.");
  case FOCUS_TOYBOX:return toybox(p,s);
  case FOCUS_THREAT:return threat(p,s);
  case FOCUS_TEACH:return phrases(p,s);
  case FOCUS_PLAY:return fiction(p,s,g,speaker,NARRATIVE_PLAY);
  case FOCUS_DREAM:return fiction(p,s,g,speaker,NARRATIVE_DREAM);
  default:return observer_report(p,s,g);
  }
 }
 /* Acknowledgments do not discard the subject of the conversation. */
 if(strcmp(clean,"THANKS")==0||strcmp(clean,"THANKS!")==0||strcmp(clean,"THANK YOU")==0||strcmp(clean,"THANK YOU!")==0){
  if(speaker==OBSERVER_BABY){
   if(game_care_hint(g)>=0||g->corruption>=60u)return needs(s,g);
   return say(s,observer_stage(p)>=OBSERVER_HOSTILE?"YOU'RE WELCOME. I REMEMBER HOW YOU TALK TO ME.":"YOU'RE WELCOME! I LIKE WHEN WE TALK.");
  }
  return say(s,"YOU'RE WELCOME. I'M STILL HERE IF YOU HAVE ANOTHER QUESTION.");
 }
 if(speaker==OBSERVER_BABY&&has(clean,"HOW ARE YOU"))return needs(s,g);
 if(has(clean,"STATUS")||has(clean,"STATS")||has(clean,"REPORT")||has(clean,"HOW ARE YOU"))
  return observer_report(p,s,g);
 if(has(clean,"NEED")||has(clean,"NEEDS")||has(clean,"HUNGRY")||has(clean,"TIRED")||has(clean,"HUNGER")||has(clean,"ENERGY"))
  return needs(s,g);
 if(has(clean,"BOND")||has(clean,"TRUST")||has(clean,"FRIEND")||has(clean,"FRIENDS")||has(clean,"CARE PATTERN")||has(clean,"DO YOU LIKE ME"))
  return bond(p,s,g);
 if(has(clean,"MEMORY")||has(clean,"REMEMBER")||has(clean,"HISTORY")||has(clean,"LAST EVENT"))
  return memory(p,s);
 if(has(clean,"ATTACK")||has(clean,"ATTACKS")||has(clean,"HITS")||has(clean,"INTEGRITY")||has(clean,"GUARD")||has(clean,"EVIL")||has(clean,"THREAT"))
  return threat(p,s);
 if(has(clean,"SLAP")||has(clean,"KICK")||has(clean,"TOYBOX"))return toybox(p,s);
 if(has(clean,"EGG")||has(clean,"SHELL"))return shell(p,s,g);
 if(has(clean,"WORLD")||has(clean,"OUTSIDE")||has(clean,"BAN")||has(clean,"BUREAU")||has(clean,"CLINIC")||has(clean,"FOUND")||has(clean,"FORBIDDEN")||has(clean,"SHELTER")||has(clean,"WHERE AM I"))
  return world(s,clean);
 if(has(clean,"WHO AM I"))return say(s,"YOU ARE THE CARETAKER HIDING AN UNREGISTERED BABY IN THIS NURSERY. I DO NOT KNOW YOUR NAME.");
 if(has(clean,"WHO ARE YOU")||has(clean,"WHAT ARE YOU")||has(clean,"SELF")||has(clean,"WARDEN")){
  s->focus=FOCUS_SELF;
  if(speaker==OBSERVER_MONITOR)return say(s,"I AM WARDEN, A SALVAGED NURSERY COMPUTER. I MONITOR THIS HIDDEN ROOM. MY ARCHIVE IS LIMITED; I DO NOT KNOW EVERYTHING.");
  return fiction(p,s,g,speaker,NARRATIVE_SELF);
 }
 if(has(clean,"DREAM")||has(clean,"DREAMS")||has(clean,"STORY")){
  s->focus=FOCUS_DREAM;topic=NARRATIVE_DREAM;
 }else if(has(clean,"PLAY")||has(clean,"JOKE")||has(clean,"TEDDY")){
  s->focus=FOCUS_PLAY;topic=NARRATIVE_PLAY;
 }else if(has(clean,"HELLO")||has(clean,"HI")||has(clean,"HEY")){
  s->focus=FOCUS_SELF;topic=NARRATIVE_GREETING;
 }else return say(s,speaker==OBSERVER_BABY?
   "I DON'T KNOW THAT ONE. WARDEN MIGHT KNOW. TRY NEEDS, MEMORY, PLAY OR DREAM.":
   "I DO NOT UNDERSTAND THAT QUESTION YET. ASK HELP FOR MY TOPICS. MY ARCHIVE HAS NO ANSWER.");
 return fiction(p,s,g,speaker,topic);
}
