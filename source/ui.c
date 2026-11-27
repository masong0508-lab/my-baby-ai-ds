/* My Baby A.I.! original touchscreen graphics.
 * 256x192 16-bit bitmap rendered directly into dedicated sub-screen VRAM.
 * No heap allocation, texture imports, or offscreen framebuffer.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"
#include "roughhouse.h"
#define W 256
#define H 192
#define RGB(r,g,b) ((uint16_t)(0x8000u | (r) | ((g)<<5) | ((b)<<10)))
#define IVORY RGB(31,30,27)
#define WHITE RGB(31,31,31)
#define INK RGB(8,7,12)
#define MUTED RGB(17,13,19)
#define PURPLE RGB(18,11,23)
#define LILAC RGB(27,22,30)
#define PINK RGB(31,19,25)
#define DEEP_PINK RGB(28,11,18)
#define BLUSH RGB(31,26,27)
#define TEAL RGB(12,24,26)
#define MINT RGB(22,30,23)
#define GREEN RGB(12,25,15)
#define LEMON RGB(31,28,13)
#define GOLD RGB(30,21,9)
#define SKY RGB(20,26,31)
#define BLUE RGB(9,17,27)
#define SHADOW RGB(22,19,24)
#define GRAY RGB(26,25,27)
#define NIGHT RGB(8,9,18)
#define SILVER RGB(23,25,30)
#define ROSE RGB(30,13,22)
#define AQUA RGB(16,29,29)
#define PAPER RGB(30,28,25)
#define PAPER_EDGE RGB(26,23,22)
#define PAPER_SHADE RGB(28,26,24)
static uint16_t screen_accent=TEAL;
static volatile uint16_t *pixels;
/* 11x11 cursor backup: 242 bytes. Restores exactly the underlying UI.
 * No additional framebuffers or per-frame allocations. */
#define CURSOR_W 11
static uint16_t cursor_bg[CURSOR_W*CURSOR_W];
static int cursor_visible,cursor_x,cursor_y,cursor_pressed;
static void cursor_erase(void){
 int x,y;
 if(!cursor_visible||!pixels)return;
 for(y=0;y<CURSOR_W;y++)for(x=0;x<CURSOR_W;x++){
  int px=cursor_x+x-5,py=cursor_y+y-5;
  if(px>=0&&px<256&&py>=0&&py<192)
   pixels[py*256+px]=cursor_bg[y*CURSOR_W+x];
 }
 cursor_visible=0;
}
void ui_virtual_cursor_update(int enabled,int cx,int cy,int pressed){
 int x,y;
 uint16_t color=pressed?LEMON:WHITE;
 if(!pixels)return;
 if(!enabled){cursor_erase();return;}
 if(cursor_visible&&cx==cursor_x&&cy==cursor_y&&pressed==cursor_pressed)
  return;
 cursor_erase();
 cursor_x=cx;cursor_y=cy;cursor_pressed=pressed;
 for(y=0;y<CURSOR_W;y++)for(x=0;x<CURSOR_W;x++){
  int px=cx+x-5,py=cy+y-5,k=y*CURSOR_W+x;
  if(px<0||px>=256||py<0||py>=192){cursor_bg[k]=0;continue;}
  cursor_bg[k]=pixels[py*256+px];
  if(x==5||y==5||((x==1||x==9)&&(y>=3&&y<=7)))
   pixels[py*256+px]=RGB(5,3,8);
  if((x==5&&y>=2&&y<=8)||(y==5&&x>=2&&x<=8))
   pixels[py*256+px]=color;
 }
 cursor_visible=1;
}
static int test_touch_down,test_touch_x,test_touch_y;
static unsigned long test_touch_count;
void ui_touch_observe(int down,int x,int y,uint32_t count){
 test_touch_down=down;
 /* A release is not a touch at 0,0. Keep the last real coordinate visible
  * so emulator testers can inspect where the previous press landed. */
 if(down&&x>=0&&x<256&&y>=0&&y<192){
  test_touch_x=x;
  test_touch_y=y;
 }
 test_touch_count=(unsigned long)count;
}
/* Glyph rows are a compact bitset, 5 columns x 7 rows, uppercase ASCII. */
static const uint8_t glyph[96][7]={
 ['A'-32]={14,17,17,31,17,17,17},
 ['B'-32]={30,17,17,30,17,17,30},
 ['C'-32]={14,17,16,16,16,17,14},
 ['D'-32]={30,17,17,17,17,17,30},
 ['E'-32]={31,16,16,30,16,16,31},
 ['F'-32]={31,16,16,30,16,16,16},
 ['G'-32]={14,17,16,23,17,17,14},
 ['H'-32]={17,17,17,31,17,17,17},
 ['I'-32]={31,4,4,4,4,4,31},
 ['J'-32]={7,2,2,2,18,18,12},
 ['K'-32]={17,18,20,24,20,18,17},
 ['L'-32]={16,16,16,16,16,16,31},
 ['M'-32]={17,27,21,21,17,17,17},
 ['N'-32]={17,25,21,19,17,17,17},
 ['O'-32]={14,17,17,17,17,17,14},
 ['P'-32]={30,17,17,30,16,16,16},
 ['Q'-32]={14,17,17,17,21,18,13},
 ['R'-32]={30,17,17,30,20,18,17},
 ['S'-32]={15,16,16,14,1,1,30},
 ['T'-32]={31,4,4,4,4,4,4},
 ['U'-32]={17,17,17,17,17,17,14},
 ['V'-32]={17,17,17,17,17,10,4},
 ['W'-32]={17,17,17,21,21,21,10},
 ['X'-32]={17,17,10,4,10,17,17},
 ['Y'-32]={17,17,10,4,4,4,4},
 ['Z'-32]={31,1,2,4,8,16,31},
 ['0'-32]={14,19,21,21,21,25,14},
 ['1'-32]={4,12,4,4,4,4,14},
 ['2'-32]={14,17,1,2,4,8,31},
 ['3'-32]={30,1,1,14,1,1,30},
 ['4'-32]={2,6,10,18,31,2,2},
 ['5'-32]={31,16,30,1,1,17,14},
 ['6'-32]={6,8,16,30,17,17,14},
 ['7'-32]={31,1,2,4,8,8,8},
 ['8'-32]={14,17,17,14,17,17,14},
 ['9'-32]={14,17,17,15,1,2,12},
 ['!'-32]={4,4,4,4,4,0,4},
 ['?'-32]={14,17,1,2,4,0,4},
 ['.'-32]={0,0,0,0,0,6,6},
 [','-32]={0,0,0,0,6,4,8},
 [':'-32]={0,4,4,0,4,4,0},
 [';'-32]={0,4,4,0,6,4,8},
 ['"'-32]={10,10,10,0,0,0,0},
 ['-'-32]={0,0,0,31,0,0,0},
 ['/'-32]={1,1,2,4,8,16,16},
 ['+'-32]={0,4,4,31,4,4,0},
 ['$'-32]={4,15,20,14,5,30,4},
 ['%'-32]={25,26,2,4,8,11,19},
 ['<'-32]={2,4,8,16,8,4,2},
 ['>'-32]={8,4,2,1,2,4,8},
 ['='-32]={0,0,31,0,31,0,0},
 ['*'-32]={0,21,14,31,14,21,0},
 ['#'-32]={10,10,31,10,31,10,10},
 ['('-32]={2,4,8,8,8,4,2},
 [')'-32]={8,4,2,2,2,4,8},
 ['\''-32]={4,4,8,0,0,0,0},
};
void ui_init(void *vram){pixels=(volatile uint16_t *)vram;}
static void solid(int x,int y,int rw,int rh,uint16_t color){
 int a,b;int x2=x+rw,y2=y+rh;
 if(x<0)x=0;
 if(y<0)y=0;
 if(x2>W)x2=W;
 if(y2>H)y2=H;
 if(!pixels||x>=x2||y>=y2)return;
 for(b=y;b<y2;b++){
  volatile uint16_t *row=pixels+(unsigned)b*W;
  /* Four halfword stores per loop branch. Keep VRAM's native access
   * width and use scalar tail writes for short/clipped spans. */
  for(a=x;a+3<x2;a+=4){
   row[a]=color;row[a+1]=color;row[a+2]=color;row[a+3]=color;
  }
  for(;a<x2;a++)row[a]=color;
 }
}
static void outline(int x,int y,int width,int height,uint16_t color){
 solid(x,y,width,1,color);solid(x,y+height-1,width,1,color);
 solid(x,y,1,height,color);solid(x+width-1,y,1,height,color);
}
/* Shared dimensional cards and buttons: no textures, allocations or
 * secondary buffer. All geometry is clipped by solid() to DS bitmap VRAM. */
/* Flat, legible notebook cards. One outline and a subtle shadow,
 * instead of multiple 1px glints that clutter a 192px touchscreen. */
static void panel(int x,int y,int width,int height,uint16_t fill,uint16_t border){
 if(width<3||height<3){
  solid(x+1,y+2,width,height,SHADOW);
  solid(x,y,width,height,fill);
  outline(x,y,width,height,border);
  return;
 }
 /* Draw only the visible offset shadow and the nonoverlapping interior
  * and border. Same final pixels as shadow->fill->outline. */
 solid(x+width,y+2,1,height-2,SHADOW);
 solid(x+1,y+height,width,2,SHADOW);
 solid(x+1,y+1,width-2,height-2,fill);
 solid(x,y,width,1,border);
 solid(x,y+height-1,width,1,border);
 solid(x,y+1,1,height-2,border);
 solid(x+width-1,y+1,1,height-2,border);
}
/* Draw horizontal font runs as single rectangles. The output matches the
 * old bit-per-pixel rasterizer while avoiding redundant VRAM write setup. */
static void char_at(int x,int y,char value,uint16_t color,int scale){
 int i,j;unsigned ch=(unsigned char)value;
 const uint8_t *bits;
 if(ch>='a'&&ch<='z')ch-=32;
 if(ch<32||ch>=128||scale<=0)return;
 bits=glyph[ch-32];
 for(j=0;j<7;j++){
  uint8_t row=bits[j];
  i=0;
  while(i<5){
   int start;
   if(!(row&(1u<<(4-i)))){i++;continue;}
   start=i++;
   while(i<5&&(row&(1u<<(4-i))))i++;
   solid(x+start*scale,y+j*scale,(i-start)*scale,scale,color);
  }
 }
}
static void label(int x,int y,const char *s,uint16_t color,int size,int max){
 int i;
 if(!s||size<=0)return;
 for(i=0;i<max&&x+i*6*size<W&&s[i];i++)
  char_at(x+i*6*size,y,s[i],color,size);
}
static void fit(int x,int y,const char *s,uint16_t color,int max){
 label(x,y,s,color,1,max);
}
static void capsule(int x,int y,int width,int height,uint16_t fill,uint16_t edge){
 if(width<7||height<7){
  solid(x+3,y,width-6,height,fill);
  solid(x,y+3,width,height-6,fill);
  solid(x+3,y,width-6,1,edge);solid(x+3,y+height-1,width-6,1,edge);
  solid(x,y+3,1,height-6,edge);solid(x+width-1,y+3,1,height-6,edge);
  return;
 }
 /* Partition the clipped-corner silhouette into disjoint spans:
  * no doubled fill over its central rectangle or overwritten edges. */
 solid(x+3,y,width-6,1,edge);
 solid(x+3,y+1,width-6,2,fill);
 solid(x+1,y+3,width-2,height-6,fill);
 solid(x,y+3,1,height-6,edge);
 solid(x+width-1,y+3,1,height-6,edge);
 solid(x+3,y+height-3,width-6,2,fill);
 solid(x+3,y+height-1,width-6,1,edge);
}
static void meter(int x,int y,int value,uint16_t fill){
 int v=value;
 if(v<0)v=0;
 if(v>100)v=100;
 solid(x,y,71,8,WHITE);
 outline(x,y,71,8,PURPLE);
 solid(x+2,y+2,(v*67)/100,4,fill);
 solid(x+2,y+2,(v*67)/100,1,WHITE);
}
/* A stat reads numerically as well as by color at native 256x192.
 * The small ivory counter is the same size in every health row. */
static void meter_value(int x,int y,int value,uint16_t fill){
 char tmp[8];
 int bounded=value<0?0:value>100?100:value;
 meter(x,y,bounded,fill);
 solid(x+48,y+1,21,6,IVORY);
 snprintf(tmp,sizeof(tmp),"%d",bounded);
 fit(x+50,y+1,tmp,INK,3);
}
static void insignia(int x,int y,uint16_t c){
 solid(x+3,y,3,3,c);
 solid(x,y+3,9,3,c);
 solid(x+3,y+6,3,3,c);
 solid(x+3,y+3,3,3,WHITE);
}
/* Readable 2x screen title plus two deliberately separated counters.
 * Avoid long headers colliding with the save/currency readout. */
static void header(const char *title,const Game *s,int can_save){
 char tmp[32];
 solid(0,0,W,27,NIGHT);
 solid(0,0,W,3,screen_accent);
 label(12,7,title,WHITE,2,10);
 fit(138,11,can_save?"SD":"RAM",can_save?MINT:PINK,3);
 capsule(167,5,37,17,IVORY,PURPLE);
 snprintf(tmp,sizeof(tmp),"D%lu",(unsigned long)s->days);
 fit(173,10,tmp,INK,4);
 capsule(209,5,40,17,LEMON,GOLD);
 snprintf(tmp,sizeof(tmp),"$%u",s->dollars);
 fit(214,10,tmp,INK,5);
 solid(0,25,W,2,screen_accent);
}
static void foot(uint16_t color){
 solid(0,176,256,16,NIGHT);
 solid(0,176,256,2,screen_accent);
 (void)color;
}
void ui_arcade_feedback(ArcadeFeedbackEvent event){
 const char *text;
 uint16_t color;
 switch(event){
 case ARCADE_FEEDBACK_PROGRESS:text="NICE! KEEP GOING.";color=MINT;break;
 case ARCADE_FEEDBACK_MISS:text="OOPS! TRY AGAIN.";color=PINK;break;
 case ARCADE_FEEDBACK_WIN:text="YOU WON! GREAT JOB.";color=LEMON;break;
 case ARCADE_FEEDBACK_LOSS:text="ROUND LOST. TRY AGAIN.";color=WHITE;break;
 default:return;
 }
 /* This pass follows the full UI draw and precedes the virtual cursor.
  * No flashes, new surface, or writes into the cabinet's playfield. */
 solid(0,178,256,14,NIGHT);
 fit(10,181,text,color,39);
}
void ui_save_warning(void){
 /* A game result must not hide that its progress is only in RAM. This
  * remains a caller-controlled warning, separate from the feedback TTL. */
 solid(0,178,256,14,NIGHT);
 fit(10,181,"SAVE FAILED. PROGRESS IN RAM!",PINK,39);
}
void ui_learning_save_warning(void){
 /* Profile persistence is separate from saving the game's progress.
  * The caller gives an actual game-save failure higher precedence. */
 solid(0,178,256,14,NIGHT);
 fit(10,181,"LEARNING NOT SAVED. KEPT IN RAM!",PINK,39);
}
static void icon_bottle(int x,int y){
 /* Stepped shoulders, round lower corners and a distinct milk line.
  * The same 32x24 card slot and fixed 5-bit palette remain. */
 solid(x+10,y,5,3,WHITE);solid(x+8,y+3,9,4,BLUE);
 solid(x+7,y+7,11,2,TEAL);solid(x+5,y+9,15,13,TEAL);
 solid(x+7,y+22,11,2,TEAL);solid(x+6,y+10,13,11,WHITE);
 solid(x+7,y+15,11,6,SKY);solid(x+7,y+14,11,1,IVORY);
 solid(x+8,y+10,2,4,IVORY);solid(x+15,y+12,3,1,TEAL);
 solid(x+15,y+16,3,1,TEAL);
}
static void icon_chat(int x,int y){
 capsule(x+3,y+3,23,18,PURPLE,WHITE);
 solid(x+7,y+20,6,5,PURPLE);
 solid(x+8,y+10,3,3,LEMON);solid(x+14,y+10,3,3,LEMON);
 solid(x+20,y+10,3,3,LEMON);
}
static void icon_play(int x,int y){
 capsule(x+3,y+4,24,17,TEAL,WHITE);
 solid(x+8,y+7,4,11,WHITE);solid(x+5,y+11,10,4,WHITE);
 solid(x+19,y+8,3,3,LEMON);solid(x+23,y+13,3,3,PINK);
}
static void icon_nap(int x,int y){
 /* Little padded bed in perspective: rounded pillow, blanket folds
  * and a real dark foot edge, without a bitmap or alpha compositor. */
 solid(x+4,y+9,22,11,PURPLE);
 solid(x+5,y+10,20,8,LILAC);
 solid(x+7,y+6,13,4,WHITE);solid(x+8,y+7,11,1,IVORY);
 solid(x+8,y+12,15,4,BLUSH);solid(x+8,y+16,15,2,PINK);
 solid(x+6,y+18,22,3,DEEP_PINK);
 solid(x+6,y+21,3,2,PURPLE);solid(x+25,y+21,3,2,PURPLE);
 label(x+11,y+1,"Z",PURPLE,1,1);
}
/* Stream bounded spans directly from a reply: no copied text buffer.
 * Prefer whole words, hard-wrap long words without losing a character,
 * and honor explicit LF/CRLF rows on the fixed 36-column grid. */
