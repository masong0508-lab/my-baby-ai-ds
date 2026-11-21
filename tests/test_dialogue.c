#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "dialogue.h"
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
 puts("PASS: bounded, deterministic offline integer dialogue engine");
 return 0;
}
