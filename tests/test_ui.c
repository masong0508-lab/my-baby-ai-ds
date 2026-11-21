#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "ui.h"
#include "roughhouse.h"
static uint16_t screen[256*256];
/* All rendering must remain inside the 256x192 visible bitmap. Each screen
 * now draws its own complete three-band surface without a blanket clear. */
static void verify_coverage(void){
 unsigned i;
 for(i=0;i<256u*192u;i++)assert(screen[i]&0x8000u);
 for(i=256u*192u;i<256u*256u;i++)assert(screen[i]==0x5a5au);
}
static uint32_t fingerprint(void){
 uint32_t h=2166136261u;
 unsigned i;
 for(i=0;i<256u*192u;i+=7){
  h^=screen[i];h*=16777619u;
 }
 return h;
}
/* Thumbnail hashes look ONLY at the 36x34 illustrated wells, not the
 * labels/page number: all sixteen cabinets need distinct, real artwork. */
static uint32_t icon_fingerprint(int x,int y){
 uint32_t h=2166136261u;
 int dx,dy;
 for(dy=0;dy<34;dy++)for(dx=0;dx<36;dx++){
  h^=screen[(y+dy)*256+x+dx];h*=16777619u;
 }
 return h;
}
/* Scan the real pad picture region separately from its background/name.
 * Four inputs must be visually distinguishable at native 256x192. */
