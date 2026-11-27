#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"
#include "roughhouse.h"
static uint16_t screen[256*256];
#define TEST_RGB(r,g,b) ((uint16_t)(0x8000u|(r)|((g)<<5)|((b)<<10)))
/* Read the actual native pixels so tests catch dropped words, wrong
 * line breaks and clipped final glyphs, not just a changing UI hash. */
static void verify_text(int x,int y,const char *text,int columns,
                         uint16_t ink,int scale){
 static const uint8_t font[96][7]={
  ['A'-32]={14,17,17,31,17,17,17}, ['B'-32]={30,17,17,30,17,17,30},
  ['C'-32]={14,17,16,16,16,17,14}, ['D'-32]={30,17,17,17,17,17,30},
  ['E'-32]={31,16,16,30,16,16,31}, ['F'-32]={31,16,16,30,16,16,16},
  ['G'-32]={14,17,16,23,17,17,14}, ['H'-32]={17,17,17,31,17,17,17},
  ['I'-32]={31,4,4,4,4,4,31}, ['J'-32]={7,2,2,2,18,18,12},
  ['K'-32]={17,18,20,24,20,18,17}, ['L'-32]={16,16,16,16,16,16,31},
  ['M'-32]={17,27,21,21,17,17,17}, ['N'-32]={17,25,21,19,17,17,17},
  ['O'-32]={14,17,17,17,17,17,14}, ['P'-32]={30,17,17,30,16,16,16},
  ['Q'-32]={14,17,17,17,21,18,13}, ['R'-32]={30,17,17,30,20,18,17},
  ['S'-32]={15,16,16,14,1,1,30}, ['T'-32]={31,4,4,4,4,4,4},
  ['U'-32]={17,17,17,17,17,17,14}, ['V'-32]={17,17,17,17,17,10,4},
  ['W'-32]={17,17,17,21,21,21,10}, ['X'-32]={17,17,10,4,10,17,17},
  ['Y'-32]={17,17,10,4,4,4,4}, ['Z'-32]={31,1,2,4,8,16,31},
  ['0'-32]={14,19,21,21,21,25,14}, ['1'-32]={4,12,4,4,4,4,14},
  ['2'-32]={14,17,1,2,4,8,31}, ['3'-32]={30,1,1,14,1,1,30},
  ['4'-32]={2,6,10,18,31,2,2}, ['5'-32]={31,16,30,1,1,17,14},
  ['6'-32]={6,8,16,30,17,17,14}, ['7'-32]={31,1,2,4,8,8,8},
  ['8'-32]={14,17,17,14,17,17,14}, ['9'-32]={14,17,17,15,1,2,12},
  ['!'-32]={4,4,4,4,4,0,4}, ['?'-32]={14,17,1,2,4,0,4},
  ['.'-32]={0,0,0,0,0,6,6}, [','-32]={0,0,0,0,6,4,8},
  [':'-32]={0,4,4,0,4,4,0}, ['-'-32]={0,0,0,31,0,0,0},
  ['\''-32]={4,4,8,0,0,0,0}, ['/'-32]={1,1,2,4,8,16,16},
  ['+'-32]={0,4,4,31,4,4,0}, ['%'-32]={25,26,2,4,8,11,19},
  ['$'-32]={4,15,20,14,5,30,4},
  ['<'-32]={2,4,8,16,8,4,2}, ['='-32]={0,0,31,0,31,0,0},
  ['*'-32]={0,21,14,31,14,21,0}, ['('-32]={2,4,8,8,8,4,2},
  [')'-32]={8,4,2,2,2,4,8}
 };
 int i,px,py;
 size_t length=strlen(text);
 assert(x>=0&&y>=0&&x+columns*6*scale<=256&&y+7*scale<=192);
 for(i=0;i<columns;i++){
  unsigned c=(size_t)i<length?(unsigned char)text[i]:' ';
  if(c>='a'&&c<='z')c-=32;
  assert(c>=32&&c<128);
  for(py=0;py<7*scale;py++)for(px=0;px<6*scale;px++){
   int expected=px<5*scale&&
                 (font[c-32][py/scale]&(1u<<(4-px/scale)))!=0;
   if((screen[(y+py)*256+x+i*6*scale+px]==ink)!=expected)
    fprintf(stderr,"Text mismatch at (%d,%d), character %d of '%s'\n",x,y,i,text);
   assert((screen[(y+py)*256+x+i*6*scale+px]==ink)==expected);
  }
 }
}
/* All rendering must remain inside the 256x192 visible bitmap. Each screen
 * now draws its own complete three-band surface without a blanket clear. */
static void verify_coverage(void){
 unsigned i;
 for(i=0;i<256u*192u;i++)assert(screen[i]&0x8000u);
 for(i=256u*192u;i<256u*256u;i++)assert(screen[i]==0x5a5au);
}
/* Current win/Favor balance must be visible above the paid unlock goal. */
static uint32_t arcade_progress_fingerprint(void){
 uint32_t hash=2166136261u;
 int x,y;
 for(y=33;y<43;y++)for(x=70;x<188;x++){
  hash^=screen[y*256+x];hash*=16777619u;
 }
 return hash;
}
/* Isolate the arcade requirement strip, not the nearby cabinet icons.
 * This catches regressions where the math exists but isn't visible. */
static uint32_t arcade_goal_fingerprint(void){
 uint32_t hash=2166136261u;
 int x,y;
 for(y=44;y<55;y++)for(x=70;x<188;x++){
  hash^=screen[y*256+x];hash*=16777619u;
 }
 return hash;
}
static uint32_t care_hint_fingerprint(void){
 uint32_t hash=2166136261u;
 int x,y;
 /* Header location never crosses ROOM capsule or TOUCH TO TALK. */
 for(y=35;y<43;y++)for(x=126;x<189;x++){
  hash^=screen[y*256+x];hash*=16777619u;
 }
 return hash;
}
/* Distinguish geometry count updates in the optional diagnostics strip.
 * This scans the real 256x192 software-VRAM framebuffer. */
static uint32_t diag_geometry_fingerprint(void){
 uint32_t h=2166136261u;
 int x,y;
 for(y=163;y<173;y++)for(x=59;x<238;x++){
  h^=screen[y*256+x];h*=16777619u;
 }
 return h;
}
static void verify_perf_strip(void){
 PerfScene sample={0};
 sample.total_frames=30;sample.current10=300;sample.average10=298;
 sample.minimum10=275;sample.maximum10=600;
 ui_perf_overlay(&sample);
 verify_text(3,0,"FPS 30.0 AVG 29.8 LOW 27.5 MAX 60.0",36,
             TEST_RGB(22,30,23),1);
 verify_coverage();
}
/* Verify the optimized capsule preserves its exact original edge and
 * corner silhouette in the unoverlaid header's left margin. */
static void verify_header_capsule(void){
 const uint16_t night=TEST_RGB(8,9,18),purple=TEST_RGB(18,11,23);
 const uint16_t ivory=TEST_RGB(31,30,27);
 assert(screen[5*256+167]==night);
 assert(screen[5*256+170]==purple);
 assert(screen[6*256+170]==ivory);
 assert(screen[8*256+167]==purple);
 assert(screen[8*256+168]==ivory);
 assert(screen[21*256+167]==night);
 assert(screen[21*256+170]==purple);
}
/* Compare actual visible meter pixels to the old design, including
 * zero, full, intermediate and saturated input through the home UI. */