static int speech_space(char c){
 return c==' '||c=='\t';
}
static int speech_rows(const char *message,int y,int spacing,int rows,
                        unsigned capacity,uint16_t ink){
 const char *next=message?message:"";
 unsigned used=0;
 int row;
 for(row=0;row<rows;row++){
  int length=0,boundary=-1;
  while(used<capacity&&speech_space(*next)){next++;used++;}
  if(used==capacity||!*next)break;
  while(length<36&&(unsigned)length<capacity-used&&next[length]&&
        next[length]!='\n'&&next[length]!='\r'){
   if(speech_space(next[length]))boundary=length;
   length++;
  }
  /* A word crossing the right edge moves intact to the next row. If
   * the word itself exceeds one row, advance exactly the drawn span. */
  if(length==36&&(unsigned)length<capacity-used&&next[length]&&
     !speech_space(next[length])&&
     next[length]!='\n'&&next[length]!='\r'&&boundary>0)
   length=boundary;
  label(17,y+row*spacing,next,ink,1,length);
  next+=length;used+=(unsigned)length;
  while(used<capacity&&speech_space(*next)){next++;used++;}
  if(used<capacity&&*next=='\r'){next++;used++;}
  if(used<capacity&&*next=='\n'){next++;used++;}
 }
 /* Never inspect a byte beyond the caller's declared text budget. */
 return used==capacity||*next!='\0';
}
static void speech_lines(const char *message,int y,int spacing,int rows){
 speech_rows(message,y,spacing,rows,UINT32_MAX,INK);
}
/* Nursery-only stationery: broad, quiet material bands rather than a
 * repeating texture behind small text. No new storage or redraw cadence. */
static void nursery_paper(void){
 solid(0,27,W,149,PAPER);
 solid(0,27,3,149,PAPER_EDGE);solid(253,27,3,149,PAPER_EDGE);
 solid(3,27,1,147,IVORY);solid(252,27,1,147,IVORY);
 solid(4,174,248,2,PAPER_SHADE);
}
static void nursery_speech_card(int height){
 panel(8,31,240,height,WHITE,PURPLE);
 /* ASK/care headings occupy this tinted strip. Every reply row remains
  * plain white, including the full 36th column and the optional third row. */
 solid(9,32,238,12,IVORY);
 solid(9,44,238,1,BLUSH);
}
/* Recess the picture and lift the label face using a few broad bands.
 * The care recommendation gets a stable edge, not flashing decoration.
 * Original touch rectangles, text grids and benefit captions stay exact. */
static void care_action_card(int x,int y,uint16_t fill,uint16_t edge,
                              int recommended){
 solid(x+1,y+2,116,30,PAPER_EDGE);
 capsule(x,y,116,30,fill,edge);
 solid(x+4,y+3,32,24,IVORY);
 solid(x+37,y+4,1,22,edge);
 solid(x+41,y+19,70,8,IVORY);
 solid(x+4,y+1,108,1,WHITE);
 solid(x+4,y+28,108,1,edge);
 if(recommended){
  solid(x+3,y+1,110,2,edge);
  solid(x+1,y+4,2,22,edge);
  solid(x+113,y+4,2,22,edge);
  solid(x+3,y+27,110,2,edge);
 }
}
/* The nursery follows an actual DS pet-care layout:
 * dialogue, a compact health summary, then FOUR large actions. */
/* Art-directed nursery-care touchscreen. The original interactive
 * FEED/CHAT/PLAY/NAP rectangles and the three-part footer dock are exact.
 * 5-bit color gradients are suggested with two broad pixel bands, not a
 * slow per-pixel compositor. Bitmap repaints occur only on state changes. */
static void home(const Game *s,int can_save,const char *message){
 int hint=game_care_hint(s);
 nursery_paper();
 /* Playful but slightly uncanny voice-message panel, inspired by the care
  * art target. Text stays at the existing safe one-pixel font grid. */
 nursery_speech_card(34);
 solid(13,34,24,14,LILAC);
 solid(18,36,14,10,PINK);
 solid(20,38,3,3,PURPLE);
 solid(27,38,3,3,PURPLE);
 capsule(39,33,42,13,IVORY,PURPLE);
 fit(45,36,"TALK",INK,4);
 capsule(84,33,39,13,MINT,TEAL);
 fit(94,36,"MIC",INK,3);
 /* Actionable priorities fit the original header's spare pixels:
  * no overlaid speech, clipped label, new card or changed hitbox. */
 {
  static const char *const names[4]={"TRY FEED","TRY CHAT",
                                     "TRY PLAY","TRY NAP"};
  fit(126,35,hint<0?"ALL GOOD":names[hint],
      hint<0?TEAL:ROSE,10);
 }
 solid(126,43,61,1,PINK);
 /* Real nursery inspection controls; deliberately outside all existing
  * four care action hitboxes and the bottom Toybox navigation strip. */
 capsule(194,34,49,13,SKY,BLUE);
 fit(203,38,"ROOM",INK,5);
 speech_lines(message,47,9,2);
 /* Numerical overlays keep all FIVE stats legible when a bar is nearly
  * empty. Existing positions are unchanged for returning players. */
 panel(8,68,240,36,WHITE,PURPLE);
 fit(17,72,"HUNGER",PURPLE,7);meter_value(61,70,s->hunger,ROSE);
 fit(144,72,"MOOD",PURPLE,5);meter_value(178,70,s->mood,GREEN);
 fit(17,85,"ENERGY",PURPLE,7);meter_value(61,83,s->energy,TEAL);
 fit(144,85,"CHAOS",PURPLE,5);meter_value(178,83,s->chaos,GOLD);
 fit(17,96,"GLITCH",MUTED,7);
 solid(62,97,175,5,GRAY);
 solid(62,97,s->corruption*175/100,4,ROSE);
 /* Large, contrasted cards; gradients and icons drawn entirely with
  * the existing single-screen bitmap primitives, no texture assets. */
 care_action_card(8,108,SKY,BLUE,hint==FEED);
 if(hint==FEED)fit(108,112,"!",ROSE,1);
 icon_bottle(12,110);label(50,111,"FEED",INK,2,4);
 fit(50,128,"HUNGER -30",BLUE,12);
 care_action_card(132,108,LEMON,GOLD,hint==CHAT);
 if(hint==CHAT)fit(232,112,"!",ROSE,1);
 icon_chat(135,110);label(173,111,"CHAT",INK,2,4);
 fit(172,128,"MOOD +7",PURPLE,10);
 care_action_card(8,143,MINT,GREEN,hint==PLAY);
 if(hint==PLAY)fit(108,148,"!",ROSE,1);
 icon_play(12,144);label(50,146,"PLAY",INK,2,4);
 fit(50,163,"MOOD +10",GREEN,10);
 care_action_card(132,143,PINK,ROSE,hint==NAP);
 if(hint==NAP)fit(232,148,"!",ROSE,1);
 icon_nap(135,145);label(172,146,"NAP",INK,2,3);
 fit(172,163,"ENERGY +25",PURPLE,11);
 /* The center dock remains the hidden Toybox entry; introducing decorative
  * details must never move ARCADE / TOYBOX / END DAY hitboxes. */
 foot(PURPLE);
 capsule(7,178,80,13,NIGHT,TEAL);
 fit(20,183,"ARCADE",WHITE,10);
 capsule(93,178,70,13,LEMON,GOLD);
 solid(99,182,8,7,PINK);
 solid(100,180,6,3,WHITE);
 fit(112,183,"TOYBOX",INK,9);
 capsule(169,178,80,13,NIGHT,TEAL);
 fit(189,183,"END DAY",WHITE,10);
 (void)can_save;
}
/* Six conversational intentions share the nursery's existing font/cards.
 * Replies fit up to three rows without repainting on an idle frame. */
static void conversation_screen(int selection,const char *message){
 static const char *const prompts[6]={
  "HOW ARE YOU?","NEED ANYTHING?","REMEMBER?",
  "TELL ME MORE","I LIKE THAT","NEW SUBJECT"
 };
 int i,selected=selection>=0&&selection<6?selection:0;
 nursery_paper();
 nursery_speech_card(55);
 fit(17,36,"ASK:",PURPLE,4);
 fit(47,36,prompts[selected],PURPLE,30);
 speech_lines(message,48,11,3);
 for(i=0;i<6;i++){
  int x=i%2?132:8,y=92+(i/2)*27;
  panel(x,y,116,24,i==selected?MINT:WHITE,i==selected?GREEN:PURPLE);
  /* Shaded lower/right edges give each choice a pressable face while
   * remaining outside all 17 label columns and their seven glyph rows. */
  solid(x+1,y+20,114,3,i==selected?RGB(19,27,20):PAPER_SHADE);
  solid(x+113,y+2,2,18,i==selected?RGB(19,27,20):PAPER_SHADE);
  if(i==selected){
   solid(x+3,y+4,3,16,GREEN);
   solid(x+3,y+2,110,1,GREEN);
  }
  fit(x+10,y+8,prompts[i],INK,17);
 }
 foot(PURPLE);
 solid(79,179,1,11,TEAL);solid(176,179,1,11,TEAL);
 fit(12,181,"B BACK",WHITE,6);
 fit(89,181,"D-PAD CHOOSE",MINT,12);
 fit(202,181,"A ASK",WHITE,5);
}
/* A small offline shelter console. The interface owns no transcript or
 * input storage: callers supply the visible strings and selected key. */
static const char observer_keys[]="QWERTYUIOPASDFGHJKL?ZXCVBNM .<";
static const char observer_numbers[]="1234567890+-*/()=?%.ZXCVBNM .<";
char ui_observer_key(unsigned key){
 if(key<30u)return observer_keys[key];
 if(key>=32u&&key<62u)return observer_numbers[key-32u];
 return '\0';
}
int ui_observer_entry_touch(int x,int y){
 return x>=80&&x<124&&y>=5&&y<21;
}
int ui_observer_touch(int x,int y){
 int row,col;
 if(x<0||x>=256||y<0||y>=192)return -1;
 if(y>=176)return 30+x/64;
 if(y>=165&&y<176&&x>=8&&x<248){
  int slot=(x-8)/60;
  if((x-8)%60<56)return 36+slot;
 }
 if(x>=130&&x<181&&y>=88&&y<100)return 35; /* MIC */
 if(x>=216&&x<248&&y>=100&&y<115)return 34;
 if(x<8||x>=246||y<118||y>=164)return -1;
 col=(x-8)/24;row=(y-118)/16;
 if(col>=10||row>=3||(x-8)%24>=22||(y-118)%16>=14)return -1;
 return row*10+col;
}
const char *ui_observer_quick_prompt(unsigned speaker,unsigned index){
 static const char *const monitor[4]={"STATUS","NEEDS","BOND","HELP"};
 static const char *const infant[4]={"HELLO","NEEDS","DREAM","MORE"};
 return index<4u?(speaker?infant[index]:monitor[index]):"";
}
void ui_observer_alert(const char *text){
 char line[40];
 unsigned n=0;
 const char *source=text?text:"WARDEN IS WATCHING.";
 while(n<sizeof(line)-1u&&source[n]&&source[n]!='\n'&&source[n]!='\r'){
  line[n]=source[n];n++;
 }
 line[n]='\0';
 solid(0,178,256,14,NIGHT);
 fit(10,181,line,AQUA,39);
}
void ui_observer_guard_prompt(unsigned integrity){
 if(integrity>100u)integrity=100u;
 /* Inset two pixels inside the shared CPU hitbox: the TOYBOX title's
  * final X reaches x=81 and must remain readable during its windup. */
 capsule(82,5,42,16,integrity>25u?LEMON:PINK,ROSE);
 fit(88,9,"GUARD",INK,5);
 solid(85,18,36,1,PURPLE);
 solid(85,18,(int)(integrity*36u/100u),1,integrity>25u?GREEN:ROSE);
 ui_observer_alert("WINDUP! TAP GUARD / RIGHT");
}
void ui_observer_save_warning(void){
 solid(0,178,256,14,NIGHT);
 fit(10,181,"SHELTER MEMORY NOT SAVED. IN RAM!",PINK,39);
}
void ui_observer_draw(const Game *s,int can_save,unsigned speaker,
                       const char *input,const char *reply,unsigned key,
                       unsigned stage,unsigned integrity){
 unsigned i,length=0,selected=ui_observer_key(key)?key&31u:0u;
 int numeric=ui_observer_key(key)&&(key&32u);
 char status[24];
 static const char *const stages[]={"FOUND","WATCHING","HOSTILE","REVEALED"};
 const char *keys=numeric?observer_numbers:observer_keys;
 const uint16_t phosphor=RGB(24,31,26);
 if(!pixels||!s)return;
 cursor_visible=0;
 screen_accent=speaker?PINK:TEAL;
 header(speaker?"BABY":"MONITOR",s,can_save);
 solid(0,27,256,149,RGB(6,11,13));
 panel(8,31,240,60,RGB(3,6,8),TEAL);
 solid(9,32,238,10,RGB(8,14,15));
 fit(17,34,speaker?"NURSERY VOICE":"WARDEN",phosphor,16);
 if(integrity>100u)integrity=100u;
 snprintf(status,sizeof(status),"BRACER %u%%",integrity);
 fit(167,34,status,phosphor,13);
 if(speech_rows(reply,45,11,4,144u,phosphor))
  fit(137,34,"...",LEMON,3);
 /* Stable mode/readiness information belongs above the input field;
  * there is no blinking cursor, scrolling texture or per-frame repaint. */
 fit(17,92,"YOUR MESSAGE",AQUA,12);
 capsule(130,88,51,12,MINT,TEAL);
 fit(144,91,"MIC",INK,3);
 fit(190,92,stages[stage>3u?3u:stage],phosphor,9);
 panel(8,101,240,14,IVORY,TEAL);
 fit(17,104,input?input:"",INK,32);
 if(input)while(length<32u&&input[length])length++;
 solid(17+(int)length*6,112,5,1,TEAL);
 capsule(216,100,32,15,numeric?MINT:LEMON,TEAL);
 fit(223,104,numeric?"ABC":"123",INK,3);
 for(i=0;i<30u;i++){
  int x=8+(int)(i%10u)*24,y=118+(int)(i/10u)*16;
  panel(x,y,22,14,i==selected?MINT:RGB(19,23,23),
                    i==selected?GREEN:TEAL);
  if(i==selected)solid(x+2,y+11,18,2,GREEN);
  if(keys[i]==' ')fit(x+5,y+3,"SP",INK,2);
  else char_at(x+8,y+3,keys[i],INK,1);
 }
 /* Suggested messages are drafts, never immediate care/terminal
  * commands. The real SEND and MIC controls remain separate. */
 for(i=0;i<4u;i++){
  const char *prompt=ui_observer_quick_prompt(speaker,i);
  int x=8+(int)i*60;
  capsule(x,165,56,11,speaker?BLUSH:MINT,TEAL);
  fit(x+(56-(int)strlen(prompt)*6)/2,167,prompt,INK,8);
 }
 foot(TEAL);
 solid(64,179,1,11,TEAL);solid(128,179,1,11,TEAL);
 solid(192,179,1,11,TEAL);
 fit(10,181,"B BACK",WHITE,6);
 fit(70,181,"Y TARGET",WHITE,8);
 fit(145,181,"TEACH",LEMON,5);
 fit(202,181,"X SEND",WHITE,6);
}
/* A user-initiated microphone lab. No listening in CARE or idle WARDEN.
 * The 256x192 bitmap is fully drawn only on interactions/meter updates. */
