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
 a.id=0;a.timer=600;a.balance=12;a.hold_frames=45;
 ui_draw(&g,&a,2,0,0,0,1,"BOTTLE");
 a.id=1;a.timer=140;a.stage=0;a.target=3;
 ui_draw(&g,&a,2,0,0,0,1,"DIAPER");
 a.id=7;a.timer=70;a.hold_frames=144;a.target=1;a.round=2;
 ui_draw(&g,&a,2,0,0,0,1,"RATTLE");
 assert(ui_rattle_touch(40,143)==0);
 assert(ui_rattle_touch(122,143)==1);
 assert(ui_rattle_touch(210,143)==2);
 assert(ui_rattle_touch(90,143)==-1);
 assert(ui_rattle_touch(210,85)==-1);
 a.id=2;a.timer=110;
 ui_draw(&g,&a,2,0,0,0,1,"DIRECTION");
 assert(ui_arcade_touch(127,100)==0);
 assert(ui_arcade_touch(170,126)==1);
 assert(ui_arcade_touch(127,148)==2);
 assert(ui_arcade_touch(100,126)==3);
 assert(ui_arcade_touch(0,0)==-1);
 assert(ui_diaper_touch(120,100)==0);
 assert(ui_diaper_touch(40,140)==3);
 assert(ui_diaper_touch(185,140)==1);
 assert(ui_diaper_touch(124,162)==2);
 assert(ui_diaper_touch(2,142)==-1);
 assert(ui_diaper_touch(240,179)==-1);
 ui_draw(&g,&a,4,0,END_GRADUATION,0,1,"CONTINUE");
 ui_draw(&g,&a,3,0,0,1,1,"RESET?");
 ui_draw(&g,&a,5,0,0,0,1,"START");
 ui_draw(&g,&a,6,0,0,0,1,"ALBUM");
 ui_touch_observe(1,120,130,3);
 ui_draw(&g,&a,7,0,0,0,1,"TOUCH DEBUG");
 {
  /* Crosshair must not stick after stylus release, or ghost at 0,0. */
  unsigned index=139u*256u+122u; /* x=31+120*194/255, y=119+130*30/191 */
  uint16_t canvas=screen[index];
  ui_touch_diag_update();
  assert(screen[index]!=canvas);
  ui_touch_diag_update(); /* same state should be a no-op */
  ui_touch_observe(0,0,0,3);
  ui_touch_diag_update();
  assert(screen[index]==canvas);
  ui_touch_observe(1,120,130,4);
  ui_touch_diag_update();
  assert(screen[index]!=canvas);
  ui_touch_observe(1,220,170,4);
  ui_touch_diag_update();
  assert(screen[index]==canvas); /* old crosshair erased on movement */
  ui_touch_observe(0,0,0,4);
  ui_touch_diag_update();
 }
 ui_draw(&g,&a,7,0,0,0,1,"TOUCH DEBUG");
 ui_touch_diag_update();
 /* Cursor must restore the exact pixels it covered, then obey a UI
  * redraw without resurrecting stale pixels from the previous scene. */
 {
  int x=121,y=130;
  uint16_t before=screen[y*256+x];
  ui_virtual_cursor_update(1,x,y,0);
  assert(screen[y*256+x]!=before);
  ui_virtual_cursor_update(0,x,y,0);
  assert(screen[y*256+x]==before);
  ui_virtual_cursor_update(1,x,y,1);
  ui_draw(&g,&a,5,0,0,0,1,"TITLE");
  ui_virtual_cursor_update(1,42,112,0);
  ui_virtual_cursor_update(0,42,112,0);
 }
 puts("PASS: bitmap UI and touch hitboxes");
 return 0;
}
