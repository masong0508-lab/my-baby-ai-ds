#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "ui.h"
static uint16_t screen[256*256];
int main(void){
 Game g;Arcade a={0};unsigned i,changed=0;
 game_init(&g);ui_init(screen);
 ui_draw(&g,&a,0,0,0,0,1,"TELL HR I'M TEETHING.");
 for(i=0;i<256u*192u;i++)if(screen[i])changed++;
 assert(changed>30000);
 assert(ui_nursery_touch(25,131)==FEED);
 assert(ui_nursery_touch(175,131)==CHAT);
 assert(ui_nursery_touch(25,159)==PLAY);
 assert(ui_nursery_touch(175,159)==NAP);
 assert(ui_nursery_touch(0,0)==-1);
 assert(ui_home_navigation(35,183)==1);
 assert(ui_home_navigation(225,183)==2);
 assert(ui_arcade_select(9,66)==0);
 assert(ui_arcade_select(195,145)==15);
 assert(ui_arcade_select(255,191)==-1);
 assert(ui_arcade_navigation(11,181)==1);
 assert(ui_arcade_navigation(112,181)==2);
 assert(ui_arcade_navigation(221,181)==3);
 assert(ui_arcade_navigation(11,122)==0);
 a.timer=110;
 ui_draw(&g,&a,1,0,0,0,1,"ARCADE");
 ui_draw(&g,&a,2,0,0,0,1,"GO!");
 assert(ui_arcade_touch(127,100)==0);
 assert(ui_arcade_touch(170,126)==1);
 assert(ui_arcade_touch(127,148)==2);
 assert(ui_arcade_touch(100,126)==3);
 assert(ui_arcade_touch(0,0)==-1);
 ui_draw(&g,&a,4,0,END_GRADUATION,0,1,"CONTINUE");
 ui_draw(&g,&a,3,0,0,1,1,"RESET?");
 ui_draw(&g,&a,5,0,0,0,1,"START");
 ui_draw(&g,&a,6,0,0,0,1,"ALBUM");
 puts("PASS: bitmap UI and touch hitboxes");
 return 0;
}