static uint32_t beat_pad_fingerprint(int x){
 uint32_t h=2166136261u;
 int dx,dy;
 for(dy=127;dy<149;dy++)for(dx=0;dx<48;dx++){
  h^=screen[dy*256+x+dx];h*=16777619u;
 }
 return h;
}
int main(void){
 Game g;Arcade a={0};unsigned i,changed=0;
 game_init(&g);ui_init(screen);
 for(i=256u*192u;i<256u*256u;i++)screen[i]=0x5a5au;
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
 assert(ui_home_navigation(130,183)==0);
 assert(ui_prank_entry_touch(125,184));
 assert(!ui_prank_entry_touch(194,45));
 assert(!ui_prank_entry_touch(225,183));
 assert(ui_roughhouse_touch(48,160)==1); /* select SLAP */
 assert(ui_roughhouse_touch(191,160)==2); /* select KICK */
 assert(ui_roughhouse_touch(130,109)==4); /* use selected toy */
 assert(ui_roughhouse_touch(128,182)==3); /* back */
 assert(ui_roughhouse_touch(128,67)==0);
 assert(ui_arcade_select_page(35,92,0)==0);
 assert(ui_arcade_select_page(173,92,0)==1);
 assert(ui_arcade_select_page(35,145,0)==2);
 assert(ui_arcade_select_page(173,145,0)==3);
 assert(ui_arcade_select_page(173,145,12)==15);
 assert(ui_arcade_select_page(35,92,8)==8);
 assert(ui_arcade_select_page(255,191,0)==-1);
 assert(ui_arcade_select_page(128,93,0)==-1);
 assert(ui_arcade_page_touch(34,44)==-1);
 assert(ui_arcade_page_touch(220,44)==1);
 assert(ui_arcade_page_touch(128,44)==0);
 assert(ui_arcade_page_touch(220,72)==0);
 assert(ui_album_page_touch(35,43)==-1);
 assert(ui_album_page_touch(221,43)==1);
 assert(ui_album_page_touch(120,43)==0);
 assert(ui_album_page_touch(221,79)==0);
 assert(ui_nursery_touch(25,109)==FEED);
 assert(ui_nursery_touch(25,143)==PLAY);
 assert(ui_nursery_touch(25,140)==-1);
 assert(ui_nursery_touch(25,175)==-1);
 assert(ui_arcade_navigation(11,181)==1);
 assert(ui_arcade_navigation(112,181)==2);
 assert(ui_arcade_navigation(221,181)==3);
 assert(ui_arcade_navigation(11,122)==0);
 {
  uint32_t hand,foot;
  ui_draw(&g,&a,8,5,0,0,1,"TOYBOX");
  verify_coverage();
  hand=fingerprint();
  ui_draw(&g,&a,8,37,0,0,1,"TOYBOX");
  verify_coverage();
  foot=fingerprint();
  assert(hand!=foot); /* SELECTED mode must be unmistakable */
  ui_draw(&g,&a,9,EGG_PUSH_FRAME,0,0,1,"EGG");
  verify_coverage();
  ui_draw(&g,&a,9,EGG_SCARE_FRAME,0,0,1,"EGG");
  verify_coverage();
  ui_draw(&g,&a,6,2,0,0,1,"ALBUM");
  verify_coverage();
 }
 a.timer=110;
 ui_draw(&g,&a,1,0,0,0,1,"ARCADE");
 {
  uint32_t first=fingerprint(), second;
  ui_draw(&g,&a,1,4,0,0,1,"ARCADE");
  second=fingerprint();
  assert(first!=second); /* Every four-cabinet page must be visibly unique. */
  ui_draw(&g,&a,1,8,0,0,1,"ARCADE");
  assert(second!=fingerprint());
  ui_draw(&g,&a,1,12,0,0,1,"ARCADE");
  assert(first!=fingerprint());
 }
 /* Test the NEW visual assets independently of labels and page design. */
 {
  uint32_t icons[16];
  unsigned id,k;
  for(id=0;id<16;id++){
   int slot=(int)id%4;
   int x=9+(slot%2)*123+6, y=73+(slot/2)*50+5;
   ui_draw(&g,&a,1,(int)id,0,0,1,"ARCADE");
   verify_coverage();
   icons[id]=icon_fingerprint(x,y);
   for(k=0;k<id;k++)assert(icons[id]!=icons[k]);
  }
 }
 ui_draw(&g,&a,2,0,0,0,1,"GO!");
 a.id=0;a.timer=600;a.balance=12;a.hold_frames=45;
 ui_draw(&g,&a,2,0,0,0,1,"BOTTLE");
 /* No more fake vertical-edge glitch strips on each playfield. */
 assert(screen[45u*256u+2u]==screen[90u*256u+2u]);
 assert(screen[45u*256u+253u]==screen[90u*256u+253u]);
 a.id=1;a.timer=140;a.stage=0;a.target=3;
 ui_draw(&g,&a,2,0,0,0,1,"DIAPER");
 a.id=2;a.timer=120;a.target=3;a.round=2;
 ui_draw(&g,&a,2,0,0,0,1,"TAX FORM");
 a.id=3;a.stage=0;a.target=1;a.timer=45;a.hold_frames=90;
 ui_draw(&g,&a,2,0,0,0,1,"CRIB KART");
 assert(ui_kart_touch(40,142)==0);
 assert(ui_kart_touch(120,142)==1);
 assert(ui_kart_touch(204,142)==2);
 assert(ui_kart_touch(87,142)==-1);
 assert(ui_kart_touch(120,190)==-1);
 a.id=4;a.phase=0;a.timer=90;a.hold_frames=90;
 a.notes[0]=1;a.notes[1]=3;a.notes[2]=2;
 ui_draw(&g,&a,2,0,0,0,1,"BABY BEATBOX");
 {
  uint32_t pads[4],watch=fingerprint();unsigned k,p;
  for(p=0;p<4;p++){
   pads[p]=beat_pad_fingerprint(12+(int)p*62);
   for(k=0;k<p;k++)assert(pads[p]!=pads[k]);
  }
  /* Both control states have a different 2x imperative prompt. */
  a.phase=1;a.stage=2;
  ui_draw(&g,&a,2,0,0,0,1,"REPEAT");
  assert(fingerprint()!=watch);
 }
 a.phase=1;a.stage=2;
 ui_draw(&g,&a,2,0,0,0,1,"REPEAT");
 assert(ui_beatbox_touch(30,148)==0);
 assert(ui_beatbox_touch(93,148)==1);
 assert(ui_beatbox_touch(155,148)==2);
 assert(ui_beatbox_touch(217,148)==3);
 assert(ui_beatbox_touch(70,148)==-1);
 assert(ui_beatbox_touch(30,100)==-1);
 a.id=5;a.timer=120;a.target=1;
 a.notes[0]=2;a.notes[1]=0;a.notes[2]=1;
 ui_draw(&g,&a,2,0,0,0,1,"SOCK MARKET");
 assert(ui_sock_touch(40,145)==0);
 assert(ui_sock_touch(122,145)==1);
 assert(ui_sock_touch(200,145)==2);
 assert(ui_sock_touch(86,145)==-1);
 assert(ui_sock_touch(40,100)==-1);
 a.id=6;a.timer=350;a.hold_frames=100;a.elapsed=51;a.round=2;
 ui_draw(&g,&a,2,0,0,0,1,"NAPTIME");
 assert(ui_nap_touch(128,145)==0);
 assert(ui_nap_touch(20,92)==0);
 assert(ui_nap_touch(256,145)==-1);
 assert(ui_nap_touch(128,183)==-1);
 a.id=7;a.timer=70;a.hold_frames=144;a.target=1;a.round=2;
 ui_draw(&g,&a,2,0,0,0,1,"RATTLE");
 assert(ui_rattle_touch(40,143)==0);
 assert(ui_rattle_touch(122,143)==1);
 assert(ui_rattle_touch(210,143)==2);
 assert(ui_rattle_touch(90,143)==-1);
 assert(ui_rattle_touch(210,85)==-1);
 a.id=8;a.timer=45;a.hold_frames=82;a.stage=0;a.target=2;a.round=1;
 ui_draw(&g,&a,2,0,0,0,1,"CORPORATE CRAWL");
 assert(ui_crawl_touch(33,153)==0);
 assert(ui_crawl_touch(199,153)==2);
 assert(ui_crawl_touch(129,153)==-1);
 assert(ui_crawl_touch(33,188)==-1);
 a.id=9;a.timer=610;a.balance=14;a.hold_frames=28;a.round=2;
 ui_draw(&g,&a,2,0,0,0,1,"STROLLER DRIFT");
 a.id=10;a.timer=130;a.stage=3;a.direction=4;a.target=1;
 a.notes[0]=6;a.notes[1]=7;a.notes[2]=8;
 ui_draw(&g,&a,2,0,0,0,1,"CEREAL CIPHER");
 assert(ui_cereal_touch(45,150)==0);
 assert(ui_cereal_touch(120,150)==1);
 assert(ui_cereal_touch(203,150)==2);
 assert(ui_cereal_touch(88,150)==-1);
 a.id=11;a.timer=120;a.hold_frames=16;a.direction=2;a.target=2;
 ui_draw(&g,&a,2,0,0,0,1,"TANTRUM BLOCKS");
 assert(ui_blocks_touch(128,152)==0);
 assert(ui_blocks_touch(33,100)==0);
 assert(ui_blocks_touch(128,183)==-1);
 a.id=12;a.timer=140;a.stage=41;a.direction=46;a.target=1;
 ui_draw(&g,&a,2,0,0,0,1,"BABY AUDIT");
 assert(ui_audit_touch(57,152)==0);
 assert(ui_audit_touch(196,152)==1);
 assert(ui_audit_touch(129,152)==-1);
 a.id=13;a.timer=300;a.target=2;a.progress=65;
 ui_draw(&g,&a,2,0,0,0,1,"GLITCH GARDEN");
 assert(ui_garden_touch(45,143)==0);
 assert(ui_garden_touch(121,143)==1);
 assert(ui_garden_touch(207,143)==2);
 assert(ui_garden_touch(83,143)==-1);
 a.id=14;a.timer=195;a.stage=1;
 a.notes[0]=2;a.notes[1]=0;a.notes[2]=1;
 ui_draw(&g,&a,2,0,0,0,1,"COURTROOM CRAYONS");
 assert(ui_court_touch(46,134)==0);
 assert(ui_court_touch(128,92)==1);
 assert(ui_court_touch(210,134)==2);
 assert(ui_court_touch(128,164)==-1);
 a.id=15;a.timer=240;a.balance=57;a.progress=55;a.round=2;
 ui_draw(&g,&a,2,0,0,0,1,"CEO OF DAYCARE");
 assert(ui_ceo_touch(46,144)==0);
 assert(ui_ceo_touch(126,144)==1);
 assert(ui_ceo_touch(213,144)==2);
 assert(ui_ceo_touch(87,144)==-1);
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
 /* The title portrait must have several distinct facial/romper colors and
  * no uninitialized VRAM columns in the baby icon's 40x36 pixel patch. */
 {
  uint16_t seen[12]={0};unsigned unique=0,x,y,k;
  for(y=78;y<112;y++)for(x=109;x<148;x++){
   uint16_t pix=screen[y*256+x];
   for(k=0;k<unique&&k<12;k++)if(seen[k]==pix)break;
   if(k==unique&&unique<12)seen[unique++]=pix;
  }
  assert(unique>=7);
 }
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
 /* Full visual coverage is a correctness requirement after removing the
  * unconditional 49,152-pixel VRAM clear in the DS renderer. */
 {
  uint32_t hashes[16];
  unsigned view,id,k;
  g.unlocked=0xffffu;
  for(view=0;view<8;view++){
   if(view==2)continue;
   ui_draw(&g,&a,(int)view,0,END_GRADUATION,0,1,"DISPLAY TEST");
   verify_coverage();
  }
  /* Both confirmation stages, both outcome types and save indicators must
   * be visually distinct even though all use the same screen geometry. */
  ui_draw(&g,&a,6,0,0,0,1,"ALBUM");
  verify_coverage();
  hashes[0]=fingerprint();
  ui_draw(&g,&a,6,1,0,0,1,"ALBUM");
  verify_coverage();
  assert(hashes[0]!=fingerprint());
  ui_draw(&g,&a,3,0,0,0,1,"RESET");
  verify_coverage();
  hashes[0]=fingerprint();
  ui_draw(&g,&a,3,0,0,1,1,"RESET");
  verify_coverage();
  assert(fingerprint()!=hashes[0]);
  ui_draw(&g,&a,4,0,END_BOTTLE,0,1,"ENDING");
  verify_coverage();
  hashes[0]=fingerprint();
  ui_draw(&g,&a,4,0,END_GRADUATION,0,1,"ENDING");
  verify_coverage();
  assert(fingerprint()!=hashes[0]);
  /* The main baby's pixel portrait must be genuinely multitone, with
   * more than a flat hair/skin/clothing tricolor palette. */
  ui_draw(&g,&a,5,0,0,0,0,"RAM MODE");
  verify_coverage();
  {
   uint16_t shades[32]={0};unsigned used=0,x,y,k;
   for(y=72;y<126;y++)for(x=98;x<158;x++){
    uint16_t pixel=screen[y*256+x];
    for(k=0;k<used;k++)if(shades[k]==pixel)break;
    if(k==used&&used<32)shades[used++]=pixel;
   }
   assert(used>=10);
  }
  hashes[0]=fingerprint();
  ui_draw(&g,&a,5,0,0,0,1,"RAM MODE");
  verify_coverage();
  assert(fingerprint()!=hashes[0]);
  for(id=0;id<16;id++){
   game_arcade_start(&g,&a,(int)id);
   assert(a.active);
   ui_draw(&g,&a,2,(int)id,0,0,1,"CABINET");
   verify_coverage();
   hashes[id]=fingerprint();
   /* Every cabinet gets its own theme and unique composition. */
   for(k=0;k<id;k++)assert(hashes[id]!=hashes[k]);
  }
  /* Safe-visuals and the unlocked state do not affect screen coverage. */
  g.safe_visuals=1;
  ui_draw(&g,&a,0,0,0,0,0,"SAFE MODE");
  verify_coverage();
 }
 puts("PASS: illustrated 4-pad Beatbox, readable care layout, 16 unique cabinets, touchscreen bounds and full coverage");
 return 0;
}
