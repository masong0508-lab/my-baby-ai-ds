#include <nds.h>
#include "sfx.h"
/* Both devkitPro libnds and BlocksDS support ARM9 soundPlayPSG. Using one
 * channel avoids Maxmod, WAV samples, memory banks and custom ARM7 audio.
 * soundKill explicitly stops each self-oscillating PSG note. */
static int voice=-1;
void sfx_backend_init(void){
 soundEnable();
 voice=-1;
}
void sfx_backend_apply(unsigned hz,unsigned volume){
 if(!hz||!volume){
  if(voice>=0)soundKill(voice);
  voice=-1;
  return;
 }
 if(volume>63u)volume=63u;
 if(hz<100u)hz=100u;
 if(hz>2000u)hz=2000u;
 if(voice<0)
  voice=soundPlayPSG(DutyCycle_25,(u16)hz,(u8)volume,64);
 else{
  soundSetFreq(voice,(u16)hz);
  soundSetVolume(voice,(u8)volume);
 }
}
