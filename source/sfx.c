#include "sfx.h"
#include <stdint.h>
/* Single hardware PSG channel. A new cue supersedes a shorter cue; the
 * jump-scare overrides everything. No timers, DMA, sample arrays or heap. */
static uint8_t current,age,enabled=1;
static unsigned last_hz,last_volume;
void sfx_init(void){
 current=0;age=0;last_hz=0;last_volume=0;enabled=1;
 sfx_backend_init();
 sfx_backend_apply(0,0);
}
void sfx_stop(void){
 current=0;age=0;last_hz=0;last_volume=0;
 sfx_backend_apply(0,0);
}
void sfx_set_enabled(int on){
 enabled=(uint8_t)(on!=0);
 if(!enabled){
  sfx_stop();
 }
}
int sfx_is_enabled(void){return enabled!=0;}
int sfx_is_playing(void){return current!=0;}
void sfx_play(SfxCue cue){
 if(!enabled||cue<=SFX_NONE||cue>SFX_SCARE)return;
 /* Rebound is low-priority: it must never cut short the main contact,
  * shell opening, dramatic pulse, or reveal. */
 if(cue==SFX_BOUNCE&&current!=SFX_NONE)return;
 current=(uint8_t)cue;age=0;last_hz=0;last_volume=0;
}
void sfx_tick(void){
 unsigned hz=0,volume=0,duration=0;
 if(current==SFX_NONE)return;
 switch(current){
 case SFX_SLAP:
  duration=8;hz=440u-age*41u;volume=(8u-age)*3u;
  break;
 case SFX_KICK:
  duration=12;hz=190u+age*14u;volume=(12u-age)*3u;
  break;
 case SFX_BOUNCE:
  duration=5;hz=300u+age*75u;volume=(5u-age)*2u;
  break;
 case SFX_CRACK:
  duration=16;hz=710u-age*26u;volume=8u+(16u-age)*2u;
  break;
 case SFX_UNSEAL:
  duration=45;hz=142u+(age/3u)*5u;volume=age<8u?age*2u:
     age<34u?16u:(45u-age);
  break;
 case SFX_PULSE:
  duration=12;hz=110u+age*3u;volume=age<4u?age*6u:(12u-age)*3u;
  break;
 case SFX_SCARE:
  duration=33;hz=880u-age*19u;volume=age<5u?28u+age*5u:
     age<23u?48u:(33u-age)*4u;
  break;
 default:duration=0;break;
 }
 if(age>=duration||duration==0){
  current=0;last_hz=0;last_volume=0;
  sfx_backend_apply(0,0);
  return;
 }
 if(hz!=last_hz||volume!=last_volume){
  sfx_backend_apply(hz,volume);
  last_hz=hz;last_volume=volume;
 }
 age++;
}