int ui_mic_touch(int x,int y){
 if(x<0||x>=256||y<0||y>=192)return 0;
 if(y>=176)return 4;
 if(y>=113&&y<128){
  if(x>=8&&x<44)return 5;
  if(x>=48&&x<84)return 6;
  if(x>=205&&x<248)return 7;
 }
 if(y>=131&&y<161){
  if(x>=8&&x<84)return 1;
  if(x>=90&&x<166)return 2;
  if(x>=172&&x<248)return 3;
 }
 return 0;
}
void ui_mic_draw(const Game *s,int can_save,const char *label,
                 const char *status,const char *match,unsigned level,
                 unsigned learned,int listening,const char *selected_label,
                 unsigned examples,int forget_armed,unsigned speaker,
                 unsigned mode,unsigned phase){
 unsigned width;
 if(!pixels||!s)return;
 cursor_visible=0;screen_accent=AQUA;
 header("MIC LAB",s,can_save);
 solid(0,27,256,149,RGB(13,20,23));
 panel(8,31,240,48,WHITE,TEAL);
 fit(17,36,mode?(mode==1u?"TRAINING":"LISTENING"):"MIC OFF / READY",
     mode?TEAL:PURPLE,23);
 capsule(181,33,60,13,speaker?BLUSH:MINT,TEAL);
 fit(187,36,speaker?"BABY":"WARDEN",INK,8);
 fit(17,49,"LABEL:",PURPLE,6);
 fit(62,49,label&&*label?label:"HELLO",INK,24);
 fit(17,64,"INPUT",PURPLE,6);
 solid(64,64,172,9,GRAY);
 /* Ignore the last capture's stale level when the microphone is OFF.
  * Calibration, waiting and actual recording have distinct colors. */
 width=!listening?0u:level>100u?172u:level*172u/100u;
 solid(64,64,(int)width,9,phase==2u?ROSE:phase==0u?GOLD:GREEN);
 for(int i=1;i<8;i++)solid(64+i*21,64,1,9,IVORY);
 panel(8,83,240,40,IVORY,TEAL);
 speech_rows(status?status:"MIC OFF. A TRAIN / X LISTEN.",88,8,2,72u,INK);
 fit(17,104,"MATCH:",PURPLE,6);
 fit(61,104,match&&*match?match:"NONE / AWAITING SAMPLE",INK,27);
 /* Browse stored words without retraining them or touching any care
  * action. An explicitly confirmed DEL is the only way to free a slot. */
 solid(0,113,256,15,RGB(13,20,23));
 capsule(8,113,36,15,MINT,TEAL);
 fit(13,117,"PREV",INK,4);
 capsule(48,113,36,15,MINT,TEAL);
 fit(53,117,"NEXT",INK,4);
 capsule(88,113,114,15,IVORY,TEAL);
 fit(94,117,selected_label&&*selected_label?selected_label:"NO LESSONS",INK,17);
 capsule(205,113,43,15,forget_armed?PINK:LEMON,ROSE);
 fit(forget_armed?209:217,117,forget_armed?"SURE?":"DEL",INK,
     forget_armed?5:3);
 capsule(8,131,76,30,mode?PINK:MINT,mode?ROSE:GREEN);
 fit(mode?28:25,139,mode?"A STOP":"A TRAIN",INK,7);
 capsule(90,131,76,30,mode?PINK:SKY,mode?ROSE:BLUE);
 fit(mode?110:106,139,mode?"X STOP":"X LISTEN",INK,8);
 capsule(172,131,76,30,(!mode&&match&&*match)?LEMON:GRAY,
         (!mode&&match&&*match)?GOLD:TEAL);
 fit(192,139,"Y USE",INK,5);
 {
  char count[32],trained[20];
  snprintf(count,sizeof(count),"SAVED LABELS %u / 24",learned>24u?24u:learned);
  fit(17,166,count,WHITE,25);
  snprintf(trained,sizeof(trained),"EX %u/2",examples>2u?2u:examples);
  fit(183,166,trained,WHITE,7);
 }
 foot(TEAL);
 fit(13,182,mode?"A/X STOP   B BACK / CANCEL":
     "B BACK   L/R BROWSE   DEL TWICE",WHITE,39);
}
/* Save failure takes precedence over mic status, preserving live RAM model. */
void ui_voice_save_warning(void){
 solid(0,178,256,14,NIGHT);
 fit(10,181,"VOICE LESSONS NOT SAVED. IN RAM!",PINK,39);
}

/* First-class interactive Room mode. Cards match the actual nursery
 * landmarks seen on the TOP hardware 3D screen. All four actions are
 * available through native touch, virtual stylus and D-pad fallback. */
static void room_screen(const Game *s,int state,const char *message){
 int selected=state&3;
 int day=(state&8)!=0,lamp=(state&16)!=0;
 int muted=(state&32)!=0; /* shared Toybox/Room sound setting */
 static const char *const name[4]={"WINDOW","LAMP","TEDDY","CRIB"};
 static const char *const benefit[4]={
  "SKY / MOON","SWITCH LIGHT","MOOD +10","ENERGY +25"
 };
 int i,x,y;
 (void)s;
 solid(0,27,256,149,LILAC);
 panel(8,31,240,41,WHITE,PURPLE);
 fit(18,36,"A LITTLE ROOM WITH BIG SECRETS",PURPLE,36);
 solid(18,46,220,1,PINK);
 fit(18,50,name[selected],INK,10);
 fit(85,50,selected==0?(day?"DAYLIGHT":"MOONLIGHT"):
              selected==1?(lamp?"LAMP: ON":"LAMP: OFF"):
              selected==2?"TEDDY SAYS HELLO":"NURSERY NAP",TEAL,22);
 /* Explicit in-game SAVE; remains available after a RAM-only START.
  * Card is above prop hitboxes and inside the existing 256x192 panel. */
 capsule(198,49,46,14,MINT,TEAL);
 fit(209,53,"SAVE",INK,5);
 fit(18,63,message?message:"TOUCH A FURNITURE CARD.",MUTED,36);
 for(i=0;i<4;i++){
  x=i%2?132:8;
  y=i/2?122:77;
  capsule(x,y,116,38,i==selected?MINT:WHITE,
          i==selected?GREEN:PURPLE);
  /* Recognizable tiny icons: a window with daylight bars, a conical lamp,
   * a plush face and a crib's frame. No tiles/assets or drawn ghost hitbox. */
  if(i==0){
   outline(x+8,y+6,28,24,BLUE);
   solid(x+20,y+7,2,22,BLUE);
   solid(x+9,y+17,25,2,BLUE);
   solid(x+(day?23:11),y+10,7,7,day?LEMON:SKY);
  }else if(i==1){
   solid(x+18,y+9,10,3,lamp?LEMON:GRAY);
   solid(x+13,y+12,20,7,lamp?GOLD:PURPLE);
   solid(x+21,y+19,4,12,INK);
   solid(x+16,y+30,14,2,INK);
  }else if(i==2){
   solid(x+9,y+9,10,10,GOLD);solid(x+28,y+9,10,10,GOLD);
   capsule(x+12,y+12,23,21,GOLD,ROSE);
   solid(x+17,y+19,3,3,INK);solid(x+28,y+19,3,3,INK);
   solid(x+22,y+25,4,4,PURPLE);
  }else{
   outline(x+9,y+10,28,20,PURPLE);
   solid(x+15,y+11,2,18,PURPLE);
   solid(x+22,y+11,2,18,PURPLE);
   solid(x+29,y+11,2,18,PURPLE);
   solid(x+8,y+31,4,3,INK);
   solid(x+32,y+31,4,3,INK);
  }
  fit(x+45,y+8,name[i],PURPLE,10);
  /* Teach the real interaction: moonlight + lamp off makes Crib
   * meaningfully more restful. The cards always match gameplay state. */
  fit(x+45,y+22,i==3&&!day&&!lamp?"BONUS +8":benefit[i],
      INK,13);
 }
 foot(PURPLE);
 capsule(8,178,240,13,NIGHT,TEAL);
 fit(18,182,"B BACK",WHITE,7);
 fit(75,182,"D-PAD",MINT,6);
 fit(151,182,muted?"Y SND OFF":"Y SND ON",
     muted?ROSE:MINT,9);
 fit(211,182,"A USE",WHITE,6);
}
/* Five-character art-directed wordmarks can occupy a genuine 2x font,
 * with the full cabinet title displayed in the section above. */
static const char *const nick[16]={
 "MILK","WIPE","TAX","KART","BEAT","SOCK","NAP","SHAKE",
 "CRAWL","DRIFT","CODE","STACK","AUDIT","GROW","COURT","CEO"
};
static void arrow(int x,int y,int dir,uint16_t fill){
 int i;
 /* Purposefully chunky hardware-era arrows, not Unicode glyphs. */
 if(dir==0){for(i=0;i<8;i++)solid(x-i,y+i,2*i+1,2,fill);solid(x-3,y+14,7,10,fill);}
 if(dir==2){solid(x-3,y,7,10,fill);for(i=0;i<8;i++)solid(x-7+i,y+9+i,15-2*i,2,fill);}
 if(dir==1){for(i=0;i<8;i++)solid(x+i,y-i,2,2*i+1,fill);solid(x-12,y-3,17,7,fill);}
 if(dir==3){for(i=0;i<8;i++)solid(x-i,y-i,2,2*i+1,fill);solid(x-5,y-3,17,7,fill);}
}

/* Original hand-authored pixel sprites for the arcade. These are tiny indexed
 * raster sheets, not the former boxes-and-arrows placeholders. The artwork is
 * embedded in ROM as text rows so the existing bitmap renderer needs no PNG
 * decoder, heap, asset loader, or second framebuffer. */
/* Shared baby portrait: warm face, small eyes, soft cheeks, chestnut tuft,
 * mint romper. A matching silhouette appears in the DS 3D nursery. */
/* Warm shaded 20x18 ROM pixel baby shared by the title and all cabinets.
 * Added hair sheen, cheek contour, bright sclera and a two-tone romper. */
static const char *const spr_baby[]={
  ".........dd.........",
  ".......hdddhh.......",
  ".....hodddooooh.....",
  ".....dddsssshho.....",
  "....osssssssssso....",
  "...ollsssssssssso...",
  "...ollhhsssshhsso...",
  "...olloossssoossoo..",
  "..oollwksssswkssoso.",
  ".osollasssssasaaoso.",
  "..oorrsssvssssrrooo.",
  "....osssvssvssao....",
  ".....osssvvssso.....",
  "......occcccco......",
  ".....omfcfmcmmo.....",
  ".....omfbbbbmmo.....",
  ".....omffmmmmmo.....",
  ".....oommmmmmoo....."
};
/* Original 32x24 ROM pixel illustration of a *split*, artificial egg.
 * The center remains visibly black: shell halves frame the living cavity.
 * 768 bytes of indexed text, no image loader or graphics allocation. */
