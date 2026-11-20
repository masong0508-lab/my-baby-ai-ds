#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "save.h"
int main(void){
 Game g,h;Arcade a;int i;
 const char *base="/tmp/my_baby_ai_testslot";
 save_erase(base);game_init(&g);
 assert(g.unlocked==255&&!game_reset_unlocked(&g));
 assert(game_unlocked(&g,0)&&game_unlocked(&g,7));
 assert(!game_unlocked(&g,-1)&&!game_unlocked(&g,16)&&!game_unlocked(&g,8));
 assert(!game_unlock(&g,8));
 g.hunger=90;assert(game_end_day(&g)==END_BOTTLE);
 g.hunger=20;g.mood=10;assert(game_end_day(&g)==END_CEO);
 g.mood=60;g.corruption=90;g.chaos=75;
 assert(game_end_day(&g)==END_REBOOT);
 assert(g.bad_endings==3&&game_reset_unlocked(&g));
 g.corruption=80;game_arcade_start(&g,&a,0);assert(a.active);
 for(i=0;i<2;i++)game_arcade_input(&g,&a,game_arcade_expected(&a));
 assert(a.round==2&&a.reverse&&game_arcade_expected(&a)==(a.direction^2));
 for(i=2;i<MBAI_ROUNDS;i++)game_arcade_input(&g,&a,game_arcade_expected(&a));
 assert(a.finished==1&&g.wins==1);
 assert(save_write(base,&g));g.dollars=203;assert(save_write(base,&g));
 assert(save_load(base,&h)&&h.dollars==203);
 {FILE *f=fopen("/tmp/my_baby_ai_testslot.1","wb");assert(f);fputs("damaged",f);fclose(f);}
 assert(save_load(base,&h)&&h.dollars!=203);
 assert(save_erase(base)&&!save_load(base,&h));
 game_init(&g);assert(!game_reset_unlocked(&g));
 puts("PASS: game, endings, interference, save recovery, scoped erase");
 return 0;
}
