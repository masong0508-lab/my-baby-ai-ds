/* Independent minimal 2D dual-screen startup probe.
 * No baby code, no FAT, no GL, no sound; determines whether the runtime
 * starts at all in DraStic before debugging game-specific graphics.
 */
#include <nds.h>
int main(void){
 videoSetMode(MODE_5_2D);
 videoSetModeSub(MODE_5_2D);
 vramSetBankA(VRAM_A_MAIN_BG);
 vramSetBankC(VRAM_C_SUB_BG);
 {
  int mainbg=bgInit(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
  int subbg=bgInitSub(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
  volatile uint16_t *p=(volatile uint16_t*)bgGetGfxPtr(mainbg);
  volatile uint16_t *q=(volatile uint16_t*)bgGetGfxPtr(subbg);
  int x,y;
  for(y=0;y<192;y++)for(x=0;x<256;x++){
   p[y*256+x]=(uint16_t)(0x8000|((x>>3)&31)|(((y>>3)&31)<<5));
   q[y*256+x]=(uint16_t)(0x8000|31|(((x>>3)&31)<<10));
  }
 }
 while(1){swiWaitForVBlank();scanKeys();}
}