static const char *const spr_egg[24]={
 "................................",
 ".......qqqqq.........wwwww......",
 ".....qqqiwwqqq.....wwwwwwwww....",
 "....qqiiiwwwwqq...wwwwwwwwwww...",
 "...qqiiiiwwwwwqmmmwwwwwwwwwwww..",
 "...qiiiiiwwwwwmnnnmwwwwwwwwwww..",
 "..qiiiiiiwwwwmnnnnnmwwwwwwwwwww.",
 "..qiiiiiiwwtwmnnnnnmwtwwwwwwwww.",
 "..iiiiiiiwwtwnnnnnnnwtwwwwwwwww.",
 ".qiiiiiiiwwtwnynnnynitiilllllllq",
 ".qiiiiiiiwwfmngnnngnmfiilllllllq",
 ".qiiiiiiiwwtmnnnnnnnmtiilllllllq",
 ".qiiiiiiiwwtynnnnnnnytiilllllllq",
 ".qiiiiiiiwwtmnnnnnnnmtiilllllllq",
 ".qiiiiiiiwwtmnnnnnnnmtiilllllllq",
 "..iiiiiiiiifynnnnnnnyfiilllllll.",
 "..iiiiiiiiitmnnkkknnmtiillllllq.",
 "..iiiiiiiiitmmnkkknmmtiillllllq.",
 "...iiiiiiiitymnnnnnmytiilllllq..",
 "...iiiiiiiiimimnnnmimiiillllqq..",
 "....iiiiiiiiiiimmmqqiiiilllqq...",
 ".....iiiiiiiii.....qqqiilqqq....",
 ".......iiiii.........qqqqq......",
 "................................"
};
static const char *const spr_bottle[]={
  "......oooo......",
  "......owwo......",
  ".....owwwwo.....",
  ".....oppppo.....",
  "....oooooooo....",
  "...ojjjwwwwwo...",
  "...ojjjwwwwwo...",
  "...ojjbbbbwwo...",
  "...ojjbbbbwwo...",
  "...ojjbbbbwwo...",
  "...ojjbbbbwwo...",
  "...ojjbbbbwwo...",
  "...ojjbbbbwwo...",
  "...ojjjwwwwwo...",
  "...ojjjppwwwo...",
  "...ojjjppwwwo...",
  "....owwwwwwo....",
  ".....oooooo....."
};
static const char *const spr_rattle[]={
  "......oooo......",
  "....ooggggoo....",
  "...oyyywwgggo...",
  "..oyyyywwggggo..",
  "..oyyyywwggggo..",
  "..oyyyyyyggggo..",
  "...oyyyyygggo...",
  "....ooyyygoo....",
  "......oooo......",
  "......otto......",
  "......otto......",
  "......otto......",
  "......otto......",
  "......otto......",
  ".....otttto.....",
  ".....otttto.....",
  "......oooo......"
};
static const char *const spr_diaper[]={
  "...oooooooooooo...",
  "..owwwwwwwwwwwwo..",
  ".ojjjjwwwwwwiiiio.",
  "ojjjjjwwwwwwiiiio",
  "ojjppjwwwwwwippiio",
  "ojjppjwwwwwwippiio",
  ".ojjjbbbbbbwiiio.",
  "..ojbbbbbbbbiio..",
  "...ojbbbbbbwio...",
  "....owwwwwwo....",
  ".....oooooo....."
};
/* Receipt sheet with inked totals, folded corners and a bright stamp. */
static const char *const spr_receipt[]={
 "..oooooooooo..",
 ".owwwwwwwwwwo.",
 "owwwwwwwwwwwwo",
 "owgwwwwwwwwgwo",
 "owwwppppwwwwo",
 "owwwwwwwwwwwo",
 "owwwwwwwwwwwo",
 "owttttttttwwo",
 "owwwwwwwwwwwo",
 "owppppwwppwwo",
 "owwwwwwwwwwwo",
 ".owwwwwwwwwwo.",
 "..oooooooooo.."
};
/* Tiny cartridge art; rendered directly to sub-screen bitmap VRAM. */
static const char *const spr_kart[]={
 "...ooooooo...",
 "..owwwwwwwwo.",
 ".owwppppwwwo",
 "owwppwwppwwwo",
 "owwwwwwwwwwwo",
 ".ooobbbbooo..",
 "..ottbbtto...",
 "..ottttttto..",
 "..ooooooo...",
 ".ooo....ooo.",
 ".ooo....ooo."
};
static const char *const spr_cone[]={
 "......oo......",
 ".....oggo.....",
 ".....oggo.....",
 "....oggggo....",
 "....owwwwo....",
 "...oggggggo...",
 "...oggggggo...",
 "..owwwwwwwwo..",
 "..oggggggggo..",
 ".oooooooooooo."
};
static const char *const spr_sock[]={
  "....oooo......",
  "...owwwwo.....",
  "...ojjwwo.....",
  "...ojjwwo.....",
  "...ojjwwo.....",
  "...ojjwwo.....",
  "...ojjwwo.....",
  "...ojjwwwbo....",
  "...ojjwwwbbo..",
  "..ojjjwwwbbbo.",
  "..ojjjwwwbbbo.",
  "..ojjjwwwbbbo.",
  "...ooooooooo.."
};
/* Additional compact original silhouettes for the newly bespoke cabinets. */
static const char *const spr_stroller[]={
 "....oossssoo......",
 "...osssssssso.....",
 "..osssssssssso....",
 ".osssshhhhsssso...",
 ".ossshssssshsso...",
 ".ossshwwswwhsso...",
 "..ossswwwwsssso...",
 "...oooooooooo.....",
 "....ottbbtto......",
 "...otttbbttto.....",
 "..oooooooooooo....",
 ".ooo........ooo...",
 ".ooo........ooo..."
};
static const char *const spr_cereal[]={
  "...ooooooooo...",
  "..owwwwwwwwwo..",
  ".owwwggggwwwwwo.",
  "owyyywwwwggggwo",
  "owywwwwwggwwgwo",
  "owywwyywwwgggwo",
  "owwwyyygwwwggwo",
  "owwwwwwwwwwwwo",
  "owwwpppppwwwwwo",
  ".owwwwwwwwwwwo.",
  "..ooooooooooo.."
};
static const char *const spr_stack[]={
 ".oooooooooooo.",
 "owwwwwwwwwwwwo",
 "owppwwwwwwppwo",
 "owppwwwwwwppwo",
 "owwwwwwwwwwwwo",
 "owwwttttttwwwo",
 "owwwttttttwwwo",
 "owwwwwwwwwwwwo",
 ".oooooooooooo."
};
static const char *const spr_flower[]={
 "......pp......",
 ".....pppp.....",
 "..pp..gg..pp..",
 ".ppp.gggg.ppp.",
 "..pp..gg..pp..",
 ".....pppp.....",
 "......pp......",
 "......tt......",
 "....ttttt.....",
 "..tt..tt..tt..",
 ".tt...tt...tt.",
 "......tt......",
 "......tt......"
};
static const char *const spr_gavel[]={
 "oooooooo......",
 "owwwwwwwo.....",
 "owwwwwwwwo....",
 "oooooooooo....",
 "......ottto...",
 ".......ottto..",
 "........ottto.",
 ".........otto.",
 "..........oo..",
 "...oooooooooo.",
 "...owwwwwwwwo.",
 "...oooooooooo."
};
/* Original 16x16 miniature assets: beatbox microphone, naptime pillow, office badge, audit clipboard and CEO briefcase with gold crown. Each stays in ROM, never copied to a new framebuffer. */
static const char *const spr_mic[]={
  "................",
  ".......ooo......",
  ".....oowwpoo....",
  "..g..owppppo.g..",
  ".....opwpppo....",
  ".g...opppppo..g.",
  ".....opppppo....",
  ".....oqqqqqo....",
  "......qqqqq.....",
  "..g....qwq....g.",
  ".......qwq......",
  ".......qwq......",
  ".....ooooooo....",
  "....ootttttoo...",
  "....ooooooooo...",
  "................"
};
static const char *const spr_pillow[]={
  "................",
  "...g.......gg...",
  "...gg.....gy....",
  "...g......gyy...",
  "..........gyyyg.",
  "...........ggg..",
  "..oooooooooooo..",
  "..oiiiiiiiiiio..",
  "..oiuuuuuuuuio..",
  "..oiuulllluuio..",
  "..oiuulllluuio..",
  "..oiuuuuuuuuio..",
  "..oiiiiiiiiiio..",
  "..oppooooooppo..",
  "................",
  "................"
};
static const char *const spr_badge[]={
  ".......pp.......",
  ".......pp.......",
  ".....owwwwo.....",
  "..oooooooooooo..",
  "..ojjjjjjjjjjo..",
  "..ojwwwwwwwwjo..",
  "..ojjjjooojjjo..",
  "..ojjjosssojjo..",
  "..ojjjokskojjo..",
  "..ojjjosssojjo..",
  "..ojjjjorojjjo..",
  "..ojjjjjjjjjjo..",
  "..ojjwwwwwwjjo..",
  "..ojjiiiiijjjo..",
  "..ojjjjjjjjjjo..",
  "..oooooooooooo.."
};
static const char *const spr_audit[]={
  ".....oooooo.....",
  ".....oggggo.....",
  "..ooooggggoooo..",
  "..owwoooooowwo..",
  "..owwwwwwwwwwo..",
  "..owuuuuwqqqwo..",
  "..owwwwwwwwwwo..",
  "..owuuuuwqqqwo..",
  "..owwwwwwwwwwo..",
  "..owuuuuwqqtwo..",
  "..owwwwwwwtwwo..",
  "..owutuuwtqqwo..",
  "..owwwtwtwwwwo..",
  "..owwwwtwwwwwo..",
  "..owwwwwwwwwwo..",
  "..oooooooooooo.."
};
static const char *const spr_ceo[]={
  "................",
  ".......y........",
  "...y........y...",
  "....y.y.y.y.....",
  ".....y...y......",
  "....gggggggg....",
  "....gyyyyyyg....",
  "....oooooooo....",
  "......ollo......",
  ".oooooolloooooo.",
  ".ottttooootttto.",
  ".otmmmmmmmmmmto.",
  ".otttffggffttto.",
  ".otttttttttttto.",
  ".oooooooooooooo.",
  "................"
};
static uint16_t art_ink(char p){
 switch(p){
 case 'o':return INK;
 case 'h':return RGB(16,11,8);
 case 's':return RGB(31,24,20);
 case 'r':return RGB(31,18,19);
 case 'p':return PINK;
 case 'w':return WHITE;
 case 'b':return SKY;
 case 'g':return GOLD;
 case 't':return TEAL;
 case 'k':return RGB(5,7,11);
 /* Extended warm baby/material palette, using only 16-bit DS RGB5 colors. */
 case 'a':return RGB(27,18,16); /* skin shadow */
 case 'c':return RGB(31,29,25); /* romper collar */
 case 'd':return RGB(23,14,9);  /* chestnut sheen */
 case 'f':return RGB(22,30,23); /* mint fabric highlight */
 case 'i':return RGB(30,27,30); /* pillow ivory */
 case 'j':return RGB(15,26,29); /* badge sky */
 case 'l':return RGB(31,27,23); /* face highlight */
 case 'm':return RGB(12,24,21); /* jade cloth */
 case 'q':return RGB(12,11,16); /* muted ink */
 case 'u':return RGB(23,19,27); /* lavender shadow */
 case 'v':return RGB(21,10,13); /* small smile */
 case 'y':return RGB(31,28,16); /* sunlit gold */
 default:return 0;
 }
}
/* Group equal-color horizontal pixels to avoid one VRAM rectangle call
 * per sprite pixel. Transparent runs never write stale/background columns. */
static void sprite(int x,int y,int scale,const char *const *rows,int count){
 int yy,xx;
 if(scale<=0)return;
 for(yy=0;yy<count;yy++){
  const char *line=rows[yy];
  xx=0;
  while(line[xx]){
   uint16_t color=art_ink(line[xx]);
   int start=xx++;
   while(line[xx]&&art_ink(line[xx])==color)xx++;
   if(color)solid(x+start*scale,y+yy*scale,(xx-start)*scale,scale,color);
  }
 }
}
/* Reuse the game's existing ROM-native thumbnails instead of another
 * sprite sheet, decoder or new VRAM allocation. Actual mini-game objects
 * identify each arcade cabinet at a glance. */
/* Center original art within the 36x34 cabinet icon wells. Doubled
 * pixel density uses the same horizontal run blitter: no new image data
 * buffers, texture allocation, or decompression cost. */
static void cabinet_thumb(int id,int x,int y){
 switch(id){
 case 0:sprite(x+2,y-1,2,spr_bottle,18);break;
 case 1:sprite(x,y+6,2,spr_diaper,11);break;
 case 2:sprite(x+4,y+4,2,spr_receipt,13);break;
 case 3:sprite(x+4,y+6,2,spr_kart,11);break;
 case 4:sprite(x+2,y+1,2,spr_mic,16);break;
 case 5:sprite(x+4,y+4,2,spr_sock,13);break;
 case 6:sprite(x+2,y+1,2,spr_pillow,16);break;
 case 7:sprite(x+2,y,2,spr_rattle,17);break;
 case 8:sprite(x+2,y+1,2,spr_badge,16);break;
 case 9:sprite(x,y+4,2,spr_stroller,13);break;
 case 10:sprite(x+2,y+6,2,spr_cereal,11);break;
 case 11:sprite(x+4,y+8,2,spr_stack,9);break;
 case 12:sprite(x+2,y+1,2,spr_audit,16);break;
 case 13:sprite(x+4,y+4,2,spr_flower,13);break;
 case 14:sprite(x+4,y+5,2,spr_gavel,12);break;
 case 15:sprite(x+2,y+1,2,spr_ceo,16);break;
 }
}
/* Four generous, readable cabinets per page. The selected index is the
 * single source of truth: page = selected / 4, so keyboard, touch and
 * unlock state never drift apart. Page controls sit above the cards. */
static void arcade_list(const Game *s,int selection){
 int slot,id,x,y,opened,page=selection/4;
 unsigned need_wins,need_dollars,need_favors;
 char tmp[40];
 solid(0,27,256,149,IVORY);
 capsule(8,32,55,23,SKY,TEAL);
 fit(20,40,"< PREV",INK,8);
 capsule(193,32,55,23,SKY,TEAL);
 fit(203,40,"NEXT >",INK,8);
 /* The header already shows current dollars. Surface the other two
  * real resources here, so every paid cabinet can be compared against
  * the requirement on the NEXT line without hidden menus. 99+ caps
  * screen text, not inventory; the gameplay totals remain unchanged. */
 {
  unsigned w=s->wins>99u?99u:(unsigned)s->wins;
  unsigned f=s->favors>99u?99u:(unsigned)s->favors;
  snprintf(tmp,sizeof(tmp),"WINS %u%s FAVORS %u%s",
           w,s->wins>99u?"+":"",f,s->favors>99u?"+":"");
  fit(71,34,tmp,PURPLE,20);
 }
 /* Two readable lines share the existing empty strip between arrows.
  * Locked cabinets now explain the precise gameplay goal without opening
  * another menu or allocating a string/buffer every frame. */
 if(!game_unlocked(s,selection)&&
    game_unlock_requirements(selection,&need_wins,
                             &need_dollars,&need_favors)){
  snprintf(tmp,sizeof(tmp),"%u WINS $%u %u FAVOR",
           need_wins,need_dollars,need_favors);
  fit(71,46,tmp,ROSE,20);
 }else fit(80,46,"READY TO PLAY",TEAL,23);
 snprintf(tmp,sizeof(tmp),"%s",game_names[selection]);
 fit(12,60,tmp,PURPLE,38);
 for(slot=0;slot<4;slot++){
  uint16_t fill,edge;
  char num[16]; /* int formatting needs up to 11 bytes under -Werror */
  id=page*4+slot;
  x=9+(slot%2)*123;
  y=73+(slot/2)*50;
  opened=game_unlocked(s,id);
  fill=id==selection?LEMON:(opened?WHITE:GRAY);
  edge=id==selection?ROSE:(opened?TEAL:MUTED);
  panel(x,y,115,44,fill,edge);
  if(id==selection)solid(x+3,y+3,109,3,ROSE);
  capsule(x+6,y+5,36,34,id==selection?PINK:opened?MINT:SILVER,edge);
  cabinet_thumb(id,x+6,y+5);
  /* The game name is now double-size and selectable at a glance. */
  label(x+49,y+8,nick[id],INK,2,5);
  fit(x+49,y+28,opened?"READY":"LOCKED",opened?TEAL:ROSE,10);
  snprintf(num,sizeof(num),"%02d",id+1);
  fit(x+99,y+34,num,MUTED,2);
 }
 foot(PURPLE);
 fit(12,181,"A PLAY",WHITE,10);
 fit(101,181,game_unlocked(s,selection)?"X OWNED":"X UNLOCK",WHITE,14);
 fit(199,181,"B BACK",WHITE,10);
}
static void sparkle(int x,int y,uint16_t color){
 solid(x+3,y,2,9,color);solid(x,y+3,9,2,color);
 solid(x+2,y+2,4,4,WHITE);
}
/* Every cabinet inherits a unified theatrical frame. Previously each
 * redraw emitted ~140 dot primitives; this lightweight motif costs fewer
 * bitmap writes and gives every activity the same visual structure. */
/* Clean playfield mat: formerly the dashed strips on both edges looked
 * like vertical rendering faults and contributed visual clutter in every
 * mini-game. Keep one consistent top rule and distinct footer boundary. */
