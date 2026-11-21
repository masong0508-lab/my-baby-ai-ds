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
#define NIGHT RGB(8,9,18)
#define SILVER RGB(23,25,30)
#define ROSE RGB(30,13,22)
#define AQUA RGB(16,29,29)
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
/* Shared dimensional cards and buttons: no textures, allocations or
 * secondary buffer. All geometry is clipped by solid() to DS bitmap VRAM. */
static void panel(int x,int y,int width,int height,uint16_t fill,uint16_t border){
 solid(x+2,y+3,width,height,SHADOW);
 solid(x,y,width,height,fill);
 outline(x,y,width,height,border);
 if(width>=9&&height>=9){
  solid(x+2,y+2,width-4,2,WHITE);
  solid(x+2,y+4,2,height-7,WHITE);
  solid(x+3,y+height-3,width-6,1,border);
 }
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
 for(i=0;s[i]&&i<max&&x+i*6*size<W;i++)
  char_at(x+i*6*size,y,s[i],color,size);
}
static void fit(int x,int y,const char *s,uint16_t color,int max){
 label(x,y,s,color,1,max);
}
static void capsule(int x,int y,int width,int height,uint16_t fill,uint16_t edge){
 solid(x+3,y,width-6,height,fill);
 solid(x,y+3,width,height-6,fill);
 solid(x+3,y,width-6,1,edge);solid(x+3,y+height-1,width-6,1,edge);
 solid(x,y+3,1,height-6,edge);solid(x+width-1,y+3,1,height-6,edge);
 if(width>=15&&height>=13){
  solid(x+5,y+2,width-10,2,WHITE);
  solid(x+5,y+height-3,width-10,1,edge);
 }
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
static void insignia(int x,int y,uint16_t c){
 solid(x+3,y,3,3,c);
 solid(x,y+3,9,3,c);
 solid(x+3,y+6,3,3,c);
 solid(x+3,y+3,3,3,WHITE);
}
static void header(const char *title,const Game *s,int can_save){
 char tmp[32];int x;
 solid(0,0,W,27,NIGHT);
 solid(0,0,W,2,screen_accent);
 solid(0,3,3,21,screen_accent);
 solid(0,24,W,3,screen_accent);
 for(x=8;x<142;x+=24)solid(x,22,12,1,SILVER);
 insignia(7,9,screen_accent);
 label(20,9,title,WHITE,1,21);
 fit(122,10,can_save?"SD":"RAM",can_save?MINT:PINK,4);
 snprintf(tmp,sizeof(tmp),"DAY %lu",(unsigned long)s->days);
 capsule(192,4,57,18,LILAC,screen_accent);
 fit(196,10,tmp,INK,9);
 snprintf(tmp,sizeof(tmp),"$%u",s->dollars);
 fit(151,10,tmp,LEMON,6);
}
static void foot(uint16_t color){
 solid(0,176,256,16,NIGHT);
 solid(0,176,256,2,screen_accent);
 solid(0,178,256,1,SILVER);
 solid(0,179,3,13,screen_accent);
 solid(253,179,3,13,screen_accent);
 (void)color;
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
/* Nursery dashboard preserves all touch hitboxes while giving the four
 * actions a single readable visual hierarchy and real progress indicators. */
static void home(const Game *s,int can_save,const char *message){
 solid(0,27,W,149,BLUSH);
 solid(0,28,W,3,screen_accent);
 panel(6,32,244,55,WHITE,PURPLE);
 fit(13,40,"HUNGER",PURPLE,7);meter(63,39,s->hunger,ROSE);
 fit(139,40,"MOOD",PURPLE,4);meter(178,39,s->mood,GREEN);
 fit(13,57,"ENERGY",PURPLE,7);meter(63,56,s->energy,TEAL);
 fit(139,57,"CHAOS",PURPLE,5);meter(178,56,s->chaos,GOLD);
 fit(13,75,"GLITCH",PURPLE,6);
 solid(66,75,171,7,GRAY);
 solid(68,77,(s->corruption*167)/100,3,DEEP_PINK);
 solid(68,77,(s->corruption*167)/100,1,PINK);
 for(int i=1;i<5;i++)solid(68+i*33,75,1,7,WHITE);
 panel(6,89,244,27,IVORY,screen_accent);
 solid(12,94,4,17,screen_accent);
 fit(21,98,message,INK,36);
 capsule(7,121,117,23,SKY,BLUE);
 solid(14,141,100,1,BLUE);icon_bottle(9,119);label(43,128,"FEED",INK,1,11);
 capsule(132,121,117,23,LEMON,GOLD);
 solid(139,141,100,1,GOLD);icon_chat(133,119);label(168,128,"CHAT",INK,1,11);
 capsule(7,148,117,23,MINT,GREEN);
 solid(14,168,100,1,GREEN);icon_play(9,146);label(43,155,"PLAY",INK,1,11);
 capsule(132,148,117,23,PINK,DEEP_PINK);
 solid(139,168,100,1,DEEP_PINK);icon_nap(133,146);label(168,155,"NAP",INK,1,11);
 foot(PURPLE);
 fit(12,181,"START ARCADE",WHITE,19);
 fit(152,181,"END DAY",WHITE,14);
 (void)can_save;
}
static const char *const nick[16]={
 "BOTTLE","DIAPER","TAX","KART","BEATS","SOCKS","NAP","RATTLE",
 "CRAWL","DRIFT","CIPHER","BLOCKS","AUDIT","GARDEN","COURT","CEO"
};
/* The 4x4 cabinet bank intentionally keeps its original coordinates:
 * selecting, purchasing and returning still use the same touch targets. */
static void arcade_list(const Game *s,int selection){
 int row,col,i,x,y;
 char tmp[40];
 solid(0,27,256,149,LILAC);
 solid(0,28,256,3,screen_accent);
 for(i=0;i<4;i++)solid(0,66+i*25,4,17,screen_accent);
 panel(5,32,246,29,WHITE,PURPLE);
 label(12,38,"DIAPER DOLLAR ARCADE",PURPLE,1,27);
 snprintf(tmp,sizeof(tmp),"$%u   FAVOR %u",s->dollars,s->favors);
 fit(13,50,tmp,TEAL,28);
 for(row=0;row<4;row++)for(col=0;col<4;col++){
  uint16_t fill,border;
  int unlocked;
  i=row*4+col;x=7+col*62;y=65+row*25;
  unlocked=game_unlocked(s,i);
  fill=i==selection?LEMON:(unlocked?IVORY:GRAY);
  border=i==selection?DEEP_PINK:(unlocked?TEAL:PURPLE);
  capsule(x,y,57,22,fill,border);
  if(i==selection){
   outline(x+1,y+1,55,20,PURPLE);
   solid(x+2,y+7,3,8,DEEP_PINK);
  }else solid(x+2,y+7,2,8,unlocked?TEAL:SILVER);
  snprintf(tmp,sizeof(tmp),"%02d",i+1);
  fit(x+7,y+8,tmp,unlocked?PURPLE:MUTED,2);
  fit(x+23,y+8,nick[i],unlocked?INK:MUTED,5);
  if(!unlocked){
   solid(x+43,y+17,9,2,ROSE);
   solid(x+46,y+13,3,4,ROSE);
  }
 }
 panel(6,165,244,11,WHITE,screen_accent);
 snprintf(tmp,sizeof(tmp),"%s",game_names[selection]);
 fit(12,167,tmp,PURPLE,38);
 foot(PURPLE);
 fit(10,181,"A PLAY",WHITE,12);
 fit(96,181,"X BUY",WHITE,11);
 fit(184,181,"B BACK",WHITE,12);
}
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
static const char *const spr_baby[]={
 ".......hhhhhh.......",
 ".....hhsssssshh.....",
 "....hssssssssssh....",
 "...hssssssssssssh...",
 "..hssshhhhhhhssssh..",
 "..hsshssssssshsssh..",
 ".hssssssssssssssssh.",
 ".hssswwsssswwsssssh.",
 ".hsssokssssoksssssh.",
 ".hssssssssssssssssh.",
 ".hssrrssssssrrssssh.",
 ".hssssssppssssssssh.",
 ".hssssspwwpsssssssh.",
 ".hssssssppssssssssh.",
 "..hssssssssssssssh..",
 "...hssssssssssssh...",
 "....hhsssssssshh....",
 "......hhhhhhhh......"
};
static const char *const spr_bottle[]={
 "......oooo......",
 "......owwo......",
 ".....owwwwo.....",
 ".....oppppo.....",
 "....oooooooo....",
 "...owwwwwwwwo...",
 "...owwwwwwwwo...",
 "...owwbbbbwwo...",
 "...owwbbbbwwo...",
 "...owwbbbbwwo...",
 "...owwbbbbwwo...",
 "...owwbbbbwwo...",
 "...owwbbbbwwo...",
 "...owwwwwwwwo...",
 "...owwwppwwwo...",
 "...owwwppwwwo...",
 "....owwwwwwo....",
 ".....oooooo....."
};
static const char *const spr_rattle[]={
 "......oooo......",
 "....ooggggoo....",
 "...ogggwwgggo...",
 "..oggggwwggggo..",
 "..oggggwwggggo..",
 "..oggggggggggo..",
 "...oggggggggo...",
 "....ooggggoo....",
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
 ".owwwwwwwwwwwwwwo.",
 "owwwwwwwwwwwwwwwo",
 "owwppwwwwwwwwppwwo",
 "owwppwwwwwwwwppwwo",
 ".owwwbbbbbbwwwwo.",
 "..owbbbbbbbbwwo..",
 "...owbbbbbbwwo...",
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
 "...owwwwo.....",
 "...owwwwo.....",
 "...owwwwo.....",
 "...owwwwo.....",
 "...owwwwo.....",
 "...owwwwwwo....",
 "...owwwwwwwo..",
 "..owwwwwwwwwo.",
 "..owwwwwwwwwo.",
 "..owwwwwwwwwo.",
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
 "owgggwwwwggggwo",
 "owgwwwwwggwwgwo",
 "owgwwggwwwgggwo",
 "owwwggggwwwggwo",
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
static uint16_t art_ink(char p){
 switch(p){
 case 'o':return INK;
 case 'h':return RGB(11,7,6);
 case 's':return RGB(31,24,18);
 case 'r':return RGB(31,15,17);
 case 'p':return PINK;
 case 'w':return WHITE;
 case 'b':return SKY;
 case 'g':return GOLD;
 case 't':return TEAL;
 case 'k':return INK;
 default:return 0;
 }
}
static void sprite(int x,int y,int scale,const char *const *rows,int count){
 int yy,xx;
 for(yy=0;yy<count;yy++){
  const char *line=rows[yy];
  for(xx=0;line[xx];xx++){
   uint16_t color=art_ink(line[xx]);
   if(color)solid(x+xx*scale,y+yy*scale,scale,scale,color);
  }
 }
}
static void sparkle(int x,int y,uint16_t color){
 solid(x+3,y,2,9,color);solid(x,y+3,9,2,color);
 solid(x+2,y+2,4,4,WHITE);
}
/* Every cabinet inherits a unified theatrical frame. Previously each
 * redraw emitted ~140 dot primitives; this lightweight motif costs fewer
 * bitmap writes and gives every activity the same visual structure. */
static void arcade_wall(uint16_t background){
 int x,y;
 solid(0,27,256,149,background);
 solid(0,27,256,3,screen_accent);
 for(x=10;x<251;x+=32)solid(x,29,15,1,WHITE);
 for(y=35;y<170;y+=22){
  solid(2,y,3,13,screen_accent);
  solid(251,y,3,13,screen_accent);
 }
 solid(0,174,256,2,screen_accent);
}
static void cabinet_chrome(const Arcade *a){
 int i;
 int goal=a->id==4?3:a->id==1||a->id==6||a->id==13||
          a->id==14?4:a->id==9?5:6;
 solid(6,72,244,3,NIGHT);
 for(i=0;i<goal;i++)
  solid(11+i*11,73,8,2,i<a->round?LEMON:screen_accent);
 for(i=0;i<3;i++)
  solid(214+i*10,73,6,2,i<a->mistakes?ROSE:SILVER);
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
 int cursor=128+a->balance/2;
 arcade_wall(BLUSH);
 panel(7,31,242,41,WHITE,TEAL);
 fit(15,38,"BOTTLE BALANCE",PURPLE,30);
 fit(15,52,a->reverse?"GLITCH: LEFT IS RIGHT!":"STEADY HANDS, HAPPY BABY",a->reverse?DEEP_PINK:TEAL,37);
 bar(16,76,224,a->hold_frames,150,MINT);
 sprite(36,90,2,spr_baby,18);
 sprite(cursor+14,91,2,spr_bottle,18);
 sparkle(17,104,LEMON);sparkle(225,104,PINK);
 solid(25,132,206,3,TEAL);
 solid(92,129,72,9,MINT);
 solid(cursor-2,127,5,14,DEEP_PINK);
 snprintf(text,sizeof(text),"STEADY %u/150",a->hold_frames);
 fit(83,115,text,PURPLE,27);
 capsule(8,144,110,29,SKY,BLUE);
 capsule(138,144,110,29,LEMON,GOLD);
 label(29,153,"< LEFT",INK,1,11);
 label(163,153,"RIGHT >",INK,1,11);
 foot(PURPLE);
 snprintf(text,sizeof(text),"SPILLS %u/3   %u SEC",a->mistakes,(a->timer+59)/60);
 fit(19,181,text,WHITE,36);
}
static void teether_challenge(const Arcade *a){
 char text[40];
 arcade_wall(MINT);
 panel(6,31,244,41,WHITE,TEAL);
 fit(13,38,"TAX RETURN TEETHER",PURPLE,35);
 snprintf(text,sizeof(text),"SORTED %u/6   %u SEC",a->round,(a->timer+59)/60);
 fit(13,53,text,DEEP_PINK,36);
 /* Office-nursery scene: receipts come from the baby executive's desk. */
 solid(91,76,74,4,GOLD);
 solid(94,79,6,48,TEAL);
 solid(155,79,6,48,TEAL);
 sprite(107,83,2,spr_baby,18);
 capsule(94,121,68,24,SKY,BLUE);
 sprite(114,111,2,spr_receipt,13);
 if(a->touch_active&&a->touch_start_x>=99&&a->touch_start_x<160&&
    a->touch_start_y>=91&&a->touch_start_y<150)
  sprite(a->touch_last_x-14,a->touch_last_y-13,2,spr_receipt,13);
 capsule(10,128,86,43,a->target==3?LEMON:SKY,GOLD);
 capsule(160,128,86,43,a->target==1?PINK:SKY,DEEP_PINK);
 for(int i=0;i<4;i++){
  solid(18+i*18,135,8,3,INK);
  solid(171+i*16,137,6,3,PURPLE);
 }
 fit(26,150,"KEEP",INK,9);
 fit(174,150,"SHRED",INK,9);
 sparkle(11,96,LEMON);sparkle(236,100,PINK);
 fit(10,77,a->target==3?"REFUND: SAVE IT!":"JUNK BILL: SHRED!",PURPLE,35);
 foot(PURPLE);
 fit(9,181,"DRAG PAPER TO THE MATCHING BIN",WHITE,40);
}
static void kart_challenge(const Arcade *a){
 char text[40];
 int i,y;
 arcade_wall(SKY);
 panel(6,31,244,42,WHITE,TEAL);
 fit(12,38,"CRIB KART",PURPLE,34);
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
 fit(12,38,"SOCK MARKET",PURPLE,35);
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
 fit(13,37,"NAPTIME NEGOTIATOR",PURPLE,35);
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
 fit(29,133,a->taped?"LOVELY!":"SHHH... FIND THE GREEN",WHITE,39);
 foot(PURPLE);
 snprintf(text,sizeof(text),"TAP IN GREEN! %u SEC  B BACK",(a->timer+59)/60);
 fit(12,181,text,WHITE,39);
}
static void beatbox_challenge(const Arcade *a){
 static const char *const pads[4]={"BOOM","CLAP","BEEP","POP"};
 static const uint16_t colors[4]={MINT,PINK,LEMON,SKY};
 char text[40];
 int active=-1;
 arcade_wall(LILAC);
 panel(6,31,244,42,WHITE,PURPLE);
 fit(12,38,"BABY BEATBOX",PURPLE,37);
 snprintf(text,sizeof(text),"TUNE %u/3  MISS %u/3",a->round+1,a->mistakes);
 fit(12,54,text,DEEP_PINK,36);
 if(!a->phase&&a->hold_frames){
  int index=(int)(a->elapsed*3u/a->hold_frames);
  if(index>2)index=2;
  active=a->notes[index];
 }
 capsule(69,79,118,20,a->phase?MINT:LEMON,TEAL);
 fit(83,85,a->phase?"YOUR TURN: REPEAT!":"WATCH THE PATTERN",INK,29);
 sprite(111,101,1,spr_baby,18);
 for(int i=0;i<3;i++)
  capsule(143+i*18,103,13,11,a->phase?(i<a->stage?GREEN:GRAY):
          (i==(int)(a->elapsed*3u/(a->hold_frames?a->hold_frames:1u))?GOLD:GRAY),INK);
 for(int i=0;i<4;i++){
  int x=12+i*62;
  capsule(x,123,48,47,active==i?WHITE:colors[i],active==i?GOLD:PURPLE);
  solid(x+17,131,14,14,active==i?LEMON:WHITE);
  outline(x+16,130,16,16,PURPLE);
  fit(x+7,152,pads[i],INK,6);
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
 fit(14,37,"DIAPER DASH",PURPLE,26);
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
 fit(13,37,"RATTLE RESCUE",PURPLE,30);
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
 fit(13,38,"CORPORATE CRAWL",PURPLE,35);
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
 fit(26,153,"JUMP UP",INK,15);
 fit(154,153,"DUCK DOWN",INK,15);
 foot(PURPLE);
 fit(10,181,a->target==0?"JUMP OVER THE CRATE!":"DUCK UNDER THE SCANNER!",WHITE,40);
}
static void stroller_challenge(const Arcade *a){
 char text[40];int x=128+a->balance;
 arcade_wall(MINT);
 panel(6,31,244,41,WHITE,TEAL);
 fit(13,38,"STROLLER DRIFT",PURPLE,35);
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
 fit(12,38,"CEREAL CIPHER",PURPLE,35);
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
 fit(12,38,"TANTRUM BLOCKS",PURPLE,36);
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
 fit(12,38,"BABY'S FIRST AUDIT",PURPLE,35);
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
 fit(25,152,"APPROVE",INK,12);
 fit(166,152,"FLAG",INK,9);
 sparkle(12,92,LEMON);sparkle(241,110,PINK);
 foot(PURPLE);
 fit(10,181,"MATCHING TOTALS? APPROVE OR FLAG",WHITE,40);
}
static void garden_challenge(const Arcade *a){
 char text[40];int i;
 arcade_wall(SKY);
 panel(6,31,244,42,WHITE,GREEN);
 fit(13,38,"GLITCH GARDENING",PURPLE,34);
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
 char text[40];int i,j;
 arcade_wall(LEMON);
 panel(6,31,244,42,WHITE,GOLD);
 fit(12,38,"COURTROOM CRAYONS",PURPLE,35);
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
  char num[3];
  capsule(x-19,y-18,38,37,i<a->stage?MINT:PINK,DEEP_PINK);
  snprintf(num,sizeof(num),"%d",i+1);
  fit(x-2,y-3,num,INK,1);
 }
 sprite(14,82,2,spr_gavel,12);
 foot(PURPLE);
 fit(9,181,"TRACE 1 TO 2 TO 3   (L UP R)",WHITE,40);
}
static void ceo_challenge(const Arcade *a){
 static const char *const actions[3]={"INVEST","PLAY","NAP"};
 char text[40];int i;
 arcade_wall(BLUSH);
 panel(6,31,244,41,WHITE,PURPLE);
 fit(12,38,"CEO OF DAYCARE",PURPLE,35);
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
  solid(x+24,130,18,15,i==0?GOLD:i==1?LEMON:WHITE);
  fit(x+12,153,actions[i],INK,10);
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
static void finale(const Game *s,int ending){
 int good=game_is_good_ending(ending),i;char tmp[40];
 uint16_t edge=good?GREEN:ROSE;
 solid(0,27,256,149,good?MINT:LILAC);
 for(i=0;i<18;i++){
  int x=(i*59+7)%250,y=32+(i*43)%135;
  solid(x,y,3,6,(i&1)?LEMON:PINK);
  solid(x-2,y+2,7,2,(i&1)?GOLD:WHITE);
 }
 panel(12,35,232,136,WHITE,edge);
 capsule(29,41,198,24,good?MINT:PINK,edge);
 fit(good?77:82,49,good?"GOOD ENDING":"BAD ENDING",INK,20);
 capsule(81,69,94,47,good?SKY:BLUSH,edge);
 sprite(108,76,2,spr_baby,18);
 insignia(40,78,good?GOLD:ROSE);
 insignia(207,78,good?GOLD:ROSE);
 fit(22,119,game_ending_name(ending),PURPLE,36);
 solid(24,132,208,1,good?GREEN:DEEP_PINK);
 fit(27,137,good?"+$8 AND +1 FAVOR!":"THE BABY GETS ANOTHER DAY",good?GREEN:ROSE,37);
 snprintf(tmp,sizeof(tmp),"%d / 4 GOOD ENDINGS RECORDED",
          game_good_ending_count(s));
 fit(41,153,tmp,MUTED,32);
 foot(PURPLE);
 fit(26,181,"A / B  CONTINUE THE STORY",WHITE,36);
}
/* Redraw diagnostic panel once on entry, then update only tiny regions.
 * Repainting 256x192 VRAM every stylus-held frame was tearing/flickering
 * on DraStic. No extra framebuffer or VRAM is needed.
 */
static int diag_last_raw=-1,diag_last_x=-1,diag_last_y=-1;
static int diag_cross_x=-1,diag_cross_y=-1;
static unsigned long diag_last_count=~0ul;
static void touch_diagnostic(void){
 solid(0,27,256,149,SKY);
 solid(0,27,256,3,screen_accent);
 panel(7,32,242,143,WHITE,PURPLE);
 capsule(14,39,228,21,NIGHT,screen_accent);
 fit(43,46,"STYLUS SIGNAL TEST",WHITE,32);
 insignia(23,45,screen_accent);
 fit(17,65,"KEY_TOUCH:",PURPLE,14);
 fit(17,79,"X/Y:",PURPLE,5);
 fit(17,93,"TAPS:",PURPLE,5);
 /* Preserve this uniform BLUSH test canvas: partial crosshair updates
  * erase only 15x15 tiles and must restore exact underlying pixels. */
 solid(18,108,220,52,BLUSH);
 outline(18,108,220,52,TEAL);
 solid(20,110,216,2,WHITE);
 fit(27,164,"B / START / SELECT: BACK",MUTED,37);
 foot(PURPLE);
 fit(10,181,"LIVE STYLUS: TOUCH THE CANVAS",WHITE,39);
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
static void title_screen(const Game *s,int storage_ok,const char *status){
 int i;char count[40];
 solid(0,27,256,149,LILAC);
 for(i=0;i<15;i++){
  int x=(i*53+17)%247,y=30+(i*41)%132;
  solid(x,y,4,4,(i&1)?LEMON:SKY);
  solid(x+1,y-2,2,8,(i&1)?WHITE:PINK);
 }
 panel(11,34,234,137,WHITE,DEEP_PINK);
 capsule(22,39,212,29,NIGHT,PINK);
 insignia(36,49,LEMON);
 label(65,49,"MY BABY A.I.!",WHITE,1,21);
 capsule(90,70,76,49,LEMON,GOLD);
 sprite(108,76,2,spr_baby,18);
 insignia(181,77,PINK);
 snprintf(count,sizeof(count),"DAY %lu",(unsigned long)s->days);
 fit(25,83,count,PURPLE,10);
 snprintf(count,sizeof(count),"$%u",s->dollars);
 fit(25,97,count,TEAL,9);
 fit(171,107,storage_ok?"SAVE READY":"RAM MODE",TEAL,13);
 fit(45,121,"SMALL BABY. BIG CHAOS.",PURPLE,31);
 capsule(48,133,160,27,MINT,TEAL);
 insignia(59,141,GREEN);
 label(94,143,"START",INK,1,10);
 fit(22,162,status,DEEP_PINK,36);
 foot(PURPLE);
 fit(9,181,"A START   X SAVE   SELECT TOUCH",WHITE,40);
}
static void ending_album(const Game *s){
 static const char *const names[8]={
  "BOTTLE TAX","BABY CEO","REBOOT","INTERN STRIKE",
  "GRADUATION","ARCADE CHAMP","GLITCH TAMER","SECOND CHANCE"
 };
 int row,col,id,x,y;char tmp[40];
 solid(0,27,256,149,BLUSH);
 solid(0,28,256,3,screen_accent);
 panel(6,32,244,27,WHITE,PURPLE);
 fit(15,40,"ENDING SCRAPBOOK",PURPLE,24);
 snprintf(tmp,sizeof(tmp),"%d/4 GOOD",game_good_ending_count(s));
 fit(185,47,tmp,TEAL,11);
 for(row=0;row<4;row++)for(col=0;col<2;col++){
  int saved,good;
  id=1+row+col*4;x=8+col*124;y=64+row*24;
  saved=(s->ending_mask&(1u<<id))!=0;good=id>=5;
  capsule(x,y,116,21,saved?(good?MINT:LEMON):GRAY,
          saved?(good?GREEN:GOLD):PURPLE);
  solid(x+3,y+5,2,10,saved?(good?GREEN:ROSE):SILVER);
  snprintf(tmp,sizeof(tmp),"%02d",id);
  fit(x+9,y+5,tmp,PURPLE,2);
  fit(x+27,y+4,saved?names[id-1]:"???",INK,14);
  fit(x+27,y+13,saved?(good?"GOOD MEMORY":"BAD MEMORY"):
      "NOT DISCOVERED",saved?TEAL:MUTED,14);
 }
 foot(PURPLE);
 fit(11,181,"B BACK   YOUR SAVES ARE SAFE",WHITE,39);
}
static void reset_screen(int confirm){
 int i;
 solid(0,27,256,149,LILAC);
 for(i=0;i<7;i++){
  solid(7+i*36,31,17,3,ROSE);
  solid(7+i*36,168,17,3,ROSE);
 }
 panel(10,35,236,136,WHITE,ROSE);
 capsule(22,44,212,28,NIGHT,ROSE);
 insignia(37,54,LEMON);
 fit(57,54,"FACTORY RESET?",WHITE,27);
 fit(28,83,"THIS ERASES MY BABY A.I. SAVE",ROSE,37);
 fit(28,96,"NOT ANY OTHER SD CARD FILES",PURPLE,37);
 solid(27,110,202,1,ROSE);
 capsule(26,116,204,25,confirm?PINK:LEMON,ROSE);
 fit(confirm?35:75,125,confirm?"HOLD L+R, THEN PRESS X":"A  REVIEW RESET",
     INK,38);
 fit(70,149,"B  CANCEL / GO BACK",PURPLE,31);
 foot(PURPLE);
 fit(35,181,"TWO-STEP CONFIRMATION ONLY",WHITE,36);
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
 char cabinet_title[24];
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
 else screen_accent=TEAL;
 if(view==2&&a){
  snprintf(cabinet_title,sizeof(cabinet_title),"CABINET %02u/16",
           (unsigned)a->id+1u);
  screen_title=cabinet_title;
 }else screen_title=view==0?"MY BABY A.I.!":view==1?"BABY ARCADE":
         view==3?"SAVE MEMORY":view==4?"ENDING!":view==5?"WELCOME":
         view==6?"MEMORY ALBUM":"TOUCH TEST";
 /* Every screen paints its own content band (y=27..175), and header/foot
  * repaint the other two bands. The previous blanket 256x192 clear was
  * 49,152 redundant 16-bit VRAM writes on every animation refresh. */
 header(screen_title,s,can_save);
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
