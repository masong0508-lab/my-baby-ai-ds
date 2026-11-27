#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "conversation.h"
#include "dialogue.h"
typedef struct {
 Game game;
 ConversationProfile profile;
 ConversationSession session;
 uint16_t displayed;
} Fixture;
static unsigned seen[CONVERSATION_TOPIC_COUNT*9u];
static const char *phase;
static void reset(Fixture *f,uint32_t seed){
 game_init(&f->game);
 conversation_profile_reset(&f->profile);
 conversation_session_reset(&f->session,f->game.days,seed);
 f->displayed=UINT16_MAX;
}
static void check_reply(Fixture *f,const ConversationReply *reply){
 assert(reply&&reply->id<conversation_catalog_count());
 assert(reply==conversation_catalog_reply(reply->id));
 assert(reply->topic==reply->id/9u&&reply->style==(reply->id%9u)/3u);
 if(reply->id==f->displayed)
  fprintf(stderr,"repeated reply %u in %s prompt %u day %lu\n",reply->id,
          phase,f->session.last_prompt,(unsigned long)f->game.days);
 assert(reply->id!=f->displayed);
 f->displayed=reply->id;
 seen[reply->id]++;
}
static const ConversationReply *ask(Fixture *f,ConversationPrompt prompt){
 Game before=f->game;
 const ConversationReply *reply=conversation_respond(&f->profile,&f->session,
                                                     &f->game,prompt);
 assert(memcmp(&before,&f->game,sizeof(before))==0);
 check_reply(f,reply);
 return reply;
}
static const ConversationReply *care(Fixture *f,Care action,int show){
 Game before=f->game,reference=f->game;
 const ConversationReply *reply=NULL;
 (void)game_care(&f->game,action);
 (void)game_care(&reference,action);
 conversation_record_care(&f->session,&before,&f->game,action);
 assert(memcmp(&reference,&f->game,sizeof(reference))==0);
 if(show){
  reply=conversation_care_response(&f->profile,&f->session,&f->game,action);
  assert(memcmp(&reference,&f->game,sizeof(reference))==0);
  if(action==CHAT)assert(!reply);
  else check_reply(f,reply);
 }
 return reply;
}
static void arcade_result(Fixture *f,int won){
 Arcade a;
 game_arcade_start(&f->game,&a,5);
 while(a.active){
  int dir=game_arcade_expected(&a);
  game_arcade_input(&f->game,&a,won?dir:(dir+1)%3);
 }
 assert(a.finished==(won?1:2));
 Game before=f->game;
 conversation_record_event(&f->session,won?CONVERSATION_EVENT_ARCADE_WIN:
                              CONVERSATION_EVENT_ARCADE_LOSS,a.id,f->game.days);
 assert(memcmp(&before,&f->game,sizeof(before))==0);
}
static void prefer_style(ConversationProfile *profile,unsigned style){
 conversation_profile_reset(profile);
 for(unsigned i=0;i<CONVERSATION_FEATURE_COUNT;i++)
  profile->adapter[style][i]=CONVERSATION_ADAPTER_LIMIT;
 assert(conversation_profile_valid(profile));
}
static void catalog_and_encoder(void){
 Game g;int8_t a[DIALOGUE_HIDDEN_COUNT],b[DIALOGUE_HIDDEN_COUNT];
 assert(sizeof(ConversationProfile)==40u&&sizeof(ConversationSession)<=128u);
 assert(conversation_catalog_count()>=120u);
 assert(!conversation_catalog_reply(conversation_catalog_count()));
 for(unsigned id=0;id<conversation_catalog_count();id++){
  const ConversationReply *r=conversation_catalog_reply(id);
  unsigned rows=1,width=0;
  assert(r&&r->id==id&&r->text&&strlen(r->text)<=110u);
  for(unsigned i=0;r->text[i];i++){
   unsigned ch=(unsigned char)r->text[i];
   if(ch=='\n'){assert(width>0u&&width<=36u);rows++;width=0;}
   else{
    assert(ch>=32u&&ch<=126u);
    assert(strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 !?.:'-/$+%#*()<>=,",
                  (int)ch));
    width++;
   }
  }
  assert(rows<=2u&&width>0u&&width<=36u); /* every care entry fits too */
  for(unsigned j=0;j<id;j++)
   assert(strcmp(r->text,conversation_catalog_reply(j)->text)!=0);
 }
 game_init(&g);
 for(unsigned i=0;i<101u;i++){
  g.hunger=(uint8_t)i;g.mood=(uint8_t)((i*17u)%101u);
  g.energy=(uint8_t)((i*31u)%101u);g.chaos=(uint8_t)((i*11u)%101u);
  g.corruption=(uint8_t)((i*7u)%101u);
  Game before=g;
  for(unsigned action=0;action<4u;action++){
   dialogue_encode(&g,(Care)action,a);dialogue_encode(&g,(Care)action,b);
   assert(memcmp(a,b,sizeof(a))==0&&memcmp(&g,&before,sizeof(g))==0);
   for(unsigned j=0;j<DIALOGUE_HIDDEN_COUNT;j++)assert(a[j]>=0);
  }
 }
 dialogue_encode(NULL,CHAT,a);
 for(unsigned j=0;j<DIALOGUE_HIDDEN_COUNT;j++)assert(a[j]==0);
 dialogue_encode(&g,CHAT,NULL);
}
static void prepare_topic(Fixture *f,ConversationTopic topic){
 switch(topic){
 case CONVERSATION_FEEL_OK:break;
 case CONVERSATION_FEEL_HUNGRY:f->game.hunger=55;break;
 case CONVERSATION_FEEL_TIRED:f->game.energy=45;break;
 case CONVERSATION_FEEL_PLAYFUL:f->game.mood=90;break;
 case CONVERSATION_FEEL_GLITCH:f->game.corruption=30;break;
 case CONVERSATION_FEEL_CHAOTIC:f->game.chaos=55;break;
 case CONVERSATION_NEED_FOOD:f->game.hunger=85;break;
 case CONVERSATION_NEED_REST:f->game.energy=10;break;
 case CONVERSATION_NEED_COMPANY:f->game.mood=20;break;
 case CONVERSATION_NEED_QUIET:f->game.corruption=75;break;
 case CONVERSATION_NEED_NONE:break;
 case CONVERSATION_MEMORY_FEED:f->game.hunger=60;(void)care(f,FEED,0);break;
 case CONVERSATION_MEMORY_CHAT:(void)care(f,CHAT,0);break;
 case CONVERSATION_MEMORY_PLAY:
  f->game.energy=90;f->game.hunger=10;(void)care(f,PLAY,0);break;
 case CONVERSATION_MEMORY_NAP:
  f->game.energy=50;f->game.hunger=20;(void)care(f,NAP,0);break;
 case CONVERSATION_MEMORY_WIN:arcade_result(f,1);break;
 case CONVERSATION_MEMORY_LOSS:arcade_result(f,0);break;
 case CONVERSATION_MEMORY_QUIET:
  f->game.energy=50;f->game.hunger=20;(void)care(f,NAP,0);
  assert(game_room_quiet_rest(&f->game,0,0));
  conversation_record_event(&f->session,CONVERSATION_EVENT_QUIET_REST,0,
                              f->game.days);
  break;
 case CONVERSATION_FULL_FEED:f->game.hunger=0;(void)care(f,FEED,0);break;
 case CONVERSATION_FULL_NAP:
  f->game.energy=100;f->game.hunger=20;(void)care(f,NAP,0);break;
 case CONVERSATION_TIRED_PLAY:
  f->game.energy=60;f->game.hunger=10;(void)care(f,PLAY,0);break;
 case CONVERSATION_REPEAT_CARE:
  for(unsigned i=0;i<3u;i++)(void)care(f,FEED,0);
  break;
 case CONVERSATION_CARE_THANKS:
  f->game.hunger=90;
  assert(ask(f,CONVERSATION_NEED)->topic==CONVERSATION_NEED_FOOD);
  (void)care(f,FEED,0);break;
 default:break;
 }
}
static void all_responses_reachable(void){
 Fixture f;
 for(unsigned topic=0;topic<CONVERSATION_TOPIC_COUNT;topic++)
 for(unsigned style=0;style<CONVERSATION_STYLE_COUNT;style++){
  const ConversationReply *reply;
  reset(&f,17u);prefer_style(&f.profile,style);
  prepare_topic(&f,(ConversationTopic)topic);
  if(topic==CONVERSATION_ACK_LIKE){
   for(unsigned n=0;n<3u;n++){
    (void)ask(&f,CONVERSATION_HOW);
    reply=ask(&f,CONVERSATION_LIKE);assert(reply->topic==topic);
   }
   continue;
  }
  if(topic==CONVERSATION_ACK_RATED){
   (void)ask(&f,CONVERSATION_HOW);(void)ask(&f,CONVERSATION_LIKE);
   for(unsigned n=0;n<3u;n++){
    reply=ask(&f,CONVERSATION_LIKE);assert(reply->topic==topic);
   }
   continue;
  }
  if(topic==CONVERSATION_ASK_FIRST){
   for(unsigned n=0;n<3u;n++){
    reply=ask(&f,CONVERSATION_LIKE);assert(reply->topic==topic);
   }
   continue;
  }
  if(topic>=CONVERSATION_IMAGINE_TEDDY&&topic<=CONVERSATION_IMAGINE_WORLD){
   unsigned seed;
   for(seed=1;seed<100u;seed++){
    reset(&f,seed);prefer_style(&f.profile,style);
    reply=ask(&f,CONVERSATION_ELSE);
    if(reply->topic==topic)break;
   }
   assert(seed<100u);
  }else if(topic==CONVERSATION_NO_MEMORY)
   reply=ask(&f,CONVERSATION_REMEMBER);
  else if(topic>=CONVERSATION_NEED_FOOD&&topic<=CONVERSATION_NEED_NONE)
   reply=ask(&f,CONVERSATION_NEED);
  else if(topic>=CONVERSATION_MEMORY_FEED&&topic<=CONVERSATION_TIRED_PLAY)
   reply=ask(&f,CONVERSATION_REMEMBER);
  else reply=ask(&f,CONVERSATION_HOW);
  assert(reply->topic==topic&&reply->style==style);
  for(unsigned n=0;n<2u;n++){
   reply=ask(&f,CONVERSATION_MORE);
   assert(reply->topic==topic&&reply->style==style);
  }
 }
 for(unsigned id=0;id<conversation_catalog_count();id++){
  if(!seen[id])fprintf(stderr,"unreachable response %u topic %u style %u\n",
                      id,id/9u,(id%9u)/3u);
  assert(seen[id]);
 }
}
static void memory_continuity_and_expiry(void){
 Fixture f;const ConversationReply *reply;
 reset(&f,123);
 f.game.wins=100;f.game.losses=100;
 assert(ask(&f,CONVERSATION_REMEMBER)->topic==CONVERSATION_NO_MEMORY);
 (void)care(&f,FEED,1);
 assert(ask(&f,CONVERSATION_REMEMBER)->topic==CONVERSATION_MEMORY_FEED);
 reply=ask(&f,CONVERSATION_MORE);
 assert(reply->topic==CONVERSATION_MEMORY_FEED);
 uint16_t previous_substantive=reply->id;
 int8_t credited[CONVERSATION_FEATURE_COUNT];
 memcpy(credited,f.session.last_features,sizeof(credited));
 assert(ask(&f,CONVERSATION_LIKE)->topic==CONVERSATION_ACK_LIKE);
 ConversationProfile after_like=f.profile;
 assert(f.session.topic==CONVERSATION_MEMORY_FEED);
 assert(memcmp(credited,f.session.last_features,sizeof(credited))==0);
 assert(ask(&f,CONVERSATION_LIKE)->topic==CONVERSATION_ACK_RATED);
 assert(memcmp(&after_like,&f.profile,sizeof(after_like))==0);
 assert(f.session.topic==CONVERSATION_MEMORY_FEED);
 reply=ask(&f,CONVERSATION_MORE);
 assert(reply->topic==CONVERSATION_MEMORY_FEED&&reply->id!=previous_substantive);
 /* Age the actual fact out by asking follow-up questions; there is no
  * phantom feeding recall once its bounded session history has expired. */
 for(unsigned i=0;i<CONVERSATION_FACT_TTL;i++)reply=ask(&f,CONVERSATION_MORE);
 assert(reply->topic==CONVERSATION_NO_MEMORY&&f.session.fact_count==0);
 assert(ask(&f,CONVERSATION_REMEMBER)->topic==CONVERSATION_NO_MEMORY);
 (void)care(&f,FEED,0);
 ConversationProfile kept=f.profile;
 (void)game_end_day(&f.game);
 f.displayed=UINT16_MAX; /* END DAY displays its own non-conversation notice. */
 assert(ask(&f,CONVERSATION_REMEMBER)->topic==CONVERSATION_NO_MEMORY);
 assert(memcmp(&kept,&f.profile,sizeof(kept))==0);
 (void)care(&f,FEED,0);
 conversation_session_reset(&f.session,f.game.days,f.game.rng);
 f.displayed=UINT16_MAX;
 assert(ask(&f,CONVERSATION_REMEMBER)->topic==CONVERSATION_NO_MEMORY);
 assert(memcmp(&kept,&f.profile,sizeof(kept))==0); /* same-day load */
 /* Quiet rest upgrades exactly one nap interaction. */
 reset(&f,9);(void)care(&f,FEED,0);(void)care(&f,NAP,0);
 assert(game_room_quiet_rest(&f.game,0,0));
 conversation_record_event(&f.session,CONVERSATION_EVENT_QUIET_REST,0,f.game.days);
 assert(f.session.fact_count==2u);
 assert(f.session.facts[0].kind==CONVERSATION_EVENT_QUIET_REST);
 assert(f.session.facts[1].kind==CONVERSATION_EVENT_FEED&&
        f.session.facts[1].age==1u);
 assert(ask(&f,CONVERSATION_REMEMBER)->topic==CONVERSATION_MEMORY_QUIET);
}
static void care_effects_and_requests(void){
 Fixture f;
 reset(&f,1);f.game.hunger=0;
 assert(care(&f,FEED,1)->topic==CONVERSATION_FULL_FEED);
 reset(&f,1);f.game.energy=100;
 assert(care(&f,NAP,1)->topic==CONVERSATION_FULL_NAP);
 reset(&f,1);f.game.energy=1;
 assert(care(&f,NAP,1)->topic==CONVERSATION_NEED_REST); /* still tired */
 reset(&f,1);f.game.hunger=100;
 assert(care(&f,FEED,1)->topic==CONVERSATION_NEED_FOOD); /* still hungry */
 reset(&f,1);f.game.energy=60;f.game.hunger=10;
 assert(care(&f,PLAY,1)->topic==CONVERSATION_TIRED_PLAY);
 reset(&f,1);f.game.energy=40;
 assert(care(&f,PLAY,1)->topic==CONVERSATION_NEED_REST);
 reset(&f,1);f.game.hunger=90;
 assert(ask(&f,CONVERSATION_NEED)->topic==CONVERSATION_NEED_FOOD);
 assert(care(&f,FEED,1)->topic==CONVERSATION_CARE_THANKS);
 assert(care(&f,FEED,1)->topic!=CONVERSATION_CARE_THANKS);
 assert(care(&f,FEED,1)->topic==CONVERSATION_REPEAT_CARE);
 /* Same sequence as native recovery: tired and chaotic, then one useful
  * NAP. Mild hunger 68 must not contradict the nursery's ALL GOOD hint. */
 reset(&f,1);f.game.energy=12;f.game.hunger=60;f.game.chaos=72;
 assert(ask(&f,CONVERSATION_NEED)->topic==CONVERSATION_NEED_REST);
 assert(care(&f,NAP,1)->topic==CONVERSATION_CARE_THANKS);
 assert(f.game.energy==37&&f.game.hunger==68&&f.game.chaos==67);
 assert(game_care_hint(&f.game)==-1);
 assert(ask(&f,CONVERSATION_NEED)->topic==CONVERSATION_NEED_NONE);
}
static void urgent_priority(void){
 Fixture f;
 static const unsigned states[][6]={
  {95,10,15,90,90,CONVERSATION_NEED_REST},
  {95,20,15,90,90,CONVERSATION_NEED_FOOD},
  {69,34,15,90,90,CONVERSATION_NEED_REST},
  {69,75,15,10,90,CONVERSATION_NEED_COMPANY},
  {35,75,75,12,90,CONVERSATION_NEED_QUIET}
 };
 for(unsigned row=0;row<sizeof(states)/sizeof(states[0]);row++)
 for(unsigned style=0;style<CONVERSATION_STYLE_COUNT;style++){
  reset(&f,17);prefer_style(&f.profile,style);
  (void)care(&f,FEED,0);
  f.game.hunger=(uint8_t)states[row][0];f.game.energy=(uint8_t)states[row][1];
  f.game.mood=(uint8_t)states[row][2];f.game.chaos=(uint8_t)states[row][3];
  f.game.corruption=(uint8_t)states[row][4];
  for(unsigned prompt=0;prompt<6u;prompt++){
   const ConversationReply *r=ask(&f,(ConversationPrompt)prompt);
   assert(r->topic==states[row][5]);
   assert(r->topic!=CONVERSATION_CARE_THANKS);
  }
 }
 reset(&f,1);f.game.hunger=69;
 assert(ask(&f,CONVERSATION_NEED)->topic==CONVERSATION_NEED_NONE);
 f.game.hunger=70;
 assert(ask(&f,CONVERSATION_NEED)->topic==CONVERSATION_NEED_FOOD);
}
static void learning_and_determinism(void){
 Fixture f;reset(&f,123);
 const ConversationReply *cold=ask(&f,CONVERSATION_HOW);
 unsigned wanted=(cold->style+1u)%CONVERSATION_STYLE_COUNT;
 unsigned learned=0;
 Game untouched=f.game;
 /* Learn from answers actually selected by inference. Other answers are
  * left unrated; no test writes the preferred row in this experiment. */
 for(unsigned attempts=0;attempts<1000u&&learned<24u;attempts++){
  const ConversationReply *r=ask(&f,CONVERSATION_HOW);
  if(r->style==wanted){(void)ask(&f,CONVERSATION_LIKE);learned++;}
 }
 assert(learned==24u&&f.profile.likes==24u);
 assert(memcmp(&untouched,&f.game,sizeof(untouched))==0);
 conversation_session_reset(&f.session,f.game.days,123);f.displayed=UINT16_MAX;
 const ConversationReply *trained=ask(&f,CONVERSATION_HOW);
 assert(trained->style==wanted&&trained->style!=cold->style);
 assert(conversation_profile_valid(&f.profile));
 /* Serialization/copying the tiny profile reproduces its learned choice
  * with a fresh session, without inheriting old facts or a hidden RNG. */
 Fixture clone=f;
 unsigned char bytes[sizeof(ConversationProfile)];
 memcpy(bytes,&f.profile,sizeof(bytes));
 conversation_profile_reset(&clone.profile);
 memcpy(&clone.profile,bytes,sizeof(bytes));
 conversation_session_reset(&f.session,f.game.days,19);f.displayed=UINT16_MAX;
 conversation_session_reset(&clone.session,clone.game.days,19);clone.displayed=UINT16_MAX;
 for(unsigned i=0;i<2000u;i++){
  ConversationPrompt p=(ConversationPrompt)(i%6u);
  const ConversationReply *a=ask(&f,p),*b=ask(&clone,p);
  assert(a->id==b->id&&memcmp(&f.profile,&clone.profile,sizeof(f.profile))==0);
  assert(memcmp(&f.session,&clone.session,sizeof(f.session))==0);
 }
 /* The rated feature vector belongs to the earlier answer, even if the
  * current needs change before the feedback arrives. */
 reset(&f,4);(void)ask(&f,CONVERSATION_HOW);
 unsigned style=f.session.style;
 int8_t credited[CONVERSATION_FEATURE_COUNT];
 memcpy(credited,f.session.last_features,sizeof(credited));
 f.game.energy=1;f.game.corruption=99;
 assert(ask(&f,CONVERSATION_LIKE)->topic==CONVERSATION_NEED_REST);
 for(unsigned i=0;i<CONVERSATION_FEATURE_COUNT;i++)
  assert(f.profile.adapter[style][i]==(credited[i]>0?1:0));
 ConversationProfile once=f.profile;
 (void)ask(&f,CONVERSATION_LIKE);
 assert(memcmp(&once,&f.profile,sizeof(once))==0);
 /* A skip is deliberately weaker: it moves one bias weight, never the
  * remembered fact or the other eleven context weights. */
 reset(&f,4);(void)ask(&f,CONVERSATION_HOW);style=f.session.style;
 (void)ask(&f,CONVERSATION_ELSE);
 assert(f.profile.changes==1&&f.profile.likes==0);
 for(unsigned s=0;s<CONVERSATION_STYLE_COUNT;s++)
  for(unsigned i=0;i<CONVERSATION_FEATURE_COUNT;i++)
   assert(f.profile.adapter[s][i]==(s==style&&i==8u?-1:0));
}
static void bounds_and_invalid_inputs(void){
 Fixture f;reset(&f,1);
 assert(!conversation_profile_valid(NULL));
 assert(conversation_profile_valid(&f.profile));
 f.profile.adapter[0][0]=CONVERSATION_ADAPTER_LIMIT+1;
 assert(!conversation_profile_valid(&f.profile));
 f.profile.adapter[0][0]=-CONVERSATION_ADAPTER_LIMIT-1;
 assert(!conversation_profile_valid(&f.profile));
 conversation_profile_reset(&f.profile);
 f.profile.likes=UINT16_MAX;f.profile.changes=UINT16_MAX;
 assert(conversation_profile_valid(&f.profile));
 (void)ask(&f,CONVERSATION_HOW);(void)ask(&f,CONVERSATION_LIKE);
 (void)ask(&f,CONVERSATION_HOW);(void)ask(&f,CONVERSATION_ELSE);
 assert(f.profile.likes==UINT16_MAX&&f.profile.changes==UINT16_MAX);
 for(unsigned i=0;i<2000u;i++){
  (void)ask(&f,CONVERSATION_HOW);(void)ask(&f,CONVERSATION_LIKE);
  assert(conversation_profile_valid(&f.profile));
 }
 ConversationSession before=f.session;
 conversation_record_event(&f.session,CONVERSATION_EVENT_FEED,0,f.game.days);
 assert(memcmp(&before,&f.session,sizeof(before))==0);
 assert(!conversation_respond(&f.profile,&f.session,&f.game,(ConversationPrompt)99));
 assert(memcmp(&before,&f.session,sizeof(before))==0);
 assert(!conversation_care_response(&f.profile,&f.session,&f.game,CHAT));
 assert(!conversation_respond(&f.profile,NULL,&f.game,CONVERSATION_HOW));
 assert(!conversation_respond(&f.profile,&f.session,NULL,CONVERSATION_HOW));
 conversation_profile_reset(NULL);conversation_session_reset(NULL,1,0);
 conversation_record_care(NULL,&f.game,&f.game,FEED);
}
int main(void){
 phase="catalog";
 catalog_and_encoder();
 phase="reachability";
 all_responses_reachable();
 phase="memory";
 memory_continuity_and_expiry();
 phase="care";
 care_effects_and_requests();
 phase="urgent";
 urgent_priority();
 phase="learning";
 learning_and_determinism();
 phase="bounds";
 bounds_and_invalid_inputs();
 printf("PASS: %u reachable authored replies, %lu-byte profile/%lu-byte session, learned style changes, factual continuity, needs priority and unchanged Game/RNG\n",
        conversation_catalog_count(),(unsigned long)sizeof(ConversationProfile),
        (unsigned long)sizeof(ConversationSession));
 return 0;
}