static void arcade_wall(uint16_t background){
 solid(0,27,256,149,background);
 solid(0,27,256,3,screen_accent);
 solid(0,174,256,2,screen_accent);
}
/* A quiet status rule instead of an extra dense row of pixel badges. */
static void cabinet_chrome(const Arcade *a){
 int i;
 int goal=a->id==4?3:a->id==1||a->id==6||a->id==13||
          a->id==14?4:a->id==9?5:6;
 solid(9,73,238,2,GRAY);
 for(i=0;i<goal;i++)
  solid(12+i*12,73,9,2,i<a->round?GREEN:TEAL);
 for(i=0;i<3;i++)
  solid(210+i*11,73,8,2,i<a->mistakes?ROSE:SILVER);
}
static void bar(int x,int y,int width,int value,int total,uint16_t color){
 int fill=total>0?value*(width-4)/total:0;
 if(fill<0)fill=0;
 if(fill>width-4)fill=width-4;
 capsule(x,y,width,12,WHITE,INK);
 solid(x+2,y+2,fill,8,color);
}
static void bottle_challenge(const Arcade *a){
 char text[40];
 /* Round away from zero: balance +/-23 must sit OUTSIDE the safe band,
  * rather than looking safe at the same pixel as the +/-22 boundary. */
 int offset=a->balance<0?-((-(int)a->balance+1)/2):((int)a->balance+1)/2;
 int cursor=128+offset;
 arcade_wall(BLUSH);
 panel(7,31,242,41,WHITE,TEAL);
 label(15,35,"BOTTLE BALANCE",PURPLE,2,14);
 fit(15,52,a->reverse?"GLITCH: LEFT IS RIGHT!":"STEADY HANDS, HAPPY BABY",a->reverse?DEEP_PINK:TEAL,37);
 bar(16,76,224,a->hold_frames,150,MINT);
 sprite(36,90,2,spr_baby,18);
 sprite(cursor+14,91,2,spr_bottle,18);
 sparkle(17,104,LEMON);sparkle(225,104,PINK);
 solid(25,132,206,3,TEAL);
 solid(128-MBAI_BOTTLE_SAFE_BALANCE/2,129,
       MBAI_BOTTLE_SAFE_BALANCE+1,9,MINT);
 solid(cursor-2,127,5,14,DEEP_PINK);
 snprintf(text,sizeof(text),"STEADY %u/150",a->hold_frames);
 fit(83,115,text,PURPLE,27);
 capsule(8,144,110,29,SKY,BLUE);
 capsule(138,144,110,29,LEMON,GOLD);
 label(33,151,"LEFT",INK,2,4);
 label(160,151,"RIGHT",INK,2,5);
 foot(PURPLE);
 snprintf(text,sizeof(text),"SPILLS %u/3   %u SEC",a->mistakes,(a->timer+59)/60);
 fit(19,181,text,WHITE,36);
}
static void teether_challenge(const Arcade *a){
 char text[40];
 arcade_wall(MINT);
 panel(6,31,244,41,WHITE,TEAL);
 label(13,35,"TAX TEETHER",PURPLE,2,11);
 snprintf(text,sizeof(text),"SORTED %u/6   %u SEC",a->round,(a->timer+59)/60);
 fit(13,53,text,DEEP_PINK,36);
 /* Office-nursery scene: receipts come from the baby executive's desk. */
 solid(91,76,74,4,GOLD);
 solid(94,79,6,48,TEAL);
 solid(155,79,6,48,TEAL);
 sprite(107,83,2,spr_baby,18);
 capsule(94,121,68,24,SKY,BLUE);
 sprite(114,111,2,spr_receipt,13);
 capsule(10,128,86,43,a->target==3?LEMON:SKY,GOLD);
 capsule(160,128,86,43,a->target==1?PINK:SKY,DEEP_PINK);
 for(int i=0;i<4;i++){
  solid(18+i*18,135,8,3,INK);
  solid(171+i*16,137,6,3,PURPLE);
 }
 label(29,145,"KEEP",INK,2,4);
 label(173,145,"SHRED",INK,2,5);
 sparkle(11,96,LEMON);sparkle(236,100,PINK);
 fit(10,77,a->target==3?"REFUND: SAVE IT!":"JUNK BILL: SHRED!",PURPLE,35);
 /* The held receipt belongs above the destination trays. Drawing it
  * before the bins made it disappear just before the player released. */
 if(a->touch_active&&a->touch_start_x>=99&&a->touch_start_x<160&&
    a->touch_start_y>=91&&a->touch_start_y<150)
  sprite(a->touch_last_x-14,a->touch_last_y-13,2,spr_receipt,13);
 foot(PURPLE);
 fit(9,181,"DRAG PAPER TO THE MATCHING BIN",WHITE,40);
}
static void kart_challenge(const Arcade *a){
 char text[40];
 int i,y;
 arcade_wall(SKY);
 panel(6,31,244,42,WHITE,TEAL);
 label(12,35,"CRIB KART",PURPLE,2,9);
 snprintf(text,sizeof(text),"SAFE %u/6  CRASH %u/3",a->round,a->mistakes);
 fit(12,54,text,DEEP_PINK,38);
 for(i=0;i<3;i++){
  int x=13+i*82;
  capsule(x,78,66,92,i==a->stage?MINT:GRAY,TEAL);
  solid(x+4,82,58,61,SHADOW);
  for(int dy=88;dy<142;dy+=18)
   solid(x+32,dy+(a->timer/4u)%13u,3,7,WHITE);
  fit(x+26,159,i==0?"L":i==1?"D":"R",PURPLE,1);
 }
 y=81+(a->hold_frames?
      (int)((a->hold_frames-a->timer)*48u/a->hold_frames):0);
 sprite(13+a->target*82+19,y,2,spr_cone,10);
 sprite(13+a->stage*82+20,137,2,spr_kart,11);
 sparkle(7,87,LEMON);sparkle(243,102,PINK);
 foot(PURPLE);
 fit(11,181,"CHANGE LANES BEFORE THE CONE!",WHITE,40);
}
static void sock_picture(int x,int y,int scale,int kind){
 static const uint16_t cloth[3]={PINK,TEAL,LEMON};
 sprite(x,y,scale,spr_sock,13);
 solid(x+5*scale,y+3*scale,3*scale,2*scale,cloth[kind%3]);
 solid(x+5*scale,y+7*scale,3*scale,2*scale,cloth[kind%3]);
}
static void sock_challenge(const Arcade *a){
 char text[40];int i;
 arcade_wall(BLUSH);
 panel(6,31,244,41,WHITE,PINK);
 label(12,35,"SOCK MARKET",PURPLE,2,11);
 snprintf(text,sizeof(text),"PAIRS %u/6  MISS %u/3",a->round,a->mistakes);
 fit(12,53,text,DEEP_PINK,38);
 capsule(9,76,238,40,MINT,TEAL);
 sock_picture(53,79,2,a->target);
 fit(96,86,"MATCH THIS",PURPLE,18);
 fit(96,97,"STRIPE COLOR",TEAL,21);
 for(i=0;i<3;i++){
  int x=13+i*82;
  capsule(x,121,66,53,WHITE,i==0?PINK:i==1?TEAL:GOLD);
  sock_picture(x+17,125,2,a->notes[i]);
  fit(x+27,164,i==0?"L":i==1?"D":"R",PURPLE,1);
 }
 foot(PURPLE);
 fit(12,181,"FIND THE MATCHING SOCK!",WHITE,38);
}
static void nap_challenge(const Arcade *a){
 char text[40];
 int phase=(int)(a->elapsed%a->hold_frames);
 int pos=20+phase*214/a->hold_frames;
 int green_x=20+38*214/100;
 int green_w=33*214/100;
 arcade_wall(BLUE);
 panel(6,31,244,41,WHITE,LILAC);
 label(13,35,"NAPTIME",PURPLE,2,7);
 snprintf(text,sizeof(text),"CALM %u/4  WAKEUPS %u/3",a->round,a->mistakes);
 fit(13,53,text,DEEP_PINK,39);
 /* Warm bedside light, sleep bonnet, pillow and a broad quiet window. */
 capsule(20,79,36,36,LEMON,GOLD);
 solid(37,82,21,23,BLUE);
 sparkle(209,89,WHITE);sparkle(236,111,LEMON);
 capsule(67,93,124,49,WHITE,TEAL);
 sprite(109,97,2,spr_baby,18);
 solid(102,125,51,5,SKY);
 capsule(15,145,226,22,WHITE,PURPLE);
 solid(20,151,214,9,GRAY);
 solid(green_x,151,green_w,9,MINT);
 solid(pos,146,4,20,DEEP_PINK);
 /* Dark legible instruction on the cream crib card. The previous WHITE
  * text was effectively invisible on the WHITE card below the baby. */
 fit(82,134,a->taped?"SWEET DREAMS!":"TAP IN GREEN",PURPLE,19);
 foot(PURPLE);
 snprintf(text,sizeof(text),"TAP IN GREEN! %u SEC  B BACK",(a->timer+59)/60);
 fit(12,181,text,WHITE,39);
}
/* Rhythm-Heaven-inspired legibility: one distinct instrument-shaped
 * silhouette for each pad, at 48x47 display size. These are our own 2D
 * pixel primitives, never an imported bitmap/VRAM texture. */