static void verify_hunger_meter(unsigned value){
 const uint16_t white=TEST_RGB(31,31,31),purple=TEST_RGB(18,11,23);
 const uint16_t rose=TEST_RGB(30,13,22);
 unsigned bar=(value>100u?100u:value)*67u/100u;
 /* The numeric overlay owns x>=109 in rows 71..77; the unobscured
  * left 48 columns must be exactly the previous bar/edge pattern. */
 for(unsigned y=0;y<8u;y++)for(unsigned x=0;x<48u;x++){
  uint16_t expected=white;
  if(!x||!y||y==7u)expected=purple;
  else if(y>=3u&&y<=5u&&x>=2u&&x<bar+2u)expected=rose;
  assert(screen[(70u+y)*256u+61u+x]==expected);
 }
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
static void verify_save_warning_caption(void){
 /* Read the actual 5x7 framebuffer glyphs, including spaces and the last
  * exclamation mark: a colored footer or truncated warning is not enough. */
 static const char alphabet[]=" SAVEFILD.PROGNM!";
 static const uint8_t rows[][7]={
  {0,0,0,0,0,0,0}, {15,16,16,14,1,1,30}, {14,17,17,31,17,17,17},
  {17,17,17,17,17,10,4}, {31,16,16,30,16,16,31},
  {31,16,16,30,16,16,16}, {31,4,4,4,4,4,31},
  {16,16,16,16,16,16,31}, {30,17,17,17,17,17,30},
  {0,0,0,0,0,6,6}, {30,17,17,30,16,16,16},
  {30,17,17,30,20,18,17}, {14,17,17,17,17,17,14},
  {14,17,16,23,17,17,14}, {17,25,21,19,17,17,17},
  {17,27,21,21,17,17,17}, {4,4,4,4,4,0,4}
 };
 const char *caption="SAVE FAILED. PROGRESS IN RAM!";
 const uint16_t pink=(uint16_t)(0x8000u|31u|(19u<<5)|(25u<<10));
 const uint16_t night=(uint16_t)(0x8000u|8u|(9u<<5)|(18u<<10));
 for(unsigned i=0;caption[i];i++){
  const char *letter=strchr(alphabet,caption[i]);
  assert(letter);
  for(unsigned y=0;y<7u;y++)for(unsigned x=0;x<6u;x++){
   int ink=x<5u&&(rows[letter-alphabet][y]&(1u<<(4u-x)));
   assert(screen[(181u+y)*256u+10u+i*6u+x]==(ink?pink:night));
  }
 }
}
static void arcade_feedback_footer(void){
 /* Host-only reference image: the DS helper allocates no framebuffer. */
 static uint16_t before[256*192];
 Game g;Arcade a;
 uint32_t hashes[4];
 unsigned id,event,i;
 game_init(&g);g.unlocked=0xffffu;
 for(id=0;id<MBAI_GAMES;id++){
  game_arcade_start(&g,&a,(int)id);
  ui_draw(&g,&a,2,(int)id,0,0,1,"FEEDBACK");
  memcpy(before,screen,sizeof(before));
  ui_arcade_feedback(ARCADE_FEEDBACK_NONE);
  ui_arcade_feedback((ArcadeFeedbackEvent)99);
  assert(memcmp(before,screen,sizeof(before))==0);
  for(event=ARCADE_FEEDBACK_PROGRESS;event<=ARCADE_FEEDBACK_LOSS;event++){
   uint32_t hash=2166136261u;
   ui_arcade_feedback((ArcadeFeedbackEvent)event);
   verify_coverage();
   /* Exact comparison preserves headers, status/progress rows, every
    * target/hitbox picture, and the footer's existing accent rule. */
   assert(memcmp(before,screen,256u*178u*sizeof(screen[0]))==0);
   assert(memcmp(before+256u*178u,screen+256u*178u,
                 256u*14u*sizeof(screen[0]))!=0);
   for(i=256u*178u;i<256u*192u;i++){
    hash^=screen[i];hash*=16777619u;
   }
   hashes[event-1u]=hash;
   for(i=0;i<event-1u;i++)assert(hashes[i]!=hash);
   /* Expiry asks for an ordinary draw: every original footer/control
    * pixel returns, without retaining a temporary feedback surface. */
   ui_draw(&g,&a,2,(int)id,0,0,1,"FEEDBACK");
   assert(memcmp(before,screen,sizeof(before))==0);
  }
  ui_arcade_feedback(ARCADE_FEEDBACK_WIN);
  ui_save_warning(); /* A failed write takes priority over the win caption. */
  verify_save_warning_caption();verify_coverage();
  assert(memcmp(before,screen,256u*178u*sizeof(screen[0]))==0);
  ui_draw(&g,&a,2,(int)id,0,0,1,"FEEDBACK");
  assert(memcmp(before,screen,sizeof(before))==0);
 }
 /* Final outcomes use the same small strip after returning to the menu. */
 ui_draw(&g,&a,1,0,0,0,1,"RESULT");
 memcpy(before,screen,sizeof(before));
 ui_arcade_feedback(ARCADE_FEEDBACK_WIN);
 ui_save_warning();
 verify_save_warning_caption();verify_coverage();
 assert(memcmp(before,screen,256u*178u*sizeof(screen[0]))==0);
 ui_draw(&g,&a,1,0,0,0,1,"RESULT");
 assert(memcmp(before,screen,sizeof(before))==0);
 /* Once the transient result has expired, the caller can keep its
  * unresolved save warning visible without any active feedback event. */
 ui_save_warning();
 verify_save_warning_caption();verify_coverage();
 assert(memcmp(before,screen,256u*178u*sizeof(screen[0]))==0);
 ui_draw(&g,&a,1,0,0,0,1,"RESULT");
 assert(memcmp(before,screen,sizeof(before))==0);
}
static void bottle_safe_band(void){
 static const int values[]={-23,-22,-1,0,1,22,23};
 const uint16_t mint=(uint16_t)(0x8000u|22u|(30u<<5)|(23u<<10));
 const uint16_t marker=(uint16_t)(0x8000u|28u|(11u<<5)|(18u<<10));
 Game g;Arcade a;
 unsigned i,x,count=0;
 game_init(&g);game_arcade_start(&g,&a,0);
 a.balance=-60; /* Move the marker off the band to inspect its full width. */
 ui_draw(&g,&a,2,0,0,0,1,"BOTTLE");
 for(x=25;x<231u;x++){
  int safe=x>=117u&&x<=139u;
  assert((screen[130u*256u+x]==mint)==safe);
  if(screen[130u*256u+x]==mint)count++;
 }
 assert(count==23u);
 for(i=0;i<sizeof(values)/sizeof(values[0]);i++){
  int value=values[i];
  int center=128+(value<0?-((-value+1)/2):((value+1)/2));
  game_arcade_start(&g,&a,0);
  a.balance=(int16_t)(value-2);a.hold_frames=20;
  game_arcade_tick(&g,&a); /* First tick adds +2 drift, reaching value. */
  assert(a.balance==value);
  assert(a.hold_frames==(value>=-22&&value<=22?21:18));
  ui_draw(&g,&a,2,0,0,0,1,"BOTTLE");
  for(int px=center-2;px<=center+2;px++)
   assert(screen[127u*256u+(unsigned)px]==marker);
  assert(screen[127u*256u+(unsigned)(center-3)]!=marker);
  assert(screen[127u*256u+(unsigned)(center+3)]!=marker);
  assert((center>=117&&center<=139)==(value>=-22&&value<=22));
  verify_coverage();
 }
}
static void conversation_touch_bounds(void){
 int x,y;
 for(y=-1;y<=192;y++)for(x=-1;x<=256;x++){
  int mic=x>=84&&x<123&&y>=33&&y<46;
  int expected=-1,entry=x>=8&&x<192&&y>=31&&y<66&&!mic;
  assert(ui_mic_entry_touch(x,y)==mic);
  if(mic)assert(ui_nursery_touch(x,y)==-1&&!ui_room_entry_touch(x,y));
  if(x>=0&&x<256&&y>=0&&y<192){
   if(y>=176)expected=x<80?6:x>=176?7:-1;
   else for(int row=0;row<3;row++){
    int top=92+27*row;
    if(y>=top&&y<top+24){
     if(x>=8&&x<124)expected=2*row;
     else if(x>=132&&x<248)expected=2*row+1;
    }
   }
  }
  assert(ui_conversation_touch(x,y)==expected);
  assert(ui_conversation_entry_touch(x,y)==entry);
  if(entry){
   assert(!ui_room_entry_touch(x,y));
   assert(ui_nursery_touch(x,y)==-1);
   assert(!ui_prank_entry_touch(x,y));
   assert(!ui_home_navigation(x,y));
  }
 }
 assert(ui_conversation_touch(INT_MIN,INT_MAX)==-1);
 assert(ui_conversation_touch(INT_MAX,INT_MIN)==-1);
 assert(!ui_conversation_entry_touch(INT_MAX,INT_MIN));
}
static void conversation_layout(void){
 static const char *const prompts[6]={
  "HOW ARE YOU?","NEED ANYTHING?","REMEMBER?",
  "TELL ME MORE","I LIKE THAT","NEW SUBJECT"
 };
 const uint16_t ink=TEST_RGB(8,7,12),purple=TEST_RGB(18,11,23);
 const uint16_t white=TEST_RGB(31,31,31),mint=TEST_RGB(22,30,23);
 const uint16_t green=TEST_RGB(12,25,15);
 Game g,original;Arcade a={0},original_arcade;
 uint32_t first;
 game_init(&g);original=g;original_arcade=a;
 for(int selected=0;selected<6;selected++){
  char question[40];
  ui_draw(&g,&a,11,selected,0,0,1,"HELLO.\nI'M STILL HERE.\nLET'S TALK.");
  verify_coverage();
  verify_text(12,7,"TALK",4,white,2);
  snprintf(question,sizeof(question),"ASK: %s",prompts[selected]);
  verify_text(17,36,question,36,purple,1);
  verify_text(17,48,"HELLO.",36,ink,1);
  verify_text(17,59,"I'M STILL HERE.",36,ink,1);
  verify_text(17,70,"LET'S TALK.",36,ink,1);
  for(int card=0;card<6;card++){
   int x=card%2?132:8,y=92+27*(card/2);
   verify_text(x+10,y+8,prompts[card],17,ink,1);
   assert(screen[(y+4)*256+x+7]==(card==selected?mint:white));
   assert(screen[y*256+x]==(card==selected?green:purple));
  }
  verify_text(12,181,"B BACK",6,white,1);
  verify_text(89,181,"D-PAD CHOOSE",12,mint,1);
  verify_text(202,181,"A ASK",5,white,1);
  assert(memcmp(&g,&original,sizeof(g))==0);
  assert(memcmp(&a,&original_arcade,sizeof(a))==0);
 }
 ui_draw(&g,&a,11,0,0,0,1,"SELECTION");first=fingerprint();
 ui_draw(&g,&a,11,-1,0,0,1,"SELECTION");assert(fingerprint()==first);
 ui_draw(&g,&a,11,6,0,0,1,"SELECTION");assert(fingerprint()==first);
 ui_draw(&g,&a,11,INT_MAX,0,0,1,"SELECTION");assert(fingerprint()==first);
 /* The entry hint fits beside the existing needs hint and ROOM control. */
 ui_draw(&g,&a,0,0,0,0,1,"HELLO.");
 verify_text(45,36,"TALK",4,ink,1);
 verify_text(94,36,"MIC",3,ink,1);
 verify_text(126,35,"ALL GOOD",8,TEST_RGB(12,24,26),1);
 verify_text(203,38,"ROOM",4,ink,1);
 verify_coverage();
}
static void conversation_wrapping(void){
 static const struct {
  const char *message,*line[3];
 } cases[]={
  {0,{"","",""}}, {"",{"","",""}},
  {"I REMEMBER OUR NAP. THE ROOM WAS QUIET, AND I LIKED THAT VERY MUCH.",
   {"I REMEMBER OUR NAP. THE ROOM WAS","QUIET, AND I LIKED THAT VERY MUCH.",""}},
  {"ABCDEFGHIJKLMNOPQRS TUVWXYZ0123456789ABC DEFGHIJKLMNOPQRSTUV",
   {"ABCDEFGHIJKLMNOPQRS","TUVWXYZ0123456789ABC","DEFGHIJKLMNOPQRSTUV"}},
  {"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ012345678",
   {"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789","ABCDEFGHIJKLMNOPQRSTUVWXYZ012345678",""}},
  {"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
   {"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789","ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",""}},
  {"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\n9876543210ZYXWVUTSRQPONMLKJIHGFEDCBA\n"
   "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
   {"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789","9876543210ZYXWVUTSRQPONMLKJIHGFEDCBA",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"}},
  {"ONE\nTWO\nTHREE",{"ONE","TWO","THREE"}},
  {"ONE\r\nTWO\r\nTHREE",{"ONE","TWO","THREE"}},
  {"\nSECOND\nTHIRD",{"","SECOND","THIRD"}},
  {"ONE\n\nTHREE",{"ONE","","THREE"}},
  {"  HELLO  WORLD. \t\nNEXT ROW.",{"HELLO  WORLD.","NEXT ROW.",""}},
  {"lowercase becomes uppercase.",{"LOWERCASE BECOMES UPPERCASE.","",""}}
 };
 Game g;Arcade a={0};
 const uint16_t ink=TEST_RGB(8,7,12);
 game_init(&g);
 for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++){
  ui_draw(&g,&a,11,0,0,0,1,cases[i].message);
  for(int row=0;row<3;row++)verify_text(17,48+11*row,cases[i].line[row],36,ink,1);
  verify_coverage();
 }
 /* Two explicit 36-character care lines use the old nursery positions,
  * preserving the final glyph and keeping newlines out of the text grid. */
 ui_draw(&g,&a,0,0,0,0,1,
         "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\n9876543210ZYXWVUTSRQPONMLKJIHGFEDCBA");
 verify_text(17,47,"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",36,ink,1);
 verify_text(17,56,"9876543210ZYXWVUTSRQPONMLKJIHGFEDCBA",36,ink,1);
 ui_draw(&g,&a,0,0,0,0,1,"WE CAN TALK AFTER THE BOTTLE. I REMEMBER YOUR KIND WORDS TODAY.");
 verify_text(17,47,"WE CAN TALK AFTER THE BOTTLE. I",36,ink,1);
 verify_text(17,56,"REMEMBER YOUR KIND WORDS TODAY.",36,ink,1);
 verify_coverage();
}
/* The large care-card outline must agree with the current recommendation,
 * including critical exhaustion taking priority over simultaneous hunger.
 * Read its actual pixels and the adjacent 2x labels, not just a UI hash. */
static void care_recommendation_cards(void){
 static const int state[][4]={
  {35,75,75,-1}, {90,75,75,FEED}, {90,10,75,NAP},
  {35,75,20,CHAT}, {35,75,75,-1}
 };
 static const uint16_t edge[]={TEST_RGB(9,17,27),TEST_RGB(30,21,9),
                               TEST_RGB(12,25,15),TEST_RGB(30,13,22)};
 const uint16_t ink=TEST_RGB(8,7,12);
 Game g;Arcade a={0};game_init(&g);
 for(unsigned i=0;i<sizeof(state)/sizeof(state[0]);i++){
  Game before;
  g.hunger=(uint8_t)state[i][0];g.energy=(uint8_t)state[i][1];
  g.mood=(uint8_t)state[i][2];before=g;
  ui_draw(&g,&a,0,0,0,0,1,"A CARE REQUEST.");
  verify_coverage();
  for(int card=0;card<4;card++){
   int x=card%2?132:8,y=card<2?108:143;
   for(int py=4;py<26;py++)
    assert((screen[(y+py)*256+x+1]==edge[card])==(card==state[i][3]));
  }
  verify_text(50,111,"FEED",4,ink,2);
  verify_text(173,111,"CHAT",4,ink,2);
  verify_text(50,146,"PLAY",4,ink,2);
  verify_text(172,146,"NAP",3,ink,2);
  assert(memcmp(&before,&g,sizeof(g))==0);
 }
}
static void learning_save_warning_footer(void){
 static uint16_t before[256*192]; /* host-only reference; never in DS code */
 static const int views[]={0,10,11};
 Game g;Arcade a={0};
 game_init(&g);
 for(unsigned i=0;i<sizeof(views)/sizeof(views[0]);i++){
  ui_draw(&g,&a,views[i],0,0,0,1,"I'M LISTENING.");
  memcpy(before,screen,sizeof(before));
  ui_learning_save_warning();
  verify_text(10,181,"LEARNING NOT SAVED. KEPT IN RAM!",39,TEST_RGB(31,19,25),1);
  verify_coverage();
  assert(memcmp(before,screen,256u*178u*sizeof(screen[0]))==0);
  assert(memcmp(before+256u*178u,screen+256u*178u,256u*14u*sizeof(screen[0]))!=0);
  ui_save_warning(); /* Root gives game-save failure the highest priority. */
  verify_save_warning_caption();
  assert(memcmp(before,screen,256u*178u*sizeof(screen[0]))==0);
  ui_draw(&g,&a,views[i],0,0,0,1,"I'M LISTENING.");
  assert(memcmp(before,screen,sizeof(before))==0);
 }
}
static void observer_touch_and_keys(void){
 static const char alpha[]="QWERTYUIOPASDFGHJKL?ZXCVBNM .<";
 static const char numeric[]="1234567890+-*/()=?%.ZXCVBNM .<";
 assert(sizeof(alpha)==31u&&sizeof(numeric)==31u);
 for(unsigned i=0;i<30u;i++){
  assert(ui_observer_key(i)==alpha[i]);
  assert(ui_observer_key(32u+i)==numeric[i]);
 }
 assert(ui_observer_key(30u)=='\0'&&ui_observer_key(31u)=='\0');
 assert(ui_observer_key(62u)=='\0'&&ui_observer_key(UINT_MAX)=='\0');
 for(int y=-2;y<194;y++)for(int x=-2;x<258;x++){
  int expected=-1;
  if(x>=0&&x<256&&y>=176&&y<192)expected=30+x/64;
  else if(x>=8&&x<248&&y>=165&&y<176&&(x-8)%60<56)
   expected=36+(x-8)/60;
  else if(x>=130&&x<181&&y>=88&&y<100)expected=35;
  else if(x>=216&&x<248&&y>=100&&y<115)expected=34;
  else for(int key=0;key<30;key++){
   int kx=8+(key%10)*24,ky=118+(key/10)*16;
   if(x>=kx&&x<kx+22&&y>=ky&&y<ky+14)expected=key;
  }
  assert(ui_observer_touch(x,y)==expected);
  assert(ui_observer_entry_touch(x,y)==(x>=80&&x<124&&y>=5&&y<21));
 }
 assert(ui_observer_touch(INT_MIN,INT_MAX)==-1);
 assert(ui_observer_touch(INT_MAX,INT_MIN)==-1);
 assert(!ui_observer_entry_touch(INT_MAX,INT_MIN));
 for(int y=-2;y<194;y++)for(int x=-2;x<258;x++){
  int expected=0;
  if(x>=0&&x<256&&y>=176&&y<192)expected=4;
  else if(x>=0&&x<256&&y>=113&&y<128){
   if(x>=8&&x<44)expected=5;
   else if(x>=48&&x<84)expected=6;
   else if(x>=205&&x<248)expected=7;
  }else if(y>=131&&y<161){
   if(x>=8&&x<84)expected=1;
   if(x>=90&&x<166)expected=2;
   if(x>=172&&x<248)expected=3;
  }
  assert(ui_mic_touch(x,y)==expected);
 }
}
static void observer_layout(void){
 const uint16_t ink=TEST_RGB(8,7,12),white=TEST_RGB(31,31,31);
 const uint16_t phosphor=TEST_RGB(24,31,26),green=TEST_RGB(12,25,15);
 Game g,before;Arcade a={0};
 uint32_t monitor;
 char input[32],reply[144]; /* Deliberately no terminators: exact read limits. */
 const char *line="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
 game_init(&g);before=g;
 memcpy(input,line,sizeof(input));
 for(unsigned i=0;i<sizeof(reply);i++)reply[i]=line[i%36u];
 ui_observer_draw(&g,1,0,input,reply,0u,0u,100u);
 verify_coverage();
 verify_text(12,7,"MONITOR",7,white,2);
 verify_text(17,34,"WARDEN",6,phosphor,1);
 verify_text(144,91,"MIC",3,ink,1);
 verify_text(18,167,"STATUS",6,ink,1);
 verify_text(81,167,"NEEDS",5,ink,1);
 verify_text(144,167,"BOND",4,ink,1);
 verify_text(204,167,"HELP",4,ink,1);
 assert(strcmp(ui_observer_quick_prompt(0u,0u),"STATUS")==0);
 assert(strcmp(ui_observer_quick_prompt(1u,0u),"HELLO")==0);
 assert(!*ui_observer_quick_prompt(0u,4u));
 verify_text(167,34,"BRACER 100%",11,phosphor,1);
 verify_text(190,92,"FOUND",9,phosphor,1);
 verify_text(17,104,"ABCDEFGHIJKLMNOPQRSTUVWXYZ012345",32,ink,1);
 for(int row=0;row<4;row++)verify_text(17,45+11*row,line,36,phosphor,1);
 verify_text(137,34,"...",3,TEST_RGB(31,28,13),1);
 /* The microphone lab has a complete offline/bounded fallback layout,
  * independent of whether an emulator actually supports microphone input. */
 ui_mic_draw(&g,1,"HELLO","NO MIC INPUT. USE KEYBOARD.","",60u,4u,0,
             "HELLO",2u,0,0u,0u,0u);
 verify_coverage();
 verify_text(12,7,"MIC LAB",7,white,2);
 verify_text(187,36,"WARDEN",6,ink,1);
 verify_text(13,117,"PREV",4,ink,1);
 verify_text(53,117,"NEXT",4,ink,1);
 verify_text(94,117,"HELLO",17,ink,1);
 verify_text(217,117,"DEL",3,ink,1);
 verify_text(183,166,"EX 2/2",7,white,1);
 verify_text(25,139,"A TRAIN",7,ink,1);
 verify_text(106,139,"X LISTEN",8,ink,1);
 verify_text(192,139,"Y USE",5,ink,1);
 /* The confirmation changes its visible label, not the other controls;
  * a fully empty library must display NO LESSONS, not stale labels. */
 {
  uint32_t ordinary=fingerprint();
  ui_mic_draw(&g,1,"HELLO","PRESS DEL AGAIN","",0u,0u,0,
              "",0u,1,1u,0u,0u);
  verify_coverage();
  verify_text(94,117,"NO LESSONS",17,ink,1);
  verify_text(209,117,"SURE?",5,ink,1);
  verify_text(183,166,"EX 0/2",7,white,1);
  assert(fingerprint()!=ordinary);
  ui_mic_draw(&g,1,"HELLO","WAIT FOR QUIET CALIBRATION.","",60u,4u,1,
              "HELLO",1u,0,1u,1u,0u);
  verify_coverage();
  verify_text(187,36,"BABY",4,ink,1);
  verify_text(28,139,"A STOP",6,ink,1);
  verify_text(110,139,"X STOP",6,ink,1);
 }
 ui_voice_save_warning();
 verify_text(10,181,"VOICE LESSONS NOT SAVED. IN RAM!",39,
             TEST_RGB(31,19,25),1);
 ui_observer_draw(&g,1,0,input,reply,0u,0u,100u);
 verify_coverage();
 monitor=fingerprint();
 for(unsigned page=0;page<2u;page++)for(unsigned selected=0;selected<30u;selected++){
  unsigned key=selected+page*32u;
  ui_observer_draw(&g,1,1,"WHY?","ONE\nTWO\nTHREE\nFOUR",key,2u,9u);
  verify_coverage();
  assert(fingerprint()!=monitor);
  verify_text(12,7,"BABY",4,white,2);
  verify_text(17,34,"NURSERY VOICE",13,phosphor,1);
  verify_text(21,167,"HELLO",5,ink,1);
  verify_text(144-3,167,"DREAM",5,ink,1);
  verify_text(204,167,"MORE",4,ink,1);
  verify_text(167,34,"BRACER 9%",11,phosphor,1);
  verify_text(190,92,"HOSTILE",9,phosphor,1);
  verify_text(17,104,"WHY?",32,ink,1);
  verify_text(17,45,"ONE",36,phosphor,1);
  verify_text(17,56,"TWO",36,phosphor,1);
  verify_text(17,67,"THREE",36,phosphor,1);
  verify_text(17,78,"FOUR",36,phosphor,1);
  verify_text(137,34,"",3,TEST_RGB(31,28,13),1);
  verify_text(223,104,page?"ABC":"123",3,ink,1);
  for(unsigned k=0;k<30u;k++){
   int x=8+(int)(k%10u)*24,y=118+(int)(k/10u)*16;
   char label[2]={ui_observer_key(k+page*32u),'\0'};
   if(label[0]==' ')verify_text(x+5,y+3,"SP",2,ink,1);
   else verify_text(x+8,y+3,label,1,ink,1);
   assert((screen[(y+11)*256+x+2]==green)==(k==selected));
  }
  assert(memcmp(&g,&before,sizeof(g))==0);
 }
 /* Empty/invalid display inputs remain bounded and readable. */
 ui_observer_draw(&g,0,0,0,0,UINT_MAX,UINT_MAX,UINT_MAX);
 verify_text(17,104,"",32,ink,1);
 for(int row=0;row<4;row++)verify_text(17,45+11*row,"",36,phosphor,1);
 verify_text(190,92,"REVEALED",9,phosphor,1);
 verify_text(167,34,"BRACER 100%",11,phosphor,1);
 verify_coverage();
 /* Longer caller buffers stop at the same visible input/reply limits. */
 {
  char longer_input[80],longer_reply[200];
  memset(longer_input,'A',sizeof(longer_input)-1u);longer_input[79]='\0';
  memset(longer_reply,'B',sizeof(longer_reply)-1u);longer_reply[199]='\0';
  ui_observer_draw(&g,1,0,longer_input,longer_reply,29u,1u,50u);
  verify_text(17,104,"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",32,ink,1);
  for(int row=0;row<4;row++)
   verify_text(17,45+11*row,"BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",36,phosphor,1);
  verify_text(137,34,"...",3,TEST_RGB(31,28,13),1);
  verify_text(190,92,"WATCHING",9,phosphor,1);
  verify_coverage();
 }
 /* The entry badge does not displace titles or stored status fields. */
 for(int view=0;view<=11;view+=11){
  ui_draw(&g,&a,view,0,0,0,1,"TEST");
  verify_text(12,7,view?"TALK":"CARE",4,white,2);
  verify_text(93,10,"CPU",3,ink,1);
  verify_text(138,11,"SD",3,TEST_RGB(22,30,23),1);
  verify_text(173,10,"D1",4,ink,1);
  verify_text(214,10,"$8",5,ink,1);
 }
}
static void observer_overlays(void){
 static uint16_t before[256*192];
 Game g;Arcade a={0};game_init(&g);
 for(int view=0;view<=8;view+=8){
  ui_draw(&g,&a,view,0,0,0,1,"TEST");
  memcpy(before,screen,sizeof(before));
  ui_observer_alert("WARDEN: CAREFUL WITH THE BABY.");
  assert(memcmp(before,screen,256u*178u*sizeof(screen[0]))==0);
  verify_text(10,181,"WARDEN: CAREFUL WITH THE BABY.",39,TEST_RGB(16,29,29),1);
  ui_observer_alert("CARETAKER HIT!\nCHECK WARDEN FOR INTEGRITY.");
  verify_text(10,181,"CARETAKER HIT!",39,TEST_RGB(16,29,29),1);
  ui_observer_guard_prompt(40u);
  verify_text(12,7,view?"TOYBOX":"CARE",view?6:4,TEST_RGB(31,31,31),2);
  verify_text(88,9,"GUARD",5,TEST_RGB(8,7,12),1);
  verify_text(10,181,"WINDUP! TAP GUARD / RIGHT",39,TEST_RGB(16,29,29),1);
  assert(memcmp(before+256u*21u,screen+256u*21u,
                256u*157u*sizeof(screen[0]))==0);
  ui_observer_save_warning();
  verify_text(10,181,"SHELTER MEMORY NOT SAVED. IN RAM!",39,TEST_RGB(31,19,25),1);
  ui_save_warning();
  verify_save_warning_caption();
  verify_coverage();
  ui_draw(&g,&a,view,0,0,0,1,"TEST");
  assert(memcmp(before,screen,sizeof(before))==0);
 }
}
int main(void){
 Game g;Arcade a={0};unsigned i,changed=0;
 game_init(&g);ui_init(screen);
 for(i=256u*192u;i<256u*256u;i++)screen[i]=0x5a5au;
 conversation_touch_bounds();
 conversation_layout();
 conversation_wrapping();
 care_recommendation_cards();
 learning_save_warning_footer();
 observer_touch_and_keys();
 observer_layout();
 observer_overlays();
 ui_draw(&g,&a,0,0,0,0,1,"TELL HR I'M TEETHING.");
 for(i=0;i<256u*192u;i++)if(screen[i])changed++;
 assert(changed>30000);
 /* Touch-only users must see and activate a distinct SAVE/LOAD
  * option without accidentally starting the game or initializing FAT. */
 {
  int x,y;
  uint16_t left,right;
  assert(ui_title_save_touch(159,140));
  assert(ui_title_save_touch(236,167));
  assert(ui_title_save_touch(199,154));
  assert(!ui_title_save_touch(158,154));
  assert(!ui_title_save_touch(237,154));
  assert(!ui_title_save_touch(199,139));
  assert(!ui_title_save_touch(199,168));
  assert(!ui_title_save_touch(128,115)); /* existing START touch */
  assert(!ui_title_save_touch(60,154));  /* yellow START card */
  for(y=0;y<192;y++)for(x=0;x<256;x++){
   int expected=x>=159&&x<237&&y>=140&&y<168;
   assert(ui_title_save_touch(x,y)==expected);
  }
  ui_draw(&g,&a,5,0,0,0,0,"WELCOME!");
  verify_coverage();
  left=screen[155u*256u+30u];
  right=screen[155u*256u+163u];
  assert(left!=right); /* yellow START and mint SAVE cards */
 }
 /* Polished care screen retains readable numbers and action hitboxes.
  * A changed hunger value must update the real rendered meter/number. */
 {
  uint32_t initial=fingerprint();
  g.hunger=13;
  ui_draw(&g,&a,0,0,0,0,1,"TELL HR I'M TEETHING.");
  verify_coverage();
  assert(fingerprint()!=initial);
  assert(screen[71u*256u+120u]!=screen[71u*256u+116u] ||
         screen[75u*256u+116u]!=screen[75u*256u+112u]);
  g.hunger=80;
  ui_draw(&g,&a,0,0,0,0,1,"TELL HR I'M TEETHING.");
  verify_coverage();
 }
 /* Guidance must actually appear on the native 256x192 nursery bitmap,
  * not merely change a non-visible model property. The same hitboxes
  * and framebuffer coverage must survive all recommendation changes. */
 {
  uint32_t normal,feed,nap,chat;
  game_init(&g);
  ui_draw(&g,&a,0,0,0,0,1,"NURSERY");
  verify_coverage();normal=care_hint_fingerprint();
  g.hunger=90;
  ui_draw(&g,&a,0,0,0,0,1,"NURSERY");
  verify_coverage();feed=care_hint_fingerprint();
  g.energy=9;
  ui_draw(&g,&a,0,0,0,0,1,"NURSERY");
  verify_coverage();nap=care_hint_fingerprint();
  g.hunger=35;g.energy=80;g.mood=25;
  ui_draw(&g,&a,0,0,0,0,1,"NURSERY");
  verify_coverage();chat=care_hint_fingerprint();
  assert(normal!=feed&&feed!=nap&&nap!=chat&&chat!=normal);
  game_init(&g);
 }
 /* Selected locked cabinets display exact win/cash/Favor requirements
  * on the EXISTING 256x192 page, in the free strip between arrows.
  * Unlocking replaces the prompt with a clear PLAY-ready indicator. */
 {
  uint32_t locked,ready;
  unsigned wins,cash,favors;
  assert(game_unlock_requirements(8,&wins,&cash,&favors));
  assert(wins==2u&&cash==10u&&favors==1u);
  assert(game_unlock_requirements(15,&wins,&cash,&favors));
  assert(wins==16u&&cash==24u&&favors==4u);
  ui_draw(&g,&a,1,8,0,0,1,"ARCADE");
  verify_coverage();locked=arcade_goal_fingerprint();
  g.unlocked|=(uint16_t)(1u<<8);
  ui_draw(&g,&a,1,8,0,0,1,"ARCADE");
  verify_coverage();ready=arcade_goal_fingerprint();
  assert(locked!=ready);
  /* Readable current WINS and FAVORS should change as the player earns
   * rewards, without buying anything or changing the selected cabinet. */
  g.unlocked=255u;
  ui_draw(&g,&a,1,8,0,0,1,"ARCADE");
  verify_coverage();locked=arcade_progress_fingerprint();
  g.wins+=6u;g.favors+=3u;
  ui_draw(&g,&a,1,8,0,0,1,"ARCADE");
  verify_coverage();ready=arcade_progress_fingerprint();
  assert(locked!=ready);
  g.wins-=6u;g.favors-=3u;
 }
 assert(ui_nursery_touch(25,131)==FEED);
 assert(ui_nursery_touch(175,131)==CHAT);
 assert(ui_nursery_touch(25,159)==PLAY);
 assert(ui_nursery_touch(175,159)==NAP);
 assert(ui_nursery_touch(0,0)==-1);
 assert(ui_home_navigation(35,183)==1);
 assert(ui_home_navigation(225,183)==2);
 assert(ui_home_navigation(130,183)==0);
 /* Room entry lives inside the CARE speech panel, never on Toybox,
  * FEED or CHAT. Native and virtual stylus use the SAME hitboxes. */
 ui_draw(&g,&a,0,0,0,0,1,"FPS TEST");
 verify_perf_strip();
 ui_draw(&g,&a,0,0,0,0,1,"GEOMETRY TEST");
 verify_header_capsule();verify_coverage();
 {
  Game meter_game=g;
  for(unsigned hunger=0;hunger<=100u;hunger++){
   meter_game.hunger=(uint8_t)hunger;
   ui_draw(&meter_game,&a,0,0,0,0,1,"GEOMETRY TEST");
   verify_hunger_meter(hunger);
  }
  verify_coverage();
 }
 assert(ui_room_entry_touch(220,40));
 assert(!ui_room_entry_touch(220,55));
 assert(!ui_room_entry_touch(120,40));
 assert(!ui_room_entry_touch(243,40));
 assert(ui_room_touch(220,54)==7); /* explicit in-room SAVE */
 assert(ui_room_touch(198,49)==7);
 assert(ui_room_touch(243,62)==7);
 assert(ui_room_touch(197,55)==-1);
 assert(ui_room_touch(244,55)==-1);
 assert(ui_room_touch(220,48)==-1);
 assert(ui_room_touch(220,63)==-1);
 assert(ui_room_touch(28,90)==0); /* window */
 assert(ui_room_touch(180,90)==1); /* lamp */
 assert(ui_room_touch(28,139)==2); /* teddy */
 assert(ui_room_touch(180,139)==3); /* crib */
 assert(ui_room_touch(129,139)==-1); /* column gap */
 assert(ui_room_touch(30,117)==-1); /* row gap */
 assert(ui_room_touch(8,163)==8&&ui_room_touch(123,174)==8);
 assert(ui_room_touch(132,163)==9&&ui_room_touch(247,174)==9);
 assert(ui_room_touch(128,166)==-1);
 assert(ui_room_touch(7,163)==-1&&ui_room_touch(248,174)==-1);
 assert(ui_room_touch(120,162)==-1&&ui_room_touch(120,175)==-1);
 assert(ui_room_touch(128,183)==4); /* return */
 assert(ui_room_touch(180,183)==5); /* native stylus sound toggle */
 assert(ui_room_touch(222,183)==6); /* native stylus USE */
 assert(ui_room_touch(150,183)==5); /* sound button left edge */
 assert(ui_room_touch(207,183)==5); /* sound button right edge */
 assert(ui_room_touch(248,183)==4); /* outside USE button */
 assert(ui_room_touch(0,0)==-1);
 assert(ui_room_touch(256,95)==-1);
 assert(ui_room_touch(40,192)==-1);
 assert(ui_peek_touch(40,119)==1&&ui_peek_touch(215,164)==1);
 assert(ui_peek_touch(39,119)==0&&ui_peek_touch(216,164)==0);
 assert(ui_peek_touch(128,183)==2&&ui_peek_touch(256,183)==0);
 {
  NurseryPeek peek={0};uint32_t wait,open,win;
  peek.phase=PEEK_WAIT;ui_peek_draw(&g,&peek,1);verify_coverage();wait=fingerprint();
  peek.phase=PEEK_OPEN;ui_peek_draw(&g,&peek,1);verify_coverage();open=fingerprint();
  peek.phase=PEEK_WON;peek.round=3;ui_peek_draw(&g,&peek,1);verify_coverage();win=fingerprint();
  assert(wait!=open&&open!=win&&wait!=win);
  {
   Game before=g;
   uint32_t scare,corrupt,locked;
   peek.phase=PEEK_SCARE;ui_peek_draw(&g,&peek,1);
   verify_coverage();scare=fingerprint();
   peek.phase=PEEK_CORRUPT;ui_peek_draw(&g,&peek,1);
   verify_coverage();corrupt=fingerprint();
   peek.phase=PEEK_LOCKED;ui_peek_draw(&g,&peek,1);
   verify_coverage();locked=fingerprint();
   assert(scare!=corrupt&&corrupt!=locked&&scare!=locked&&locked!=open);
   assert(memcmp(&g,&before,sizeof(g))==0);
  }
 }
 {
  uint32_t night,day,lamp_off,light_on,teddy;
  ui_draw(&g,&a,10,16,0,0,1,"ROOM TOUR");
  verify_coverage();night=fingerprint();
  ui_draw(&g,&a,10,24,0,0,1,"ROOM TOUR"); /* daylight */
  verify_coverage();day=fingerprint();
  assert(day!=night);
  ui_draw(&g,&a,10,1|8,0,0,1,"LIGHTS OFF");
  verify_coverage();lamp_off=fingerprint();
  ui_draw(&g,&a,10,1|8|16,0,0,1,"LIGHTS ON");
  verify_coverage();light_on=fingerprint();
  assert(light_on!=lamp_off);
  ui_draw(&g,&a,10,2|16,0,0,1,"TEDDY PROMOTED");
  verify_coverage();teddy=fingerprint();
  assert(teddy!=light_on);
  /* Room sound is visible and controllable independently of Toybox:
   * 32=globally muted, drawn inside the existing footer with no overlaps. */
  {
   uint32_t sound_on,sound_off;
   ui_draw(&g,&a,10,2|16,0,0,1,"TEDDY PROMOTED");
   verify_coverage();sound_on=fingerprint();
   ui_draw(&g,&a,10,2|16|32,0,0,1,"TEDDY PROMOTED");
   verify_coverage();sound_off=fingerprint();
   assert(sound_on!=sound_off);
  }
  /* The crib itself explains the conditional bonus, not hidden knowledge.
   * Compare the SAME crib card under lit and quiet-night conditions. */
  {
   uint32_t quiet,day,lit;
   ui_draw(&g,&a,10,3,0,0,1,"QUIET NIGHT");
   verify_coverage();quiet=fingerprint();
   ui_draw(&g,&a,10,3|8,0,0,1,"DAYLIGHT");
   verify_coverage();day=fingerprint();
   ui_draw(&g,&a,10,3|16,0,0,1,"LAMP ON");
   verify_coverage();lit=fingerprint();
   assert(quiet!=day&&quiet!=lit);
  }
  ui_draw(&g,&a,0,0,0,0,1,"BACK IN THE NURSERY");
  verify_coverage();
 }
 assert(ui_prank_entry_touch(125,184));
 assert(!ui_prank_entry_touch(194,45));
 assert(!ui_prank_entry_touch(225,183));
 assert(ui_roughhouse_touch(48,160)==1); /* select SLAP */
 assert(ui_roughhouse_touch(191,160)==2); /* select KICK */
 assert(ui_roughhouse_touch(130,109)==4); /* use selected toy */
 assert(ui_roughhouse_touch(95,83)==4);
 assert(ui_roughhouse_touch(159,134)==4);
 assert(ui_roughhouse_touch(94,110)==0); /* margins never slap doll */
 assert(ui_roughhouse_touch(160,110)==0);
 assert(ui_roughhouse_touch(38,110)==0);
 assert(ui_roughhouse_touch(128,182)==3); /* back */
 assert(ui_roughhouse_touch(128,67)==0);
 /* Footer hitboxes support real and virtual pen even with KEY_A masked.
  * Center readout acts as a deliberate NEXT, not an accidental SKIP. */
 assert(ui_egg_cinematic_touch(41,183)==1);
 assert(ui_egg_cinematic_touch(83,176)==1);
 assert(ui_egg_cinematic_touch(84,176)==2);
 assert(ui_egg_cinematic_touch(150,182)==2);
 assert(ui_egg_cinematic_touch(171,190)==2);
 assert(ui_egg_cinematic_touch(172,181)==3);
 assert(ui_egg_cinematic_touch(241,180)==3);
 assert(ui_egg_cinematic_touch(128,100)==3);
 assert(ui_egg_cinematic_touch(15,35)==3);
 assert(ui_egg_cinematic_touch(128,165)==0);
 assert(ui_egg_cinematic_touch(128,23)==0);
 assert(ui_egg_cinematic_touch(-1,180)==0);
 assert(ui_egg_cinematic_touch(256,180)==0);
 assert(ui_egg_cinematic_touch(100,192)==0);
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
 assert(ui_arcade_help_touch(193,57)&&ui_arcade_help_touch(247,70));
 assert(!ui_arcade_help_touch(192,57)&&!ui_arcade_help_touch(248,70));
 assert(ui_arcade_pause_touch(112,8)&&ui_arcade_pause_touch(135,22));
 assert(!ui_arcade_pause_touch(111,8)&&!ui_arcade_pause_touch(136,22));
 assert(ui_arcade_guide_touch(8,136)==1&&ui_arcade_guide_touch(123,164)==1);
 assert(ui_arcade_guide_touch(132,136)==2&&ui_arcade_guide_touch(247,164)==2);
 assert(ui_arcade_guide_touch(128,150)==0&&ui_arcade_guide_touch(128,183)==3);
 assert(ui_arcade_guide_touch(256,183)==0);
 assert(ui_arcade_guide_touch(61,68)==4&&ui_arcade_guide_touch(238,81)==4);
 assert(ui_arcade_guide_touch(60,68)==0&&ui_arcade_guide_touch(239,81)==0);
 {
  uint32_t guides[MBAI_GAMES],pause;
  Game live=g,practice;
  Arcade trial;
  for(unsigned id=0;id<MBAI_GAMES;id++){
   ui_arcade_guide_draw(&live,id,0,0,1);verify_coverage();
   guides[id]=fingerprint();
   for(unsigned j=0;j<id;j++)assert(guides[id]!=guides[j]);
   assert(game_arcade_practice_start(&live,&practice,&trial,(int)id));
   ui_draw(&live,&trial,2,(int)id,0,0,1,"PRACTICE");
   verify_text(12,7,"PRACTICE",8,TEST_RGB(31,31,31),2);
   verify_coverage();
   assert(screen[73*256+199]==TEST_RGB(12,24,26));
   trial.timer=trial.timer_limit/4u;ui_arcade_clock(&trial);
   assert(screen[73*256+96]==TEST_RGB(30,13,22));
   assert(screen[73*256+199]==TEST_RGB(26,25,27));
   trial.timer=0;ui_arcade_clock(&trial);
   assert(screen[73*256+96]==TEST_RGB(26,25,27));
   verify_coverage();
  }
  ui_arcade_guide_draw(&live,10u,0,ARCADE_PRACTICE_STANDARD,1);verify_coverage();
  verify_text(67,72,"Y MODE: STANDARD",16,TEST_RGB(8,7,12),1);
  ui_arcade_guide_draw(&live,10u,0,ARCADE_PRACTICE_RELAXED,1);verify_coverage();
  verify_text(67,72,"Y MODE: RELAXED",15,TEST_RGB(8,7,12),1);
  ui_arcade_guide_draw(&live,10u,0,ARCADE_PRACTICE_EXPERT,1);verify_coverage();
  verify_text(67,72,"Y MODE: EXPERT",14,TEST_RGB(8,7,12),1);
  assert(game_arcade_practice_start_mode(&live,&practice,&trial,10,ARCADE_PRACTICE_EXPERT));
  game_arcade_input(&practice,&trial,trial.target);
  assert(trial.reverse==1u);
  ui_draw(&live,&trial,2,10,0,0,1,"PRACTICE");
  verify_text(95,91,"-",1,TEST_RGB(18,11,23),1);
  verify_coverage();
  ui_arcade_guide_draw(&live,15u,1,1,1);verify_coverage();pause=fingerprint();
  {
   ArcadeRecord record={5,3,2,120};
   ui_arcade_record_draw(&record);
   verify_text(10,167,"CLEARS 3/5 BEST 120T CLEAN 2",27,TEST_RGB(8,7,12),1);
   verify_coverage();
  }
  assert(pause!=guides[15]);
  ui_arcade_feedback_mode(ARCADE_FEEDBACK_WIN,1);
  verify_text(10,181,"PRACTICE CLEARED! NO REWARD.",28,TEST_RGB(31,28,13),1);
  verify_coverage();
 }
 {
  uint32_t hand,foot;
  ui_draw(&g,&a,8,5,0,0,1,"TOYBOX");
  verify_coverage();
  hand=fingerprint();
  ui_draw(&g,&a,8,37,0,0,1,"TOYBOX");
  verify_coverage();
  foot=fingerprint();
  assert(hand!=foot); /* SELECTED mode must be unmistakable */
  ui_draw(&g,&a,8,37|(1<<16),0,0,1,"TOYBOX");
  verify_coverage();
  assert(foot!=fingerprint()); /* SFX ON/OFF has visible UI feedback */
  ui_draw(&g,&a,8,69,0,0,1,"TOYBOX"); /* one hit plus first stress */
  verify_coverage();
  assert(foot!=fingerprint());
  foot=fingerprint();
  ui_draw(&g,&a,8,(4<<6)|43,0,0,1,"TOYBOX"); /* stage-two crack */
  verify_coverage();
  assert(foot!=fingerprint());
  ui_draw(&g,&a,9,EGG_PUSH_FRAME,0,0,1,"EGG");
  verify_coverage();
  {
   uint32_t playing=fingerprint();
   ui_draw(&g,&a,9,EGG_PUSH_FRAME|EGG_FILM_PAUSED_FLAG,0,0,1,"EGG");
   verify_coverage();
   assert(playing!=fingerprint()); /* visible PAUSED/RESUME state */
  }
  ui_draw(&g,&a,9,EGG_SCARE_FRAME,0,0,1,"EGG");
  verify_coverage();
  /* Large containment alert and diagnostic icon have distinct high
   * contrast states; progress meter sample coordinates stay untouched. */
  {
   uint16_t danger=screen[57u*256u+20u];
   uint16_t lit=screen[137u*256u+31u];
   uint16_t warning_icon=screen[96u*256u+201u];
   assert(lit==screen[137u*256u+187u]);
   ui_draw(&g,&a,9,0,0,0,1,"EGG");
   verify_coverage();
   assert(screen[57u*256u+20u]!=danger ||
          screen[96u*256u+201u]!=warning_icon);
   assert(lit!=screen[137u*256u+187u]);
  }
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
 a.id=4;a.active=1;a.phase=0;a.elapsed=0;a.timer=90;a.hold_frames=90;
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
  ui_touch_diag_update(1320u,426u);
  assert(screen[index]!=canvas);
  ui_touch_diag_update(1320u,426u); /* same state should be a no-op */
  ui_touch_observe(0,0,0,3);
  ui_touch_diag_update(1320u,426u);
  assert(screen[index]==canvas);
  ui_touch_observe(1,120,130,4);
  ui_touch_diag_update(1320u,426u);
  assert(screen[index]!=canvas);
  ui_touch_observe(1,220,170,4);
  ui_touch_diag_update(1320u,426u);
  assert(screen[index]==canvas); /* old crosshair erased on movement */
  ui_touch_observe(0,0,0,4);
  ui_touch_diag_update(1320u,426u);
 }
 ui_draw(&g,&a,7,0,0,0,1,"TOUCH DEBUG");
 ui_touch_diag_update(1320u,426u);
 /* Live 3D submission telemetry must be visible, bounded to its own
  * strip, and redrawn only when a budget counter changes. */
 {
  uint32_t initial,verts_changed,polys_changed;
  ui_touch_diag_update(1320u,426u);
  initial=diag_geometry_fingerprint();
  ui_touch_diag_update(1321u,426u);
  verts_changed=diag_geometry_fingerprint();
  assert(verts_changed!=initial);
  ui_touch_diag_update(1321u,426u); /* same input is stable */
  assert(diag_geometry_fingerprint()==verts_changed);
  ui_touch_diag_update(1321u,1900u); /* polygon budget exceeded */
  polys_changed=diag_geometry_fingerprint();
  assert(polys_changed!=verts_changed);
  verify_coverage();
 }
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
  /* Three-page memory book has actual original egg illustration on
   * unlock, not two empty rectangles. It must also be visibly different
   * from its still-sealed state, without repainting beyond VRAM bounds. */
  ui_draw(&g,&a,6,2,0,0,1,"ALBUM");
  verify_coverage();
  hashes[0]=fingerprint();
  g.ending_mask|=(uint16_t)(1u<<END_EGG);
  ui_draw(&g,&a,6,2,0,0,1,"ALBUM");
  verify_coverage();
  assert(hashes[0]!=fingerprint());
  ui_draw(&g,&a,4,0,END_EGG,0,1,"ENDING");
  verify_coverage();
  /* The egg icon on the standalone ending card uses at least six
   * separate original colors plus the dark cavity, not a flat box. */
  {
   uint16_t shades[32]={0};unsigned used=0,x,y,k;
   for(y=76;y<118;y++)for(x=97;x<162;x++){
    uint16_t pixel=screen[y*256+x];
    for(k=0;k<used;k++)if(shades[k]==pixel)break;
    if(k==used&&used<32)shades[used++]=pixel;
   }
   assert(used>=8);
  }
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
 arcade_feedback_footer();
 bottle_safe_band();
 puts("PASS: 16 unique cabinets, bounded feedback footer, accurate Bottle safe band, touchscreen bounds and full coverage");
 return 0;
}
