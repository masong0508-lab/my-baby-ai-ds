/* Independent BlocksDS touchscreen/ARM7 diagnostic for DraStic.
 *
 * This has NO baby-game logic, file access, 3D rendering, keyboard widget,
 * texture buffers or custom touch gate. Uses the documented libnds API:
 * swiWaitForVBlank(), scanKeys(), keysHeld(), touchRead().
 *
 * Screen 1: LEFT indicator goes GREEN for KEY_TOUCH; RIGHT goes YELLOW
 * for KEY_A. Status/counter/coordinates are visible at all times.
 * Screen 2: tap anywhere on its plain lilac background to move crosshair.
 * Static bitmap drawn once, then only small changed rectangles are updated.
 */
#include <nds.h>
#include <stdint.h>
#include <stdio.h>
#define RGB(r,g,b) ((uint16_t)(0x8000u | (r) | ((g)<<5) | ((b)<<10)))
#define NAVY RGB(6,7,15)
#define WHITE RGB(31,31,31)
#define RED RGB(29,7,11)
#define GREEN RGB(9,27,14)
#define YELLOW RGB(31,28,6)
#define GRAY RGB(13,14,21)
#define CYAN RGB(12,26,29)
#define LILAC RGB(24,21,31)
#define INK RGB(5,5,12)
static volatile uint16_t *top,*bottom;
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
 [':'-32]={0,4,4,0,4,4,0},
 ['/'-32]={1,1,2,4,8,16,16},
 ['-'-32]={0,0,0,31,0,0,0},
 ['.'-32]={0,0,0,0,0,6,6},
 ['!'-32]={4,4,4,4,4,0,4},
};
static void rect(volatile uint16_t *v,int x,int y,int w,int h,uint16_t col){
 int i,j,xe=x+w,ye=y+h;
 if(x<0)x=0;if(y<0)y=0;
 if(xe>256)xe=256;if(ye>192)ye=192;
 if(x>=xe||y>=ye)return;
 for(j=y;j<ye;j++)for(i=x;i<xe;i++)v[j*256+i]=col;
}
static void letter(volatile uint16_t *v,int x,int y,char ch,uint16_t col,int size){
 int a,b,i,j;
 unsigned c=(unsigned char)ch;
 if(c<32||c>=128)return;
 for(j=0;j<7;j++)for(i=0;i<5;i++){
  if((glyph[c-32][j]&(1u<<(4-i)))==0)continue;
  for(b=0;b<size;b++)for(a=0;a<size;a++)
   rect(v,x+i*size+a,y+j*size+b,1,1,col);
 }
}
static void label(volatile uint16_t *v,int x,int y,
                  const char *text,uint16_t col,int size){
 for(int i=0;text[i]&&x+i*6*size+5*size<=256;i++)
  letter(v,x+i*6*size,y,text[i],col,size);
}
static void indicator(int x,int touch,int is_touch){
 uint16_t col=is_touch?(touch?GREEN:RED):(touch?YELLOW:GRAY);
 rect(top,x,40,111,65,col);
 label(top,x+14,50,is_touch?"TOUCH":"A KEY",WHITE,2);
 label(top,x+20,84,touch?"ON":"OFF",WHITE,2);
}
static void draw_static(void){
 rect(top,0,0,256,192,NAVY);
 rect(top,0,0,256,27,CYAN);
 label(top,21,8,"INPUT TEST ROM",INK,1);
 label(top,12,113,"TOUCH BOTTOM SCREEN",WHITE,1);
 label(top,12,128,"IF TOUCH ON = GREEN",WHITE,1);
 label(top,12,143,"IF A KEY = YELLOW",WHITE,1);
 label(top,12,171,"ALIVE",CYAN,1);
 rect(bottom,0,0,256,192,LILAC);
 rect(bottom,12,10,232,25,CYAN);
 label(bottom,43,19,"TAP ANYWHERE",INK,1);
 label(bottom,23,44,"THIS IS THE DS TOUCH SCREEN",INK,1);
 label(bottom,28,162,"CROSSHAIR = STYLUS DATA",INK,1);
 label(bottom,37,179,"NO GAME CODE",INK,1);
}
static void coord_readout(int x,int y,unsigned taps){
 char line[44];
 rect(top,10,155,190,12,NAVY);
 snprintf(line,sizeof(line),"X %03d Y %03d TAPS %u",x,y,taps);
 label(top,11,157,line,WHITE,1);
}
static void cross(int x,int y,int on){
 if(x<7||y<57||x>248||y>155)return;
 rect(bottom,x-6,y-6,13,13,LILAC);
 if(on){
  rect(bottom,x-1,y-6,3,13,RED);
  rect(bottom,x-6,y-1,13,3,RED);
 }
}
int main(void){
 int touch_last=-1,a_last=-1,x_last=-1,y_last=-1;
 int cross_x=-1,cross_y=-1,taps=0,was_touch=0;
 unsigned frame=0;
 lcdMainOnTop();
 videoSetMode(MODE_5_2D);
 videoSetModeSub(MODE_5_2D);
 vramSetBankA(VRAM_A_MAIN_BG);
 vramSetBankC(VRAM_C_SUB_BG);
 {
  int a=bgInit(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
  int b=bgInitSub(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
  top=(volatile uint16_t *)bgGetGfxPtr(a);
  bottom=(volatile uint16_t *)bgGetGfxPtr(b);
 }
 draw_static();
 coord_readout(0,0,0);
 while(1){
  touchPosition point={0};
  int pressed,a_pressed,xy_changed=0;
  swiWaitForVBlank();
  scanKeys();
  uint32_t held=keysHeld();
  pressed=(held&KEY_TOUCH)!=0;
  a_pressed=(held&KEY_A)!=0;
  if(pressed){
   touchRead(&point);
   if(!was_touch)taps++;
  }
  if(pressed!=touch_last){indicator(8,pressed,1);touch_last=pressed;}
  if(a_pressed!=a_last){indicator(137,a_pressed,0);a_last=a_pressed;}
  if(pressed){
   if(x_last!=(int)point.px||y_last!=(int)point.py){
    x_last=point.px;y_last=point.py;xy_changed=1;
   }
   if(cross_x>=0)cross(cross_x,cross_y,0);
   cross_x=point.px;cross_y=point.py;
   cross(cross_x,cross_y,1);
  }else if(cross_x>=0){
   cross(cross_x,cross_y,0);
   cross_x=-1;cross_y=-1;
  }
  if(xy_changed||(was_touch!=pressed))coord_readout(x_last,y_last,(unsigned)taps);
  if(frame%45u==0u)rect(top,57,170,7,8,(frame/45u&1u)?WHITE:GREEN);
  was_touch=pressed;
  frame++;
 }
}