static void beat_pad_icon(int pad,int x,int y,int lit){
 uint16_t shine=lit?LEMON:WHITE;
 switch(pad){
 case 0: /* BOOM / drum */
  capsule(x+9,y+1,29,19,NIGHT,PURPLE);
  capsule(x+14,y+3,19,15,PINK,WHITE);
  solid(x+21,y+8,5,5,shine);
  break;
 case 1: /* CLAP / offset palms */
  solid(x+13,y+4,8,15,INK);
  solid(x+22,y+2,8,17,INK);
  solid(x+15,y+5,5,11,shine);
  solid(x+24,y+3,5,12,shine);
  solid(x+11,y+11,23,5,PINK);
  break;
 case 2: /* BEEP / toy synthesizer */
  capsule(x+7,y+2,35,19,NIGHT,PURPLE);
  solid(x+10,y+5,29,8,TEAL);
  solid(x+13,y+7,6,4,shine);
  solid(x+22,y+6,5,5,LEMON);
  solid(x+30,y+7,6,4,shine);
  break;
 default: /* POP / starburst bubble */
  capsule(x+13,y+2,22,18,PINK,WHITE);
  solid(x+18,y+5,8,4,shine);
  solid(x+9,y+9,5,3,LEMON);
  solid(x+37,y+8,5,4,LEMON);
  solid(x+27,y+17,4,3,ROSE);
  break;
 }
}
static void beatbox_challenge(const Arcade *a){
 static const char *const pads[4]={"BOOM","CLAP","BEEP","POP"};
 static const uint16_t colors[4]={MINT,PINK,LEMON,SKY};
 char text[40];
 int active=-1;
 arcade_wall(LILAC);
 panel(6,31,244,42,WHITE,PURPLE);
 label(12,35,"BABY BEATBOX",PURPLE,2,12);
 snprintf(text,sizeof(text),"TUNE %u/3  MISS %u/3",a->round+1,a->mistakes);
 fit(12,54,text,DEEP_PINK,36);
 if(!a->phase&&a->hold_frames){
  int index=(int)(a->elapsed*3u/a->hold_frames);
  if(index>2)index=2;
  active=a->notes[index];
 }
 /* A single emphatic WATCH/REPEAT cue avoids squashed microtext. */
 capsule(64,79,128,21,a->phase?MINT:LEMON,TEAL);
 label(a->phase?86:94,83,a->phase?"REPEAT":"WATCH",INK,2,6);
 sprite(111,101,1,spr_baby,18);
 for(int i=0;i<3;i++)
  capsule(143+i*18,103,13,11,a->phase?(i<a->stage?GREEN:GRAY):
          (i==(int)(a->elapsed*3u/(a->hold_frames?a->hold_frames:1u))?GOLD:GRAY),INK);
 for(int i=0;i<4;i++){
  int x=12+i*62;
  capsule(x,123,48,47,active==i?WHITE:colors[i],active==i?GOLD:PURPLE);
  beat_pad_icon(i,x,127,active==i);
  /* Four real 2x touch verbs. The 48px pad and its hitbox are unchanged. */
  label(x+(i==3?6:0),151,pads[i],INK,2,4);
 }
 foot(PURPLE);
 snprintf(text,sizeof(text),"%s  %u SEC   B BACK",a->phase?"REPEAT":"LISTEN",(a->timer+59)/60);
 fit(11,181,text,WHITE,39);
}
static void diaper_challenge(const Arcade *a){
 static const char *const steps[4]={
  "RUB OVER THE MESS!",
  "DRAG CLEAN DIAPER ON",
  "SWIPE UP TO FOLD",
  "TAP LEFT, THEN RIGHT"
 };
 char text[40];int i;
 arcade_wall(BLUSH);
 panel(6,31,244,41,WHITE,PINK);
 label(14,35,"DIAPER DASH",PURPLE,2,11);
 snprintf(text,sizeof(text),"STEP %u/4",a->stage+1);
 fit(184,37,text,DEEP_PINK,10);
 fit(14,52,steps[a->stage<4?a->stage:3],TEAL,37);
 /* Illustrated changing mat, not a directional-button quiz. */
 capsule(46,78,164,86,WHITE,LILAC);
 for(i=0;i<5;i++)solid(56+i*34,82,14,3,BLUSH);
 sprite(108,84,2,spr_baby,18);
 solid(103,119,51,23,SKY);
 sprite(106,129,2,spr_diaper,11);
 if(a->stage==0){
  solid(116,132,12,7,GOLD);solid(134,135,10,6,GOLD);
  sparkle(178,99,LEMON);
  bar(74,158,108,a->progress,90,PINK);
 }else if(a->stage==1){
  sprite(16,143,2,spr_diaper,11);
  if(a->touch_active&&a->touch_start_x<77&&a->touch_start_y>=143)
   sprite(a->touch_last_x-18,a->touch_last_y-12,2,spr_diaper,11);
  sparkle(178,117,LEMON);
  fit(12,135,"DRAG",DEEP_PINK,5);
 }else if(a->stage==2){
  for(i=0;i<3;i++)arrow(130,146-i*12,0,TEAL);
 }else{
  capsule(68,121,39,20,a->taped?MINT:LEMON,DEEP_PINK);
  capsule(149,121,39,20,LEMON,DEEP_PINK);
  fit(75,127,a->taped?"OK":"1",INK,4);
  fit(163,127,"2",INK,4);
 }
 foot(PURPLE);
 snprintf(text,sizeof(text),"MISS %u/3   %u SEC   B BACK",a->mistakes,(a->timer+59)/60);
 fit(11,181,text,WHITE,40);
}
static void rattle_challenge(const Arcade *a){
 char text[40];
 int lane,y,i,fall;
 arcade_wall(SKY);
 panel(6,31,244,40,WHITE,TEAL);
 label(13,35,"RATTLE RESCUE",PURPLE,2,13);
 snprintf(text,sizeof(text),"CAUGHT %u/6  MISSED %u/3",a->round,a->mistakes);
 fit(13,53,text,TEAL,38);
 for(i=0;i<3;i++){
  int x=13+i*82;
  capsule(x,76,66,91,i==a->target?WHITE:BLUSH,LILAC);
  solid(x+4,130,58,29,LEMON);
  outline(x+4,130,58,29,GOLD);
  fit(x+27,159,i==0?"L":i==1?"D":"R",PURPLE,1);
 }
 lane=13+a->target*82;
 fall=a->hold_frames?(int)((a->hold_frames-a->timer)*64u/a->hold_frames):0;
 y=80+fall;
 sprite(lane+17,y,2,spr_rattle,17);
 sparkle(8,90,PINK);sparkle(240,105,LEMON);
 foot(PURPLE);
 fit(11,181,"TAP GOLD ZONE AS RATTLE FALLS",WHITE,39);
}
static void crawl_challenge(const Arcade *a){
 char text[40];int x;
 arcade_wall(SKY);
 panel(6,31,244,42,WHITE,TEAL);
 label(13,35,"OFFICE CRAWL",PURPLE,2,12);
 snprintf(text,sizeof(text),"ESCAPED %u/6  BUMPS %u/3",a->round,a->mistakes);
 fit(13,54,text,DEEP_PINK,39);
 capsule(14,81,229,52,BLUSH,PINK);
 solid(14,124,229,8,PURPLE);
 for(x=16;x<237;x+=34)solid(x,121,16,3,GOLD);
 sprite(49,a->stage==0?89:a->stage==2?109:100,2,spr_baby,18);
 x=204-(int)((a->hold_frames-a->timer)*117u/a->hold_frames);
 if(a->target==0){
  /* Jump across the low office supplies crate. */
  capsule(x,115,26,17,LEMON,GOLD);
  solid(x+6,111,14,5,PINK);
 }else{
  /* Duck beneath the overhead HR scanning arm. */
  solid(x+3,85,6,45,TEAL);
  solid(x-16,85,29,9,DEEP_PINK);
  solid(x-9,94,13,5,LEMON);
 }
 capsule(10,141,109,31,a->stage==0?LEMON:MINT,TEAL);
 capsule(137,141,109,31,a->stage==2?LEMON:PINK,DEEP_PINK);
 label(41,149,"JUMP",INK,2,4);
 label(168,149,"DUCK",INK,2,4);
 foot(PURPLE);
 fit(10,181,a->target==0?"JUMP OVER THE CRATE!":"DUCK UNDER THE SCANNER!",WHITE,40);
}
static void stroller_challenge(const Arcade *a){
 char text[40];int x=128+a->balance;
 arcade_wall(MINT);
 panel(6,31,244,41,WHITE,TEAL);
 label(13,35,"STROLLER DRIFT",PURPLE,2,14);
 snprintf(text,sizeof(text),"LAPS %u/5  BUMPS %u/3",a->round,a->mistakes);
 fit(13,53,text,DEEP_PINK,37);
 solid(42,77,171,66,SHADOW);
 for(int i=0;i<4;i++){
  int y=81+i*17;
  solid(47,y,7,8,WHITE);solid(201,y,7,8,WHITE);
 }
 solid(101,77,54,65,GRAY);
 solid(101,77,3,65,LEMON);
 solid(152,77,3,65,LEMON);
 for(int i=0;i<5;i++)solid(127,82+i*13,3,6,WHITE);
 sprite(x-16,102,2,spr_stroller,13);
 bar(75,144,108,a->hold_frames,70,MINT);
 fit(88,158,"KEEP CENTER!",PURPLE,21);
 capsule(8,143,57,29,SKY,BLUE);
 capsule(191,143,57,29,LEMON,GOLD);
 fit(14,154,"LEFT",INK,7);
 fit(198,154,"RIGHT",INK,7);
 foot(PURPLE);
 snprintf(text,sizeof(text),"STAY ON TRACK   %u SEC",(a->timer+59)/60);
 fit(16,181,text,WHITE,39);
}
static void cereal_challenge(const Arcade *a){
 char text[40];
 arcade_wall(LEMON);
 panel(6,31,244,42,WHITE,GOLD);
 label(12,35,"CEREAL CIPHER",PURPLE,2,13);
 snprintf(text,sizeof(text),"SOLVED %u/6  WRONG %u/3",a->round,a->mistakes);
 fit(12,54,text,DEEP_PINK,38);
 capsule(36,79,184,33,WHITE,PURPLE);
 snprintf(text,sizeof(text),"%u + %u = ?",a->stage,a->direction);
 fit(83,91,text,PURPLE,18);
 sparkle(18,95,WHITE);sparkle(232,95,PINK);
 for(int i=0;i<3;i++){
  int x=13+i*82;
  capsule(x,119,66,55,i==0?PINK:i==1?MINT:SKY,PURPLE);
  sprite(x+17,121,1,spr_cereal,11);
  snprintf(text,sizeof(text),"%u",a->notes[i]);
  fit(x+28,139,text,INK,2);
  fit(x+28,160,i==0?"L":i==1?"D":"R",PURPLE,1);
 }
 foot(PURPLE);
 fit(12,181,"COUNT CEREAL: PICK THE SUM!",WHITE,38);
}
static void blocks_challenge(const Arcade *a){
 char text[40];int i;
 arcade_wall(LILAC);
 panel(6,31,244,41,WHITE,PURPLE);
 label(12,35,"TANTRUM BLOCKS",PURPLE,2,14);
 snprintf(text,sizeof(text),"STACK %u/6  MISSED %u/3",a->round,a->mistakes);
 fit(12,53,text,DEEP_PINK,38);
 capsule(12,77,232,94,SKY,PURPLE);
 for(i=0;i<3;i++){
  int x=35+i*81;
  solid(x,100,58,4,i==a->target?LEMON:WHITE);
  if(i==a->target)sparkle(x+26,94,LEMON);
 }
 /* The marker glides from side to side until the player drops it. */
 sprite(42+a->direction*81,83,2,spr_stack,9);
 for(i=0;i<a->round;i++){
  int y=144-i*10;
  capsule(90+(i&1)*6,y,78,10,i&1?MINT:PINK,DEEP_PINK);
 }
 capsule(73,151,109,18,LEMON,GOLD);
 fit(105,157,"DROP!",INK,9);
 foot(PURPLE);
 snprintf(text,sizeof(text),"A / TAP TO DROP   %u SEC",(a->timer+59)/60);
 fit(13,181,text,WHITE,39);
}
static void audit_challenge(const Arcade *a){
 char text[40];
 arcade_wall(MINT);
 panel(6,31,244,41,WHITE,TEAL);
 label(12,35,"BABY AUDIT",PURPLE,2,10);
 snprintf(text,sizeof(text),"CHECKED %u/6  WRONG %u/3",a->round,a->mistakes);
 fit(12,53,text,DEEP_PINK,38);
 /* Two legible invoice numbers on a stamped clipboard. */
 panel(31,77,194,52,WHITE,PURPLE);
 solid(94,74,68,8,GOLD);
 fit(46,85,"INVOICE",TEAL,13);
 fit(151,85,"RECEIPT",TEAL,13);
 snprintf(text,sizeof(text),"%u",a->stage);
 fit(60,104,text,PURPLE,3);
 snprintf(text,sizeof(text),"%u",a->direction);
 fit(161,104,text,PURPLE,3);
 solid(127,89,2,34,LILAC);
 capsule(11,137,109,35,LEMON,GOLD);
 capsule(136,137,109,35,PINK,DEEP_PINK);
 label(25,147,"APPROVE",INK,2,7);
 label(161,147,"FLAG",INK,2,4);
 sparkle(12,92,LEMON);sparkle(241,110,PINK);
 foot(PURPLE);
 fit(10,181,"MATCHING TOTALS? APPROVE OR FLAG",WHITE,40);
}
static void garden_challenge(const Arcade *a){
 char text[40];int i;
 arcade_wall(SKY);
 panel(6,31,244,42,WHITE,GREEN);
 label(13,35,"GLITCH GARDEN",PURPLE,2,13);
 snprintf(text,sizeof(text),"FLOWERS %u/4  WITHERED %u/3",a->round,a->mistakes);
 fit(13,54,text,DEEP_PINK,39);
 for(i=0;i<3;i++){
  int x=13+i*82;
  capsule(x,85,66,89,i==a->target?MINT:BLUSH,GREEN);
  solid(x+7,146,52,22,GOLD);
  solid(x+14,142,37,6,PURPLE);
  sprite(x+20,108,2,spr_flower,13);
  if(i==a->target){
   solid(x+7,95,52,4,GRAY);
   solid(x+7,95,(a->progress*52)/100,4,TEAL);
   sparkle(x+4,92,LEMON);
  }
  fit(x+27,162,i==0?"L":i==1?"D":"R",WHITE,1);
 }
 foot(PURPLE);
 fit(13,181,"HOLD TO WATER THE LIT FLOWER!",WHITE,40);
}
static int court_node_x(int node){return node==0?46:node==1?128:210;}
static int court_node_y(int node){return node==1?92:134;}
static void court_challenge(const Arcade *a){
 /* Three clue labels are constant: avoid warning-prone formatting
  * and per-frame string allocations/formatting work on the DS. */
 static const char *const clue_numbers[3]={"1","2","3"};
 char text[40];int i,j;
 arcade_wall(LEMON);
 panel(6,31,244,42,WHITE,GOLD);
 label(12,35,"CRAYON COURT",PURPLE,2,12);
 snprintf(text,sizeof(text),"CASES %u/4  OBJECTIONS %u/3",a->round,a->mistakes);
 fit(12,54,text,DEEP_PINK,40);
 /* Numbered clues form a new three-node route every case. */
 for(i=0;i<2;i++){
  int x0=court_node_x(a->notes[i]),y0=court_node_y(a->notes[i]);
  int x1=court_node_x(a->notes[i+1]),y1=court_node_y(a->notes[i+1]);
  for(j=1;j<12;j++){
   int x=x0+(x1-x0)*j/12;
   int y=y0+(y1-y0)*j/12;
   solid(x-2,y-2,4,4,i<a->stage?GREEN:LILAC);
  }
 }
 for(i=0;i<3;i++){
  int x=court_node_x(a->notes[i]),y=court_node_y(a->notes[i]);
  capsule(x-19,y-18,38,37,i<a->stage?MINT:PINK,DEEP_PINK);
  fit(x-2,y-3,clue_numbers[i],INK,1);
 }
 sprite(14,82,2,spr_gavel,12);
 foot(PURPLE);
 fit(9,181,"TRACE 1 TO 2 TO 3   (L UP R)",WHITE,40);
}
static void ceo_challenge(const Arcade *a){
 static const char *const actions[3]={"FUND","PLAY","NAP"};
 char text[40];int i;
 arcade_wall(BLUSH);
 panel(6,31,244,41,WHITE,PURPLE);
 label(12,35,"DAYCARE CEO",PURPLE,2,11);
 snprintf(text,sizeof(text),"DECISIONS %u/6  FAILS %u/3",a->round,a->mistakes);
 fit(12,53,text,DEEP_PINK,38);
 panel(15,78,226,38,WHITE,LILAC);
 fit(23,84,"CASH",PURPLE,5);
 fit(23,98,"MORALE",PURPLE,8);
 solid(76,85,150,8,GRAY);
 solid(76,99,150,8,GRAY);
 solid(76,85,a->balance*150/100,8,TEAL);
 solid(76,99,a->progress*150/100,8,PINK);
 solid(76+35*150/100,82,2,28,GOLD);
 for(i=0;i<3;i++){
  int x=13+i*82;
  capsule(x,121,66,51,i==0?MINT:i==1?PINK:SKY,PURPLE);
  /* Distinct desk-ledger, play block and crescent icons. These are not
   * interchangeable abstract squares anymore; the captions remain 2x. */
  if(i==0){
   capsule(x+22,129,22,18,LEMON,GOLD);
   solid(x+26,132,3,12,INK);
   solid(x+31,134,9,2,WHITE);
   solid(x+31,139,9,2,WHITE);
  }else if(i==1){
   capsule(x+24,128,18,18,SKY,BLUE);
   solid(x+28,131,10,10,PINK);
   solid(x+32,129,2,14,WHITE);
  }else{
   capsule(x+22,129,22,18,LEMON,GOLD);
   solid(x+33,130,13,14,SKY);
   solid(x+26,133,5,4,WHITE);
  }
  label(x+12,149,actions[i],INK,2,4);
 }
 foot(PURPLE);
 fit(9,181,"BALANCE THE BOOKS AND BABY!",WHITE,40);
}
static void challenge(const Game *s,const Arcade *a){
 char tmp[40];int i;
 solid(0,27,256,149,IVORY);
 panel(5,32,246,37,WHITE,LILAC);
 fit(12,39,game_names[a->id],INK,38);
 snprintf(tmp,sizeof(tmp),"ROUND %u/6  MISS %u/3",a->round+1,a->mistakes);
 fit(12,53,tmp,PURPLE,38);
 if(a->reverse){
  solid(0,72,256,16,DEEP_PINK);
  label(60,77,"GLITCH: REVERSE!",WHITE,1,24);
 }else{
  solid(0,72,256,16,MINT);
  label(72,77,"FOLLOW ARROW",INK,1,24);
 }
 capsule(66,90,124,73,SKY,TEAL);
 solid(74,98,108,57,WHITE);
 arrow(a->direction==1?155:a->direction==3?103:127,
       a->direction==0?102:a->direction==2?127:125,
       a->direction,a->reverse?DEEP_PINK:BLUE);
 for(i=0;i<6;i++)solid(92+i*12,166,9,5,i<a->round?GREEN:LILAC);
 foot(PURPLE);
 snprintf(tmp,sizeof(tmp),"%u FRAMES   B BACK",a->timer);
 fit(30,181,tmp,WHITE,30);
 (void)s;
}
/* The finale is a collectible illustrated postcard rather than a plain
 * text result, while retaining the same continue-on-any-input behavior. */
/* The ending is an actual keepsake: clean character art, one dominant
 * outcome label, one sentence of reward and a single continue affordance. */
static void finale(const Game *s,int ending){
 if(ending==END_EGG){
  /* A coherent terminal art card: muted vault, unmistakable cracked egg
   * pictogram, and pale-green reveal instead of random flat rectangles. */
  solid(0,27,256,149,NIGHT);
  panel(10,34,236,138,NIGHT,GREEN);
  capsule(23,42,210,27,INK,ROSE);
  label(63,49,"BAD ENDING",WHITE,2,10);
  capsule(82,74,92,48,INK,TEAL);
  sprite(97,74,2,spr_egg,24);
  fit(27,124,"THE NURSERY WAS AN INCUBATOR",WHITE,37);
  solid(25,136,206,2,GREEN);
  fit(61,142,"FILE 09: UNSEALED",LEMON,27);
  fit(32,157,"THE THING INSIDE REMEMBERS.",MINT,37);
  foot(PURPLE);
  fit(15,182,"A CONTINUE      B CONTINUE",WHITE,37);
  (void)s;
  return;
 }
 int good=game_is_good_ending(ending);
 uint16_t edge=good?GREEN:ROSE;
 char count[36];
 solid(0,27,256,149,good?MINT:BLUSH);
 panel(10,34,236,137,WHITE,edge);
 capsule(18,40,220,27,good?MINT:PINK,edge);
 label(good?62:68,46,good?"GOOD ENDING":"BAD ENDING",INK,2,11);
 capsule(89,72,78,44,IVORY,edge);
 sprite(108,76,2,spr_baby,18);
 insignia(41,87,good?GOLD:ROSE);
 insignia(205,87,good?GOLD:ROSE);
 fit(20,122,game_ending_name(ending),PURPLE,36);
 solid(23,133,208,1,edge);
 fit(24,139,good?"+$8 AND +1 FAVOR":"ANOTHER DAY FOR BABY",good?GREEN:ROSE,34);
 snprintf(count,sizeof(count),"GOOD MEMORIES %d/4",game_good_ending_count(s));
 fit(24,154,count,MUTED,28);
 foot(PURPLE);
 fit(15,182,"A CONTINUE      B CONTINUE",WHITE,37);
}
/* Synthetic-only slapstick lab: no realistic injury or gore, just
 * a toy shell that rebounds and develops an ominous stress signal. */
