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
  counts[intent]++;
  line=dialogue_reply(&s,(Care)(j%4));
  assert(line&&strlen(line)>10&&strlen(line)<40);
 }
 for(i=0;i<6;i++)printf("classifier class %d: %d observations\n",i,counts[i]);
 puts("PASS: bounded, deterministic offline integer dialogue engine");
 return 0;
}
