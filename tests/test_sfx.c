#include <assert.h>
#include <stdio.h>
#include "sfx.h"
/* Mock the PSG backend, proving no note can run forever. */
static int calls,starts,stops;
static unsigned max_hz,max_volume,last_volume;
void sfx_backend_init(void){calls=starts=stops=0;}
void sfx_backend_apply(unsigned hz,unsigned vol){
 calls++;
 if(!hz||!vol){stops++;last_volume=0;return;}
 assert(hz>=100&&hz<=2000);
 assert(vol<=63);
 starts++;last_volume=vol;
 if(hz>max_hz)max_hz=hz;
 if(vol>max_volume)max_volume=vol;
}
static void finish(SfxCue cue,int upper){
 int i;
 sfx_play(cue);
 assert(sfx_is_playing());
 for(i=0;i<upper;i++)sfx_tick();
 assert(!sfx_is_playing());
 assert(last_volume==0);
}
int main(void){
 int n;
 sfx_init();
 assert(sfx_is_enabled());
 finish(SFX_SLAP,14);
 finish(SFX_KICK,18);
 finish(SFX_BOUNCE,12);
 finish(SFX_CRACK,24);
 finish(SFX_UNSEAL,54);
 finish(SFX_PULSE,19);
 finish(SFX_SCARE,45);
 /* The scare begins with a clear high alarm, then drops to bass instead
  * of an indistinguishable sustained beep. Audio never exceeds PSG limits. */
 sfx_play(SFX_SCARE);
 sfx_tick();
 assert(max_hz>=1200);
 for(int i=0;i<11;i++)sfx_tick();
 assert(sfx_is_playing());
 sfx_stop();
 sfx_play(SFX_SCARE);
 sfx_play(SFX_BOUNCE); /* must not interrupt the reveal */
 assert(sfx_is_playing());
 sfx_set_enabled(0);
 assert(!sfx_is_playing()&&!sfx_is_enabled());
 n=calls;
 sfx_play(SFX_KICK);
 for(int i=0;i<18;i++)sfx_tick();
 assert(calls==n); /* mute does zero hardware operations */
 sfx_set_enabled(1);
 finish(SFX_KICK,18);
 assert(max_hz>700&&max_volume<=63&&starts>20&&stops>=8);
 printf("PASS: bounded one-channel PSG envelopes, silence, cue priority and mute (%d commands)\n",calls);
 return 0;
}