/* Talking-Tom-like tool selection on the DS's *lower* touch display:
 * select SLAP/KICK on the dock, then tap the doll to apply the selected
 * impulse. The upper DS 3D screen is intentionally not a touchscreen. */
static void roughhouse_screen(int encoded){
 int selected=(encoded&32)?ROUGHHOUSE_KICK:ROUGHHOUSE_SLAP;
 int charge=encoded&31;
 int hits=(encoded>>6)&255;
 int audio=(encoded&(1<<16))!=0;
 int filled=charge*208/(int)ROUGHHOUSE_CRACK_CHARGE;
 char count[22];
 if(filled>208)filled=208;
 solid(0,27,256,149,IVORY);
 panel(9,31,238,45,WHITE,ROSE);
 label(14,36,"TOYBOX",PURPLE,2,6);
 if(hits>0){
  snprintf(count,sizeof(count),"HITS %d/6",hits>6?6:hits);
  fit(177,42,count,ROSE,10);
 }else fit(139,42,"SELECT A TOOL",MUTED,16);
 fit(15,57,charge>=11?"UNSEALED...":charge>=6?"CRACKED SHELL":"SHELL STRESS",
     charge>=11?GREEN:ROSE,18);
 fit(187,57,audio?"SFX ON":"SFX OFF",audio?GREEN:MUTED,7);
 solid(15,68,219,5,GRAY);
 solid(19,69,filled,3,ROSE);
 panel(10,79,236,62,WHITE,LILAC);
 capsule(95,83,65,52,LEMON,GOLD);
 sprite(111,87,2,spr_baby,18);
 label(21,97,"TAP",INK,2,3);
 fit(20,115,"TO USE",PURPLE,9);
 label(184,97,"DOLL",INK,2,4);
 fit(184,115,charge>=11?"ALIEN?":charge>=6?"AWAKE?":"BOING!",
     charge>=11?GREEN:ROSE,9);
 capsule(9,145,115,29,selected==ROUGHHOUSE_SLAP?PINK:GRAY,
         selected==ROUGHHOUSE_SLAP?ROSE:PURPLE);
 /* A mitten/hand is the selection icon, not a violent depiction. */
 solid(20,151,7,11,WHITE);
 solid(21,148,5,5,WHITE);
 solid(28,156,3,7,WHITE);
 label(46,153,"SLAP",INK,2,4);
 capsule(132,145,115,29,selected==ROUGHHOUSE_KICK?LEMON:GRAY,
         selected==ROUGHHOUSE_KICK?GOLD:PURPLE);
 /* Boot-shaped tool icon remains an 8-polygon fixed bitmap primitive. */
 solid(143,151,7,10,WHITE);
 solid(148,159,13,5,WHITE);
 solid(157,161,3,3,PURPLE);
 label(171,153,"KICK",INK,2,4);
 foot(PURPLE);
 fit(7,181,"A USE  L/R TOOL  B BACK  Y SFX",WHITE,40);
}
/* Containment-monitor staging. Layout is intentionally compact enough
 * for a real DS: broad 2x warning, high-contrast readout, tiny wireframe
 * silhouette and three ACTUAL tappable controls. */
static void egg_icon(int x,int y,int scary){
 uint16_t c=scary?GREEN:TEAL;
 outline(x+3,y+1,39,42,c);
 /* Silhouette, pointed eyes, jaw and four unmistakable fangs. */
 solid(x+12,y+8,22,3,c);
 solid(x+8,y+11,30,15,NIGHT);
 solid(x+10,y+13,12,5,c);solid(x+25,y+13,12,5,c);
 solid(x+14,y+14,4,4,INK);solid(x+29,y+14,4,4,INK);
 solid(x+11,y+25,25,4,c);
 solid(x+13,y+29,21,9,INK);
 solid(x+13,y+29,3,7,IVORY);solid(x+20,y+29,3,8,IVORY);
 solid(x+28,y+29,3,8,IVORY);solid(x+34,y+29,3,7,IVORY);
 solid(x+16,y+38,15,2,c);
}
static void egg_scene_screen(int selection){
 int frame=selection&0xffff;
 int paused=(selection&EGG_FILM_PAUSED_FLAG)!=0;
 int phase=frame<(int)EGG_CRACK_FRAME?0:
           frame<(int)EGG_PUSH_FRAME?1:
           frame<(int)EGG_SCARE_FRAME?2:3;
 int i;
 char integrity[8];
 static const unsigned integrity_steps[4]={98u,64u,12u,3u};
 uint16_t warning=phase==3?ROSE:GREEN;
 snprintf(integrity,sizeof(integrity),"%02u%%",integrity_steps[phase]);
 solid(0,27,256,149,NIGHT);
 panel(8,30,240,142,INK,warning);
 solid(12,33,232,2,TEAL);
 fit(17,37,"INCUBATOR",WHITE,12);
 fit(142,37,"SPECIMEN 09",MINT,16);
 /* Wide alert banner is intentionally the screen's focal heading. */
 capsule(14,49,228,27,phase==3?ROSE:PURPLE,phase==3?PINK:GREEN);
 if(phase==3)label(20,55,"CONTAINMENT BREACH",WHITE,2,18);
 else if(phase==2)label(37,55,"IT IS AWAKE",WHITE,2,13);
 else if(phase==1)label(31,55,"SHELL BREACH",WHITE,2,14);
 else label(43,55,"SIGNAL FOUND",WHITE,2,13);
 fit(18,82,"SPECIMEN 09 / UNIDENTIFIED",MINT,29);
 solid(18,95,3,29,phase==3?ROSE:GREEN);
 if(phase==0){
  fit(27,96,"SYNTHETIC SHELL",WHITE,26);
  fit(27,109,"SECOND HEARTBEAT",LEMON,26);
 }else if(phase==1){
  fit(27,96,"PORCELAIN FRACTURED",WHITE,26);
  fit(27,109,"BIOFLUID DETECTED",LEMON,26);
 }else if(phase==2){
  fit(27,96,"EYES IN THE CAVITY",WHITE,26);
  fit(27,109,"SOMETHING SEES YOU",LEMON,26);
 }else{
  fit(27,96,"IT RECOGNIZED YOU",WHITE,26);
  fit(27,109,"CONTAINMENT FAILED",PINK,26);
 }
 egg_icon(190,83,phase>=2);
 /* The fictional monitor progressively loses containment in lockstep with
  * the cinematic acts. Staged numbers update ONLY at phase changes: no
  * busy per-frame VRAM repaint or simulated live medical data. */
 fit(19,126,"SHELL",MINT,6);
 fit(61,126,integrity,phase==3?PINK:LEMON,6);
 fit(120,126,phase==3?"BREACHED":paused?"ON HOLD":"UNSTABLE",
     phase==3?PINK:LEMON,12);
 /* The meter is at y=137, x=31+52*i by contract: emulator playback tests
  * identify the four phases using THESE pixels. Do not move them. */
 for(i=0;i<4;i++)solid(27+i*52,136,45,3,i<=phase?LEMON:GRAY);
 /* Below the fixed phase meter: microtelemetry like the art target.
  * Tiny bands remain readable without constantly re-uploading 256x192. */
 outline(15,143,73,17,TEAL);
 outline(92,143,72,17,TEAL);
 outline(168,143,74,17,TEAL);
 fit(20,146,"VITALS",MINT,10);
 fit(97,146,"SIGNAL",MINT,10);
 fit(174,146,phase==3?"DANGER":"BREACH",phase==3?PINK:MINT,10);
 solid(20,155,6,1,GREEN);solid(26,152,2,4,GREEN);
 solid(28,155,5,1,GREEN);solid(33,151,2,5,GREEN);
 solid(35,155,12,1,GREEN);solid(47,154,2,3,GREEN);
 solid(49,155,28,1,GREEN);
 for(i=0;i<6;i++)solid(98+i*9,157-i,6,i+1,GREEN);
 for(i=0;i<5;i++)solid(175+i*12,155,9,2,i<=phase?ROSE:GRAY);
 fit(paused?76:43,163,
     paused?"FILM PAUSED":phase==3?"SUBJECT AT CAMERA":"TAP READOUT FOR NEXT",
     paused?LEMON:phase==3?PINK:MINT,35);
 /* These are actual footer controls, aligned to their unchanged three
  * touch zones. B/Start, X, and A remain direct button equivalents. */
 foot(PURPLE);
 capsule(4,178,79,13,NIGHT,TEAL);
 capsule(88,178,79,13,paused?ROSE:NIGHT,paused?PINK:TEAL);
 capsule(173,178,79,13,NIGHT,GREEN);
 fit(23,181,"B SKIP",WHITE,9);
 fit(105,181,paused?"X RESUME":"X PAUSE",WHITE,9);
 fit(194,181,"A NEXT",WHITE,9);
}
/* Redraw diagnostic panel once on entry, then update only tiny regions.
 * Repainting 256x192 VRAM every stylus-held frame was tearing/flickering
 * on DraStic. No extra framebuffer or VRAM is needed.
 */
static int diag_last_raw=-1,diag_last_x=-1,diag_last_y=-1;
static int diag_cross_x=-1,diag_cross_y=-1;
static unsigned long diag_last_count=~0ul;
static unsigned diag_last_vertices=~0u,diag_last_polygons=~0u;
/* Clear touchscreen diagnostics, preserving the untouched 220x52
 * BLUSH test canvas: crosshair erasure relies on that solid background. */
static void touch_diagnostic(void){
 solid(0,27,256,149,IVORY);
 panel(7,32,242,143,WHITE,TEAL);
 capsule(14,39,228,22,NIGHT,TEAL);
 label(53,43,"STYLUS TEST",WHITE,2,11);
 fit(17,65,"KEY_TOUCH:",PURPLE,14);
 fit(17,79,"X/Y:",PURPLE,5);
 fit(17,93,"TAPS:",PURPLE,5);
 solid(18,108,220,52,BLUSH);
 outline(18,108,220,52,TEAL);
 /* Lightweight geometry submission diagnostic. No expensive FPS guess:
  * values reflect exactly what the top-screen 3D renderer submitted. */
 fit(19,164,"VERTS:",MUTED,6);
 fit(135,164,"POLYS:",MUTED,6);
 foot(PURPLE);
 fit(10,181,"DRAW AREA   B/START/SELECT BACK",WHITE,39);
 diag_last_raw=-1;diag_last_x=-1;diag_last_y=-1;
 diag_cross_x=-1;diag_cross_y=-1;
 diag_last_count=~0ul;
 diag_last_vertices=~0u;diag_last_polygons=~0u;
}
void ui_touch_diag_update(unsigned vertices,unsigned polygons){
 char tmp[36];
 int x,y;
 if(!pixels)return;
 if(diag_last_raw!=test_touch_down){
  solid(104,63,132,12,WHITE);
  fit(108,65,test_touch_down?"DOWN - INPUT SEEN":"UP / NO SIGNAL",
      test_touch_down?GREEN:DEEP_PINK,21);
  diag_last_raw=test_touch_down;
 }
 if(diag_last_x!=test_touch_x||diag_last_y!=test_touch_y){
  solid(63,78,169,12,WHITE);
  snprintf(tmp,sizeof(tmp),"%03d / %03d",test_touch_x,test_touch_y);
  fit(68,79,tmp,INK,26);
  diag_last_x=test_touch_x;
  diag_last_y=test_touch_y;
 }
 if(diag_last_count!=test_touch_count){
  solid(63,92,169,12,WHITE);
  snprintf(tmp,sizeof(tmp),"%lu",test_touch_count);
  fit(68,93,tmp,INK,24);
  diag_last_count=test_touch_count;
 }
 /* Update only these two small status regions when geometry totals
  * actually change. Values beyond the portable submission budget are
  * visibly red instead of silently declaring the scene healthy. */
 if(diag_last_vertices!=vertices){
  solid(59,163,65,11,WHITE);
  snprintf(tmp,sizeof(tmp),"%u",vertices);
  fit(62,164,tmp,vertices>5000u?ROSE:TEAL,8);
  diag_last_vertices=vertices;
 }
 if(diag_last_polygons!=polygons){
  solid(173,163,65,11,WHITE);
  snprintf(tmp,sizeof(tmp),"%u",polygons);
  fit(176,164,tmp,polygons>1850u?ROSE:TEAL,8);
  diag_last_polygons=polygons;
 }
 /* Only erase/draw a 13x13 patch within the little touch canvas. */
 if(!test_touch_down||test_touch_x<0||test_touch_y<0||
    test_touch_x>255||test_touch_y>191){
  /* Release must remove the previous crosshair exactly once. Otherwise
   * the touch-test canvas appears to show a stylus still being held. */
  if(diag_cross_x>=0){
   solid(diag_cross_x-7,diag_cross_y-7,15,15,BLUSH);
   diag_cross_x=-1;diag_cross_y=-1;
  }
  return;
 }
 x=31+(test_touch_x*194)/255;
 y=119+(test_touch_y*30)/191;
 if(x==diag_cross_x&&y==diag_cross_y)return;
 if(diag_cross_x>=0)
  solid(diag_cross_x-7,diag_cross_y-7,15,15,BLUSH);
 solid(x-1,y-6,3,13,DEEP_PINK);
 solid(x-6,y-1,13,3,DEEP_PINK);
 diag_cross_x=x;diag_cross_y=y;
}
/* The top screen provides the character portrait, while the bottom
 * presents one unmistakable action and a comfortable 2x wordmark. */
static void title_screen(const Game *s,int storage_ok,const char *status){
 (void)s;
 solid(0,27,256,149,IVORY);
 panel(9,33,238,137,WHITE,TEAL);
 capsule(16,39,224,27,LILAC,PURPLE);
 label(48,45,"MY BABY A.I.!",INK,2,13);
 capsule(85,70,86,57,MINT,TEAL);
 sprite(98,72,3,spr_baby,18);
 fit(18,76,"INFANTS",ROSE,10);
 fit(18,87,"OUTLAWED",ROSE,10);
 solid(18,99,60,1,BLUSH);
 fit(18,108,"HIDDEN",PURPLE,10);
 fit(18,119,"SHELTER",PURPLE,10);
 fit(178,76,"WARDEN",TEAL,10);
 fit(178,87,"IS WATCHING",TEAL,11);
 solid(178,99,60,1,BLUSH);
 fit(178,108,"SOMETHING",ROSE,10);
 fit(178,119,"IS WRONG",ROSE,10);
 fit(18,129,status?status:"WELCOME!",TEAL,36);
 /* Two real, high-contrast title actions, matched to their touch bounds.
  * The START button remains separate from save-device probing, which
  * intentionally occurs only on an explicit user request. */
 capsule(19,140,132,28,LEMON,GOLD);
 label(58,147,"START",INK,2,5);
 capsule(159,140,78,28,MINT,TEAL);
 fit(171,150,"SAVE/LOAD",INK,11);
 foot(PURPLE);
 fit(10,181,"A START   X SAVE   SELECT TEST",WHITE,39);
 (void)storage_ok;
}
/* Match the visible 78x28 right-hand title card, with no accidental
 * touches on the baby portrait, START card or off-screen coordinates. */
