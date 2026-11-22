#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "voice.h"
static const int16_t wave[16]={0,12000,23500,31000,30000,21000,9000,-3000,
 -14000,-23500,-30000,-27000,-17000,-7000,2000,7500};
static void chunk(VoiceCapture *c,int period,int amplitude,unsigned frame){
 int16_t pcm[VOICE_FRAME_SAMPLES];unsigned i;
 for(i=0;i<VOICE_FRAME_SAMPLES;i++){
  int index=(int)(i+frame*17u)/period;
  int16_t x=wave[index&15];
  pcm[i]=(int16_t)(x*amplitude/100);
 }
 voice_capture_pcm16(c,pcm,VOICE_FRAME_SAMPLES);
}
static void record(VoiceCapture *c,int period,int amplitude,int mod){
 unsigned i;voice_capture_reset(c);
 for(i=0;i<4;i++)chunk(c,1,0,i);
 for(i=0;i<18;i++)chunk(c,period,amplitude+((i+mod)%4)*2,i);
 for(i=0;i<4;i++)chunk(c,1,0,i);
 assert(c->state==VOICE_READY);
}
/* Test a large fixed microphone ADC baseline, not merely zero PCM.
 * The recognizer must still hear the voice's deviations from that bias. */
static void record_biased(VoiceCapture *c,int period,int amplitude,int mod,int bias){
 unsigned n,i;int16_t pcm[VOICE_FRAME_SAMPLES];
 voice_capture_reset(c);
 for(n=0;n<26u;n++){
  unsigned frame=n<4u?n:n<22u?n-4u:n-22u;
  for(i=0;i<VOICE_FRAME_SAMPLES;i++){
   int index=(int)(i+frame*17u)/period;
   int sample=n<4u||n>=22u?bias:
      wave[index&15]*(amplitude+(int)((frame+(unsigned)mod)%4u)*2)/100+bias;
   assert(sample>=INT16_MIN&&sample<=INT16_MAX);
   pcm[i]=(int16_t)sample;
  }
  voice_capture_pcm16(c,pcm,VOICE_FRAME_SAMPLES);
 }
 assert(c->state==VOICE_READY);
}
int main(void){
 VoiceProfile p;VoiceCapture c;unsigned score;int match;
 voice_profile_reset(&p);assert(voice_profile_valid(&p));
 record(&c,2,75,0);assert(voice_recognize(&p,&c,&score)<0);
 assert(voice_train(&p,"hello",&c)==1);
 record(&c,2,78,1);assert(voice_train(&p,"hello",&c)==2);
 record(&c,4,70,0);assert(voice_train(&p,"WORLD",&c)==1);
 record(&c,4,78,1);assert(voice_train(&p,"WORLD",&c)==2);
 record(&c,2,78,2);match=voice_recognize(&p,&c,&score);
 assert(match==0&&score<55u);
 record(&c,4,75,2);match=voice_recognize(&p,&c,&score);
 assert(match==1&&score<55u);
 record_biased(&c,2,75,1,6000);
 match=voice_recognize(&p,&c,&score);
 assert(match==0&&score<55u);
 for(int n=0;n<22;n++){
  char label[25];snprintf(label,sizeof(label),"PHRASE %d",n);
  record(&c,n+5,65,n);assert(voice_train(&p,label,&c)==1);
 }
 assert(p.used==24&&voice_profile_valid(&p));
 assert(strcmp(voice_label(&p,0),"HELLO")==0);
 record(&c,22,70,1);assert(voice_train(&p,"NEW COMMAND",&c)==1);
 assert(p.used==24&&strcmp(voice_label(&p,0),"NEW COMMAND")==0);
 assert(!voice_train(&p,"\nINVALID",&c));
 assert(!voice_train(&p,"abcdefghijklmnopqrstuvwxyz",&c));
 assert(voice_profile_valid(&p));
 {VoiceProfile q=p;q.entries[0].samples=3;assert(!voice_profile_valid(&q));}
 voice_capture_reset(&c);
 for(unsigned n=0;n<VOICE_MAX_WAIT;n++)chunk(&c,1,0,n);
 assert(c.state==VOICE_NO_SPEECH);
 /* A steady +/-20,000 ADC bias with no voice is not speech. It must not
  * fill the meter, generate a learned pattern or trigger a false word. */
 for(int bias=-20000;bias<=20000;bias+=40000){
  int16_t pcm[VOICE_FRAME_SAMPLES];
  for(unsigned k=0;k<VOICE_FRAME_SAMPLES;k++)pcm[k]=(int16_t)bias;
  voice_capture_reset(&c);
  for(unsigned n=0;n<VOICE_MAX_WAIT;n++)
   voice_capture_pcm16(&c,pcm,VOICE_FRAME_SAMPLES);
  assert(c.state==VOICE_NO_SPEECH&&c.frames==0u&&c.level==0u);
  assert(voice_recognize(&p,&c,&score)<0);
 }
 puts("PASS: offline mic frontend, 24 taught phrases, two-example matching, noise rejection and bounds");
 return 0;
}
