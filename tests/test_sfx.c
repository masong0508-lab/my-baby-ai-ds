#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "sfx.h"
/* Mock the PSG backend, proving no note can run forever. */
static int calls,starts,stops;
static unsigned max_hz,max_volume,last_volume,last_hz,note_changes;
void sfx_backend_init(void){calls=starts=stops=0;}
void sfx_backend_apply(unsigned hz,unsigned vol){
 calls++;
 if(!hz||!vol){stops++;last_volume=0;last_hz=0;return;}
 if(last_hz&&hz!=last_hz)note_changes++;
 last_hz=hz;
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
static uint32_t cue_signature(SfxCue cue){
 uint32_t hash=2166136261u;
 sfx_stop();sfx_play(cue);
 for(unsigned i=0;i<64u;i++){
  sfx_tick();
  hash^=last_hz;hash*=16777619u;
  hash^=last_volume;hash*=16777619u;
 }
 assert(!sfx_is_playing()&&last_hz==0&&last_volume==0);
 return hash;
}
static void cannot_interrupt(SfxCue primary,SfxCue lower){
 unsigned hz[64],volume[64],playing[64];
 sfx_stop();sfx_play(primary);
 for(unsigned i=0;i<64u;i++){
  sfx_tick();hz[i]=last_hz;volume[i]=last_volume;
  playing[i]=(unsigned)sfx_is_playing();
 }
 sfx_play(primary);
 for(unsigned i=0;i<64u;i++){
  if(i==1u)sfx_play(lower);
  sfx_tick();
  assert(last_hz==hz[i]&&last_volume==volume[i]);
  assert((unsigned)sfx_is_playing()==playing[i]);
 }
}
static void arcade_cues(void){
 static const SfxCue cues[]={SFX_ARCADE_PROGRESS,SFX_ARCADE_MISS,
                            SFX_ARCADE_WIN,SFX_ARCADE_LOSS};
 static const SfxCue horror[]={SFX_CRACK,SFX_UNSEAL,SFX_PULSE,SFX_SCARE};
 uint32_t signatures[4];
 unsigned i,j;
 for(i=0;i<4u;i++){
  finish(cues[i],31);
  signatures[i]=cue_signature(cues[i]);
  for(j=0;j<i;j++)assert(signatures[i]!=signatures[j]);
  for(j=0;j<4u;j++)cannot_interrupt(horror[j],cues[i]);
 }
 for(i=0;i<2u;i++)for(j=2;j<4u;j++)cannot_interrupt(cues[j],cues[i]);
 /* A new result or intentional care can supersede the old melody. */
 sfx_play(SFX_ARCADE_WIN);sfx_tick();
 sfx_play(SFX_ARCADE_LOSS);sfx_tick();assert(last_hz==392u);
 sfx_play(SFX_FEED);sfx_tick();assert(last_hz==392u&&last_volume==20u);
 sfx_play(SFX_ARCADE_WIN);sfx_tick();
 sfx_play(SFX_SCARE);sfx_tick();assert(last_hz==1220u);
 sfx_set_enabled(0);
 int before=calls;
 for(i=0;i<4u;i++){
  sfx_play(cues[i]);
  for(j=0;j<31u;j++)sfx_tick();
 }
 assert(calls==before&&!sfx_is_playing());
 sfx_set_enabled(1);
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
 /* Each care action has its own short original, finite musical identity.
  * All share the same PSG channel and no sample bank or extra RAM. */
 finish(SFX_FEED,24);
 finish(SFX_CHAT,54);
 finish(SFX_PLAY,28);
 finish(SFX_NAP,40);
 finish(SFX_ROOM_SWITCH,16);
 finish(SFX_MOBILE,36);
 finish(SFX_LULLABY,54);
 assert(cue_signature(SFX_LULLABY)!=cue_signature(SFX_NAP));
 cannot_interrupt(SFX_SCARE,SFX_LULLABY);
 cannot_interrupt(SFX_UNSEAL,SFX_LULLABY);
 cannot_interrupt(SFX_PULSE,SFX_LULLABY);
 {
  uint32_t signatures[4];
  for(unsigned i=0;i<4u;i++){
   SfxCue cue=(SfxCue)(SFX_BEAT_BOOM+i);
   finish(cue,16);
   signatures[i]=cue_signature(cue);
   for(unsigned j=0;j<i;j++)assert(signatures[i]!=signatures[j]);
   cannot_interrupt(SFX_SCARE,cue);
   cannot_interrupt(SFX_ARCADE_WIN,cue);
  }
 }
 assert(note_changes>=15);
 /* Audio CHAT is synchronized to the rig's three 12-tick mouth pulses:
  * age 0..7 anticipation is silent, 8..43 contains the babble, and
  * age 44 clears the PSG voice. */
 {
  int first;
  /* A PLAY/FEED note must stop BEFORE CHAT's intentional silent prelude.
   * Otherwise a stale PSG oscillator overlaps the baby's closed mouth. */
  sfx_play(SFX_FEED);
  sfx_tick();
  assert(last_hz==392u);
  sfx_play(SFX_CHAT);
  assert(last_hz==0u&&last_volume==0u);
  first=starts;
  for(int i=0;i<=8;i++)sfx_tick();
  assert(starts==first);
  for(int i=9;i<44;i++)sfx_tick();
  assert(starts>first&&sfx_is_playing());
  sfx_tick();
  assert(!sfx_is_playing()&&last_volume==0);
 } /* melodies, not flat one-note beeps */
 /* The scare begins with a clear high alarm, then drops to bass instead
  * of an indistinguishable sustained beep. Audio never exceeds PSG limits. */
 sfx_play(SFX_SCARE);
 sfx_tick();
 assert(max_hz>=1200);
 /* Ordinary nursery notes must NEVER replace the jump-scare attack. */
 sfx_play(SFX_CHAT);
 sfx_tick();
 assert(last_hz==1120u); /* second-frame scare, not chat frequency */
 for(int i=0;i<11;i++)sfx_tick();
 assert(sfx_is_playing());
 sfx_stop();
 sfx_play(SFX_UNSEAL);
 sfx_tick();
 sfx_play(SFX_MOBILE);
 sfx_tick();
 assert(last_hz==142u); /* no chime interrupts the shell opening */
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
 arcade_cues();
 assert(max_hz>700&&max_volume<=63&&starts>20&&stops>=8);
 printf("PASS: bounded one-channel PSG envelopes, silence, cue priority and mute (%d commands)\n",calls);
 return 0;
}