int ui_title_save_touch(int x,int y){
 return x>=159&&x<237&&y>=140&&y<168;
}
/* Collection stamps carry ONE readable label apiece instead of two
 * crowded microtext lines. Color indicates discovered good/bad endings. */
/* Scrapbook uses two calm 2x2 pages, not eight 22px-high strips.
 * Each keepsake has readable number, title and discovery state. */
static void ending_album(const Game *s,int page){
 static const char *const names[8]={
  "BOTTLE TAX","BABY CEO","REBOOT","INTERN RISE",
  "GRADUATION","ARCADE STAR","GLITCH HERO","SECOND LIFE"
 };
 int row,col,id,x,y,seen=0;char tmp[36];
 if(page>2)page=2;
 if(page<0)page=0;
 if(page==2){
  int discovered=(s->ending_mask&(1u<<END_EGG))!=0;
  solid(0,27,256,149,NIGHT);
  capsule(9,32,55,28,SKY,TEAL);
  fit(17,42,"< PREV",INK,8);
  capsule(192,32,55,28,SKY,TEAL);
  fit(201,42,"NEXT >",INK,8);
  fit(95,40,"PAGE 3/3",WHITE,12);
  panel(19,67,218,101,NIGHT,discovered?GREEN:PURPLE);
  capsule(82,74,92,48,INK,discovered?TEAL:GRAY);
  if(discovered){
   sprite(97,74,2,spr_egg,24);
   fit(33,130,"09  THE INCUBATOR WAS ALIVE",WHITE,36);
   fit(68,148,"TAP TO REPLAY",LEMON,30);
  }else{
   fit(113,90,"09?",WHITE,5);
   fit(51,130,"09  ???  SEALED",WHITE,30);
   fit(42,148,"SECRET MEMORY NOT FOUND",MUTED,35);
  }
  foot(PURPLE);
  fit(11,181,"B BACK   L/R CHANGE PAGE",WHITE,40);
  return;
 }
 solid(0,27,256,149,IVORY);
 for(id=1;id<=8;id++)if(s->ending_mask&(1u<<id))seen++;
 capsule(9,32,55,28,SKY,TEAL);
 fit(17,42,"< PREV",INK,8);
 capsule(192,32,55,28,SKY,TEAL);
 fit(201,42,"NEXT >",INK,8);
 snprintf(tmp,sizeof(tmp),"PAGE %d/3",page+1);
 fit(93,38,tmp,PURPLE,12);
 snprintf(tmp,sizeof(tmp),"%d / 9 FOUND",seen+((s->ending_mask&(1u<<END_EGG))?1:0));
 fit(96,52,tmp,TEAL,11);
 for(row=0;row<2;row++)for(col=0;col<2;col++){
  int saved,good;
  id=page*4+row*2+col+1;
  x=9+col*123;y=68+row*53;
  saved=(s->ending_mask&(1u<<id))!=0;good=id>=5;
  panel(x,y,114,47,saved?WHITE:GRAY,saved?(good?GREEN:GOLD):PURPLE);
  capsule(x+5,y+6,35,34,saved?(good?MINT:LEMON):SILVER,
          saved?(good?GREEN:GOLD):PURPLE);
  snprintf(tmp,sizeof(tmp),"%02d",id);
  label(x+10,y+14,tmp,PURPLE,2,2);
  fit(x+44,y+13,saved?names[id-1]:"???",INK,11);
  fit(x+44,y+30,saved?(good?"GOOD END":"BAD END"):"NOT FOUND",
      saved?(good?GREEN:ROSE):MUTED,11);
 }
 foot(PURPLE);
 fit(12,181,"B BACK   L/R CHANGE PAGE",WHITE,39);
}
/* No touchscreen activation on reset. The only destructive path remains
 * the deliberately difficult L+R+X hardware gesture after A review. */
static void reset_screen(int confirm){
 solid(0,27,256,149,BLUSH);
 panel(10,35,236,136,WHITE,ROSE);
 capsule(22,43,212,31,NIGHT,ROSE);
 label(57,51,"RESET SAVE?",WHITE,2,11);
 fit(31,86,"ERASES THIS BABY'S SAVE",ROSE,32);
 fit(31,99,"OTHER FILES ARE UNTOUCHED",PURPLE,34);
 solid(28,111,200,2,ROSE);
 capsule(25,120,206,27,confirm?PINK:LEMON,ROSE);
 label(confirm?53:66,126,confirm?"L+R THEN X":"A REVIEW",INK,2,12);
 fit(82,153,"B CANCEL",PURPLE,18);
 foot(PURPLE);
 fit(12,181,"SAVE ERASE REQUIRES CONFIRM",WHITE,39);
}
int ui_arcade_navigation(int x,int y){
 if(y<176||y>=192||x<0||x>=256)return 0;
 return x<80?1:x<174?2:3;
}
void ui_draw(const Game *s,const Arcade *a,int view,int selection,
             int ending,int reset_step,int can_save,const char *message){
 static const uint16_t accents[16]={
  SKY,PINK,MINT,LEMON,LILAC,AQUA,SKY,LEMON,
  MINT,TEAL,GOLD,PINK,LEMON,GREEN,SKY,AQUA
 };
 const char *screen_title;
 if(!pixels||!s)return;
 /* Full redraw intentionally invalidates the old virtual-cursor backing. */
 cursor_visible=0;
 if(view==2&&a&&a->id<16)screen_accent=accents[a->id];
 else if(view==0)screen_accent=AQUA;
 else if(view==1)screen_accent=LEMON;
 else if(view==3)screen_accent=ROSE;
 else if(view==4)screen_accent=game_is_good_ending(ending)?MINT:ROSE;
 else if(view==5)screen_accent=PINK;
 else if(view==6)screen_accent=SKY;
 else if(view==8||view==9)screen_accent=ROSE;
 else if(view==10)screen_accent=MINT;
 else if(view==11)screen_accent=AQUA;
 else screen_accent=TEAL;
 if(view==2&&a)screen_title="MINIGAME";
 else screen_title=view==0?"CARE":view==1?"ARCADE":
         view==3?"RESET":view==4?"ENDING":view==5?"WELCOME":
         view==6?"ALBUM":view==7?"TOUCH TEST":
         view==8?"TOYBOX":view==10?"ROOM":view==11?"TALK":"INCUBATOR";
 /* Every screen paints its own content band (y=27..175), and header/foot
  * repaint the other two bands. The previous blanket 256x192 clear was
  * 49,152 redundant 16-bit VRAM writes on every animation refresh. */
 header(screen_title,s,can_save);
 if(view==0||view==11){
  capsule(80,5,44,16,MINT,TEAL);
  fit(93,10,"CPU",INK,3);
 }
 switch(view){
 case 0:home(s,can_save,message);break;
 case 1:arcade_list(s,selection);break;
 case 2:if(a->id==0)bottle_challenge(a);
        else if(a->id==1)diaper_challenge(a);
        else if(a->id==2)teether_challenge(a);
        else if(a->id==3)kart_challenge(a);
        else if(a->id==4)beatbox_challenge(a);
        else if(a->id==5)sock_challenge(a);
        else if(a->id==6)nap_challenge(a);
        else if(a->id==7)rattle_challenge(a);
        else if(a->id==8)crawl_challenge(a);
        else if(a->id==9)stroller_challenge(a);
        else if(a->id==10)cereal_challenge(a);
        else if(a->id==11)blocks_challenge(a);
        else if(a->id==12)audit_challenge(a);
        else if(a->id==13)garden_challenge(a);
        else if(a->id==14)court_challenge(a);
        else if(a->id==15)ceo_challenge(a);
        else challenge(s,a);
        cabinet_chrome(a);
        break;
 case 3:reset_screen(reset_step);break;
 case 4:finale(s,ending);break;
 case 5:title_screen(s,can_save,message);break;
 case 6:ending_album(s,selection);break;
 case 7:touch_diagnostic();break;
 case 8:roughhouse_screen(selection);break;
 case 9:egg_scene_screen(selection);break;
 case 10:room_screen(s,selection,message);break;
 case 11:conversation_screen(selection,message);break;
 }
}
/* Three separated footer targets prevent accidental skip during the
 * buildup. Tapping the center readout is equivalent to A NEXT. */
int ui_egg_cinematic_touch(int x,int y){
 if(x<0||x>=256||y<0||y>=192)return 0;
 if(y>=176)return x<84?1:x<172?2:3;
 if(x>=15&&x<241&&y>=35&&y<165)return 3;
 return 0;
}
int ui_prank_entry_touch(int x,int y){
 return x>=93&&x<163&&y>=176&&y<192;
}
/* Tool selection is separate from activation (4). Never trigger physics
 * when a user only meant to switch to the other cartoon instrument. */
int ui_roughhouse_touch(int x,int y){
 if(x<0||x>=256||y<0||y>=192)return 0;
 if(y>=176)return 3;
 if(y>=145&&y<174){
  if(x>=9&&x<124)return ROUGHHOUSE_SLAP;
  if(x>=132&&x<247)return ROUGHHOUSE_KICK;
 }
 /* Only the illustrated doll is tappable; blank panel text is inert.
  * This also makes directional touch impulses intentional and repeatable. */
 if(y>=83&&y<135&&x>=95&&x<160)return 4;
 return 0;
}
/* Return 0..3 for the four actual prop cards, 4 for BACK, -1 for
 * whitespace. All hitboxes are visibly filled in room_screen(). */
int ui_room_touch(int x,int y){
 if(x<0||x>=256||y<0||y>=192)return -1;
 if(x>=198&&x<244&&y>=49&&y<63)return 7; /* SAVE */
 if(y>=176){
  /* Footer labels are ACTUAL touch buttons (not invisible instructions).
   * Center retains the historic BACK area to preserve existing tests. */
  if(x>=150&&x<208)return 5; /* SOUND, same as physical Y */
  if(x>=208&&x<248)return 6; /* USE highlighted prop */
  return 4;
 }
 if(y>=77&&y<115)return x>=8&&x<124?0:x>=132&&x<248?1:-1;
 if(y>=122&&y<161)return x>=8&&x<124?2:x>=132&&x<248?3:-1;
 return -1;
}
int ui_room_entry_touch(int x,int y){
 return x>=194&&x<243&&y>=34&&y<47;
}
int ui_mic_entry_touch(int x,int y){
 return x>=84&&x<123&&y>=33&&y<46;
}
int ui_conversation_entry_touch(int x,int y){
 return x>=8&&x<192&&y>=31&&y<66&&!ui_mic_entry_touch(x,y);
}
int ui_conversation_touch(int x,int y){
 int column,row;
 if(x<0||x>=W||y<0||y>=H)return -1;
 if(y>=176)return x<80?6:x>=176?7:-1;
 column=x>=8&&x<124?0:x>=132&&x<248?1:-1;
 if(column<0)return -1;
 for(row=0;row<3;row++){
  int top=92+row*27;
  if(y>=top&&y<top+24)return row*2+column;
 }
 return -1;
}
int ui_nursery_touch(int x,int y){
 if(x<7||x>=249)return -1;
 if(y>=108&&y<139)return x<128?FEED:CHAT;
 if(y>=143&&y<174)return x<128?PLAY:NAP;
 return -1;
}
int ui_home_navigation(int x,int y){
 if(x<0||x>=256||y<176||y>=192)return 0;
 if(x<89)return 1;
 if(x>=167)return 2;
 return 0; /* selected Toybox lives in the center of the dock */
}
int ui_album_page_touch(int x,int y){
 if(y<32||y>=61)return 0;
 if(x>=9&&x<64)return -1;
 if(x>=192&&x<247)return 1;
 return 0;
}
int ui_arcade_page_touch(int x,int y){
 if(y<32||y>=55)return 0;
 if(x>=8&&x<64)return -1;
 if(x>=193&&x<248)return 1;
 return 0;
}
int ui_arcade_select_page(int x,int y,int selection){
 int page,col,row;
 if(selection<0||selection>=16)return -1;
 if(y<73||y>=167)return -1;
 if(x>=9&&x<124)col=0;
 else if(x>=132&&x<247)col=1;
 else return -1;
 if(y<117)row=0;
 else if(y>=123&&y<167)row=1;
 else return -1;
 page=selection/4;
 return page*4+row*2+col;
}
int ui_arcade_touch(int x,int y){
 if(y<90||y>=163||x<66||x>=190)return -1;
 if(y<116)return 0;
 if(y>=143)return 2;
 if(x<125)return 3;
 return 1;
}

/* Diaper Dash buttons occupy different zones from generic arrow prompts.
 * Their displayed labels must correspond to the interactive hitboxes. */
int ui_diaper_touch(int x,int y){
 if(y>=88&&y<119&&x>=77&&x<181)return 0; /* TAPE / up */
 if(y>=119&&y<155){
  if(x>=8&&x<115)return 3;   /* WIPE / left */
  if(x>=142&&x<249)return 1; /* NEW / right */
 }
 if(y>=155&&y<175&&x>=77&&x<181)return 2; /* FOLD / down */
 return -1;
}

/* Three large lanes used by kart and matching-sock cabinets. */
static int cabinet_lane(int x){
 if(x>=13&&x<79)return 0;
 if(x>=95&&x<161)return 1;
 if(x>=177&&x<243)return 2;
 return -1;
}
int ui_kart_touch(int x,int y){
 return y>=91&&y<174?cabinet_lane(x):-1;
}
int ui_sock_touch(int x,int y){
 return y>=121&&y<175?cabinet_lane(x):-1;
}
int ui_nap_touch(int x,int y){
 return x>=12&&x<244&&y>=88&&y<175?0:-1;
}
/* Four large rhythmic pads, each mapping to one D-pad equivalent. */
int ui_crawl_touch(int x,int y){
 if(y<135||y>=174)return -1;
 if(x>=10&&x<120)return 0;
 if(x>=136&&x<247)return 2;
 return -1;
}
int ui_cereal_touch(int x,int y){
 return y>=119&&y<175?cabinet_lane(x):-1;
}
int ui_blocks_touch(int x,int y){
 return x>=12&&x<244&&y>=82&&y<175?0:-1;
}
int ui_audit_touch(int x,int y){
 if(y<137||y>=175)return -1;
 if(x>=11&&x<120)return 0;
 if(x>=136&&x<245)return 1;
 return -1;
}
int ui_garden_touch(int x,int y){
 return y>=105&&y<175?cabinet_lane(x):-1;
}
int ui_court_touch(int x,int y){
 int i;
 for(i=0;i<3;i++){
  int dx=x-court_node_x(i),dy=y-court_node_y(i);
  if(dx*dx+dy*dy<=20*20)return i;
 }
 return -1;
}
int ui_ceo_touch(int x,int y){
 return y>=121&&y<173?cabinet_lane(x):-1;
}
int ui_beatbox_touch(int x,int y){
 int i;
 if(y<123||y>=171)return -1;
 for(i=0;i<4;i++)if(x>=12+i*62&&x<60+i*62)return i;
 return -1;
}
/* Three generous touchscreen lanes; the golden catch band is the target. */
int ui_rattle_touch(int x,int y){
 if(x<13||x>=243||y<130||y>=172)return -1;
 if(x<79)return 0;
 if(x>=95&&x<161)return 1;
 if(x>=177&&x<243)return 2;
 return -1;
}
