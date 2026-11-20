/* My Baby A.I.! original touchscreen graphics.
 * 256x192 16-bit bitmap rendered directly into dedicated sub-screen VRAM.
 * No heap allocation, texture imports, or offscreen framebuffer.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"
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
 test_touch_x=x;
 test_touch_y=y;
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
 [':'-32]={0,4,4,0,4,4,0},
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
  for(a=x;a<x2;a++)row[a]=color;
 }
}
static void outline(int x,int y,int width,int height,uint16_t color){
 solid(x,y,width,1,color);solid(x,y+height-1,width,1,color);
 solid(x,y,1,height,color);solid(x+width-1,y,1,height,color);
}
static void panel(int x,int y,int width,int height,uint16_t fill,uint16_t border){
 solid(x+2,y+3,width,height,SHADOW);
 solid(x,y,width,height,fill);
 outline(x,y,width,height,border);
 solid(x+1,y+1,width-2,2,WHITE);
}
static void char_at(int x,int y,char value,uint16_t color,int scale){
 int i,j,dx,dy;unsigned ch=(unsigned char)value;
 const uint8_t *bits;
 if(ch>='a'&&ch<='z')ch-=32;
 if(ch<32||ch>=128)return;
 bits=glyph[ch-32];
 for(j=0;j<7;j++)for(i=0;i<5;i++)
  if(bits[j]&(1u<<(4-i)))
   for(dy=0;dy<scale;dy++)for(dx=0;dx<scale;dx++)
    solid(x+i*scale+dx,y+j*scale+dy,1,1,color);
}
static void label(int x,int y,const char *s,uint16_t color,int size,int max){
 int i;
 if(!s)return;
 for(i=0;s[i]&&i<max;i++)char_at(x+i*6*size,y,s[i],color,size);
}
static void fit(int x,int y,const char *s,uint16_t color,int max){
 label(x,y,s,color,1,max);
}
static void capsule(int x,int y,int width,int height,uint16_t fill,uint16_t edge){
 solid(x+3,y,width-6,height,fill);
 solid(x,y+3,width,height-6,fill);
 solid(x+3,y,width-6,1,edge);solid(x+3,y+height-1,width-6,1,edge);
 solid(x,y+3,1,height-6,edge);solid(x+width-1,y+3,1,height-6,edge);
}
static void meter(int x,int y,int value,uint16_t fill){
 int v=value;
 if(v<0)v=0;
 if(v>100)v=100;
 solid(x,y,71,8,WHITE);outline(x,y,71,8,INK);
 solid(x+2,y+2,(v*67)/100,4,fill);
}
static void header(const char *title,const Game *s){
 char tmp[32];int x;
 solid(0,0,W,27,PURPLE);
 solid(0,24,W,3,DEEP_PINK);
 for(x=0;x<256;x+=18)solid(x+4,22,9,2,LEMON);
 label(9,7,title,WHITE,1,24);
 snprintf(tmp,sizeof(tmp),"DAY %lu",(unsigned long)s->days);
 capsule(191,5,58,17,PINK,WHITE);
 label(196,10,tmp,INK,1,9);
 snprintf(tmp,sizeof(tmp),"$%u",s->dollars);
 fit(153,10,tmp,LEMON,6);
}
static void foot(uint16_t color){
 solid(0,176,256,16,color);
 for(int x=0;x<256;x+=16)solid(x,176,8,1,WHITE);
}
static void icon_bottle(int x,int y){
 solid(x+8,y+3,9,5,BLUE);solid(x+10,y,5,3,WHITE);
 solid(x+5,y+8,15,16,WHITE);outline(x+5,y+8,15,16,TEAL);
 solid(x+7,y+14,11,7,SKY);solid(x+10,y+9,4,3,PINK);
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
 solid(x+4,y+9,22,11,LILAC);solid(x+7,y+6,13,4,WHITE);
 solid(x+6,y+18,22,3,DEEP_PINK);
 label(x+11,y+1,"Z",PURPLE,1,1);
}
static void home(const Game *s,int can_save,const char *message){
 char line[48];
 solid(0,27,W,149,BLUSH);
 solid(0,28,W,2,PINK);
 panel(6,34,244,52,WHITE,LILAC);
 fit(13,41,"HUNGER",PURPLE,7);meter(63,40,s->hunger,DEEP_PINK);
 fit(139,41,"MOOD",PURPLE,4);meter(178,40,s->mood,GREEN);
 fit(13,58,"ENERGY",PURPLE,7);meter(63,57,s->energy,TEAL);
 fit(139,58,"CHAOS",PURPLE,5);meter(178,57,s->chaos,GOLD);
 fit(13,75,"GLITCH",PURPLE,6);
 solid(66,75,169,5,LILAC);solid(66,75,s->corruption*169/100,5,DEEP_PINK);
 panel(6,90,244,25,IVORY,PINK);
 solid(12,96,5,13,DEEP_PINK);
 label(21,98,message,INK,1,36);
 capsule(7,121,117,23,SKY,BLUE);icon_bottle(9,119);label(43,128,"FEED",INK,1,11);
 capsule(132,121,117,23,LEMON,GOLD);icon_chat(133,119);label(168,128,"CHAT",INK,1,11);
 capsule(7,148,117,23,MINT,GREEN);icon_play(9,146);label(43,155,"PLAY",INK,1,11);
 capsule(132,148,117,23,PINK,DEEP_PINK);icon_nap(133,146);label(168,155,"NAP",INK,1,11);
 foot(PURPLE);
 fit(12,181,"ARCADE",WHITE,14);
 fit(149,181,"END DAY",WHITE,14);
 (void)line;
 (void)can_save;
}
static const char *const nick[16]={
 "BOTTLE","DIAPER","TAX","KART","BEATS","SOCKS","NAP","RATTLE",
 "CRAWL","DRIFT","CIPHER","BLOCKS","AUDIT","GARDEN","COURT","CEO"
};
static void arcade_list(const Game *s,int selection){
 int row,col,i,x,y;
 char tmp[40];
 solid(0,27,256,149,BLUSH);
 panel(5,32,246,29,WHITE,LILAC);
 label(12,39,"DIAPER DOLLAR ARCADE",PURPLE,1,27);
 snprintf(tmp,sizeof(tmp),"$%u  FAV %u",s->dollars,s->favors);
 fit(13,51,tmp,DEEP_PINK,26);
 for(row=0;row<4;row++)for(col=0;col<4;col++){
  i=row*4+col;x=7+col*62;y=65+row*25;
  capsule(x,y,57,22,i==selection?LEMON:(game_unlocked(s,i)?MINT:GRAY),i==selection?DEEP_PINK:PURPLE);
  snprintf(tmp,sizeof(tmp),"%02d",i+1);
  label(x+4,y+4,tmp,INK,1,2);
  fit(x+22,y+4,nick[i],INK,5);
  if(!game_unlocked(s,i))solid(x+44,y+15,7,2,DEEP_PINK);
 }
 panel(6,165,244,11,WHITE,PURPLE);
 snprintf(tmp,sizeof(tmp),"%s",game_names[selection]);
 fit(12,167,tmp,INK,38);
 foot(PURPLE);
 fit(10,181,"PLAY",WHITE,8);
 fit(98,181,"BUY",WHITE,8);
 fit(191,181,"BACK",WHITE,8);
}
static void arrow(int x,int y,int dir,uint16_t fill){
 int i;
 /* Purposefully chunky hardware-era arrows, not Unicode glyphs. */
 if(dir==0){for(i=0;i<8;i++)solid(x-i,y+i,2*i+1,2,fill);solid(x-3,y+14,7,10,fill);}
 if(dir==2){solid(x-3,y,7,10,fill);for(i=0;i<8;i++)solid(x-7+i,y+9+i,15-2*i,2,fill);}
 if(dir==1){for(i=0;i<8;i++)solid(x+i,y-i,2,2*i+1,fill);solid(x-12,y-3,17,7,fill);}
 if(dir==3){for(i=0;i<8;i++)solid(x-i,y-i,2,2*i+1,fill);solid(x-5,y-3,17,7,fill);}
}
/* The first two cabinets have genuinely different native-resolution UIs. */
static void bottle_challenge(const Arcade *a){
 char text[40];int cursor=128+a->balance/2;
 solid(0,27,256,149,BLUSH);
 panel(7,31,242,45,WHITE,TEAL);
 fit(16,39,"BOTTLE BALANCE",PURPLE,32);
 fit(16,55,a->reverse?"GLITCH! CONTROLS FLIPPED":"KEEP BOTTLE UPRIGHT",a->reverse?DEEP_PINK:TEAL,35);
 solid(26,89,204,17,WHITE);outline(26,89,204,17,INK);
 solid(94,91,68,13,MINT);
 solid(cursor-2,86,5,24,DEEP_PINK);
 icon_bottle(cursor-15,107);
 snprintf(text,sizeof(text),"STABLE %u / 150",a->hold_frames);
 fit(58,117,text,INK,26);
 capsule(8,138,110,32,SKY,BLUE);
 capsule(138,138,110,32,LEMON,GOLD);
 label(33,149,"< LEFT",INK,1,11);
 label(162,149,"RIGHT >",INK,1,11);
 foot(PURPLE);
 snprintf(text,sizeof(text),"SPILLS %u/3  TIME %u",a->mistakes,a->timer/60);
 fit(16,181,text,WHITE,36);
}
static void diaper_challenge(const Arcade *a){
 static const char *const steps[4]={"WIPE","NEW DIAPER","FOLD","TAPE"};
 char text[40];int target=game_arcade_expected(a);
 solid(0,27,256,149,BLUSH);
 panel(6,31,244,51,WHITE,PINK);
 fit(15,39,"DIAPER DASH",PURPLE,28);
 snprintf(text,sizeof(text),"STEP %u/4 : %s",a->stage+1,steps[a->stage<4?a->stage:3]);
 fit(15,53,text,INK,36);
 fit(15,68,a->reverse?"CORRUPTION: OPPOSITE!":"TAP THE CORRECT TASK",a->reverse?DEEP_PINK:TEAL,36);
 capsule(77,88,103,29,target==0?LEMON:GRAY,GOLD);
 fit(108,99,"TAPE",INK,10);
 capsule(8,119,106,36,target==3?LEMON:SKY,TEAL);
 fit(40,132,"WIPE",INK,10);
 capsule(142,119,106,36,target==1?LEMON:SKY,TEAL);
 fit(180,132,"NEW",INK,8);
 capsule(77,155,103,19,target==2?LEMON:MINT,GREEN);
 fit(115,161,"FOLD",INK,8);
 foot(PURPLE);
 snprintf(text,sizeof(text),"MISTAKES %u/3   %u F",a->mistakes,a->timer);
 fit(15,181,text,WHITE,37);
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
static void finale(const Game *s,int ending){
 int good=game_is_good_ending(ending);char tmp[40];int i;
 solid(0,27,256,149,good?MINT:LILAC);
 for(i=0;i<15;i++)solid((i*53)%255,30+(i*29)%136,3,6,(i&1)?LEMON:PINK);
 panel(12,43,232,124,WHITE,good?GREEN:DEEP_PINK);
 capsule(39,49,178,21,good?MINT:PINK,good?GREEN:DEEP_PINK);
 label(69,55,good?"GOOD ENDING!":"BAD ENDING!",INK,1,18);
 for(i=0;i<5;i++){
  solid(45+i*7,83-i*2,4,4,LEMON);
  solid(203-i*7,83-i*2,4,4,LEMON);
 }
 fit(24,97,game_ending_name(ending),INK,36);
 fit(27,122,good?"+$8 AND +1 FAVOR!":"THE BABY GETS ANOTHER DAY.",PURPLE,36);
 snprintf(tmp,sizeof(tmp),"FOUND %d GOOD ENDINGS",game_good_ending_count(s));
 fit(65,139,tmp,MUTED,29);
 foot(PURPLE);
 fit(28,181,"A / B CONTINUE PLAYING",WHITE,30);
}
/* Redraw diagnostic panel once on entry, then update only tiny regions.
 * Repainting 256x192 VRAM every stylus-held frame was tearing/flickering
 * on DraStic. No extra framebuffer or VRAM is needed.
 */
static int diag_last_raw=-1,diag_last_x=-1,diag_last_y=-1;
static int diag_cross_x=-1,diag_cross_y=-1;
static unsigned long diag_last_count=~0ul;
static void touch_diagnostic(void){
 solid(0,27,256,149,BLUSH);
 panel(7,32,242,143,WHITE,PURPLE);
 capsule(14,39,228,21,LILAC,PURPLE);
 fit(27,46,"STYLUS SIGNAL TEST",INK,32);
 fit(17,65,"KEY_TOUCH:",PURPLE,14);
 fit(17,79,"X/Y:",PURPLE,5);
 fit(17,93,"TAPS:",PURPLE,5);
 solid(18,108,220,52,BLUSH);
 outline(18,108,220,52,TEAL);
 fit(27,164,"B / START / SELECT: BACK",MUTED,37);
 foot(PURPLE);
 fit(10,181,"TAP LOWER SCREEN; WATCH SIGNAL",WHITE,39);
 diag_last_raw=-1;diag_last_x=-1;diag_last_y=-1;
 diag_cross_x=-1;diag_cross_y=-1;
 diag_last_count=~0ul;
}
void ui_touch_diag_update(void){
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
 /* Only erase/draw a 13x13 patch within the little touch canvas. */
 if(!test_touch_down||test_touch_x<0||test_touch_y<0||
    test_touch_x>255||test_touch_y>191)return;
 x=31+(test_touch_x*194)/255;
 y=119+(test_touch_y*30)/191;
 if(x==diag_cross_x&&y==diag_cross_y)return;
 if(diag_cross_x>=0)
  solid(diag_cross_x-7,diag_cross_y-7,15,15,BLUSH);
 solid(x-1,y-6,3,13,DEEP_PINK);
 solid(x-6,y-1,13,3,DEEP_PINK);
 diag_cross_x=x;diag_cross_y=y;
}
static void title_screen(const Game *s,int storage_ok,const char *status){
 int i;char count[40];
 solid(0,27,256,149,PINK);
 /* Arcade-era promotional title screen drawn by the DS itself. */
 for(i=0;i<18;i++){
  int x=(i*59)%247,y=29+(i*47)%135;
  solid(x,y,5,5,(i&1)?LEMON:SKY);
 }
 panel(15,42,226,128,WHITE,DEEP_PINK);
 capsule(28,52,200,37,LILAC,PURPLE);
 label(50,63,"MY BABY A.I.!",INK,1,21);
 capsule(52,103,152,26,MINT,TEAL);
 label(81,112,"START GAME",INK,1,20);
 snprintf(count,sizeof(count),"DAY %lu  $%u",(unsigned long)s->days,s->dollars);
 fit(75,134,count,MUTED,21);
 fit(42,148,storage_ok?"SAVE STORAGE ACTIVE":"X: TRY SAVE STORAGE",TEAL,32);
 fit(33,160,status,DEEP_PINK,36);
 foot(PURPLE);fit(10,181,"START PLAY  SELECT TOUCH TEST",WHITE,39);
}
static void ending_album(const Game *s){
 int row,col,id,x,y;char tmp[40];
 solid(0,27,256,149,BLUSH);
 panel(6,32,244,27,WHITE,PURPLE);
 fit(15,42,"ENDING MEMORY ALBUM",PURPLE,31);
 for(row=0;row<4;row++)for(col=0;col<2;col++){
  id=1+row+col*4;x=8+col*124;y=64+row*24;
  capsule(x,y,116,21,(s->ending_mask&(1u<<id))?(id>=5?MINT:LEMON):GRAY,PURPLE);
  snprintf(tmp,sizeof(tmp),"%02d",id);
  fit(x+5,y+6,tmp,INK,3);
  fit(x+25,y+6,(s->ending_mask&(1u<<id))?game_ending_name(id):"NOT FOUND",INK,14);
 }
 foot(PURPLE);
 fit(11,181,"B: BACK  SAVE KEPT",WHITE,32);
}
static void reset_screen(int confirm){
 solid(0,27,256,149,LILAC);
 panel(11,37,234,132,IVORY,DEEP_PINK);
 capsule(23,48,210,25,PINK,DEEP_PINK);
 label(48,56,"FACTORY RESET?",INK,1,24);
 fit(27,88,"DELETES THIS GAME SAVE",DEEP_PINK,35);
 fit(34,105,"NO OTHER SD FILES",INK,32);
 fit(24,129,confirm?"HOLD L+R AND PRESS X":"A: REVIEW RESET",PURPLE,38);
 fit(40,147,"B: CANCEL",INK,26);
 foot(PURPLE);fit(38,181,"ONLY IF YOU CHOOSE",WHITE,30);
}
int ui_arcade_navigation(int x,int y){
 if(y<176||y>=192||x<0||x>=256)return 0;
 return x<80?1:x<174?2:3;
}
void ui_draw(const Game *s,const Arcade *a,int view,int selection,
             int ending,int reset_step,int can_save,const char *message){
 if(!pixels||!s)return;
 /* Full UI render overwrites prior cursor pixels; don't restore old
  * background from a different screen into the newly drawn screen. */
 cursor_visible=0;
 solid(0,0,256,192,IVORY);
 header(view==0?"MY BABY A.I.!":view==1?"BABY ARCADE":view==2?"MINIGAME":view==3?"SAVE MEMORY":view==4?"ENDING!":view==5?"WELCOME":"MEMORY ALBUM",s);
 switch(view){
 case 0:home(s,can_save,message);break;
 case 1:arcade_list(s,selection);break;
 case 2:if(a->id==0)bottle_challenge(a);
        else if(a->id==1)diaper_challenge(a);
        else challenge(s,a);
        break;
 case 3:reset_screen(reset_step);break;
 case 4:finale(s,ending);break;
 case 5:title_screen(s,can_save,message);break;
 case 6:ending_album(s);break;
 case 7:touch_diagnostic();break;
 }
}
int ui_nursery_touch(int x,int y){
 if(x<7||x>=249)return -1;
 if(y>=121&&y<145)return x<127?FEED:CHAT;
 if(y>=148&&y<172)return x<127?PLAY:NAP;
 return -1;
}
int ui_home_navigation(int x,int y){
 if(y<176||y>=192)return 0;
 return x<128?1:2;
}
int ui_arcade_select(int x,int y){
 if(x<7||x>=253||y<65||y>=165)return -1;
 int col=(x-7)/62,row=(y-65)/25;
 if(col>3||row>3)return -1;
 return row*4+col;
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
