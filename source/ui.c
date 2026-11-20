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
 if(x<0)x=0;if(y<0)y=0;if(x2>W)x2=W;if(y2>H)y2=H;
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
 if(v<0)v=0;if(v>100)v=100;
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
 snprintf(line,sizeof(line),"$%u  FAV %u  %s",s->dollars,s->favors,can_save?"SAVED":"RAM");
 fit(6,181,line,WHITE,28);
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
 fit(8,181,"A PLAY  X BUY  B BACK",WHITE,27);
}
static void arrow(int x,int y,int dir,uint16_t fill){
 int i;
 /* Purposefully chunky hardware-era arrows, not Unicode glyphs. */
 if(dir==0){for(i=0;i<8;i++)solid(x-i,y+i,2*i+1,2,fill);solid(x-3,y+14,7,10,fill);}
 if(dir==2){solid(x-3,y,7,10,fill);for(i=0;i<8;i++)solid(x-7+i,y+9+i,15-2*i,2,fill);}
 if(dir==1){for(i=0;i<8;i++)solid(x+i,y-i,2,2*i+1,fill);solid(x-12,y-3,17,7,fill);}
 if(dir==3){for(i=0;i<8;i++)solid(x-i,y-i,2,2*i+1,fill);solid(x-5,y-3,17,7,fill);}
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
void ui_draw(const Game *s,const Arcade *a,int view,int selection,
             int ending,int reset_step,int can_save,const char *message){
 if(!pixels||!s)return;
 solid(0,0,256,192,IVORY);
 header(view==0?"MY BABY A.I.!":view==1?"BABY ARCADE":view==2?"MINIGAME":view==3?"SAVE MEMORY": "ENDING ALBUM",s);
 switch(view){
 case 0:home(s,can_save,message);break;
 case 1:arcade_list(s,selection);break;
 case 2:challenge(s,a);break;
 case 3:reset_screen(reset_step);break;
 case 4:finale(s,ending);break;
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
