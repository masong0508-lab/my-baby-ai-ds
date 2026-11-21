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
 if(!enabled||cue<=SFX_NONE||cue>SFX_MOBILE)return;
 /* Bedtime jingles and care chirps cannot silence the dramatic reveal or
  * cut through a shell-crack/unseal cue. Horror retains absolute priority. */
 if(cue>=SFX_FEED&&(current==SFX_SCARE||
                    current==SFX_CRACK||current==SFX_UNSEAL))return;
 /* Rebound is low-priority: it must never cut short the main contact,
  * shell opening, dramatic pulse, or reveal. */
 if(cue==SFX_BOUNCE&&current!=SFX_NONE)return;
 /* A newly selected CHAT begins with eight silent anticipation ticks.
  * Cut the PREVIOUS audible note immediately, rather than letting its
  * PSG oscillator drone beneath the baby's silent opening gesture. */
 if(last_hz||last_volume)sfx_backend_apply(0,0);
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
 case SFX_FEED:
  /* Warm three-note bottle jingle: a tiny 392->494->587 rise. */
  duration=16;
  hz=age<5u?392u:age<10u?494u:587u;
  volume=age<11u?20u:(16u-age)*4u;
  break;
 case SFX_CHAT:{
  unsigned syllable,pulse,mouth;
  /* Match the existing baby mouth animation EXACTLY:
   * first syllable at 8..19, second 20..31, third 32..43.
   * A quiet 8-tick anticipation gives the infant time to lean forward. */
  duration=44;
  if(age<8u){hz=0;volume=0;break;}
  syllable=(age-8u)/12u;
  pulse=(age-8u)%12u;
  mouth=pulse<=6u?pulse:12u-pulse;
  hz=(syllable==0u?325u:syllable==1u?405u:350u)+
     (pulse/2u)*7u;
  volume=mouth*4u;
  break;
 }
 case SFX_PLAY:
  /* Three bright toy notes followed by a quick resolving tonic. */
  duration=20;
  hz=age<5u?440u:age<10u?554u:age<15u?659u:523u;
  volume=age<13u?24u:(20u-age)*3u;
  break;
 case SFX_NAP:
  /* Original quiet descending lullaby; never a sudden alarm. */
  duration=30;
  hz=age<8u?392u:age<16u?330u:age<23u?262u:196u;
  volume=age<17u?14u:(30u-age);
  break;
 case SFX_ROOM_SWITCH:
  /* Small two-note click confirms window/lamp state changes. */
  duration=8;
  hz=age<4u?740u:554u;
  volume=(8u-age)*2u;
  break;
 case SFX_MOBILE:
  /* Gentle wind-up mobile triad; a single finite cue on CRIB taps. */
  duration=27;
  hz=age<9u?587u:age<18u?740u:659u;
  volume=age<17u?16u:(27u-age)*2u;
  break;
 case SFX_SCARE:
  /* Distinct attack + low distorted synthetic growl in one PSG voice:
   * 3 ticks of sharp alarm, an 8-tick plunge, then a short decaying
   * subharmonic tail. No PCM, second sound channel, or repeating strobe. */
  duration=38;
  if(age<3u){
   hz=1220u-age*100u;volume=60u-age*2u;
  }else if(age<11u){
   hz=410u-(age-3u)*25u;volume=49u-(age-3u)*2u;
  }else{
   hz=155u+(age%3u)*13u-(age-11u)*2u;
   volume=(38u-age)*2u;
  }
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
