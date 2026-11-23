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
/* A controlled phoneme-pattern fixture. These are quantized feature
 * traces, not a claim of accuracy on unrecorded human speech. */
static const uint8_t voice_shape_a[VOICE_BINS][VOICE_FEATURES]={
 {50,18,10,12},{50,18,10,12},{56,21,13,14},{65,24,16,17},
 {84,75,32,32},{88,75,34,32},{80,69,30,30},{60,51,20,20},
 {45,33,10,14},{42,18,9,12},{38,15,8,10},{38,15,8,10}
};
static const uint8_t voice_shape_b[VOICE_BINS][VOICE_FEATURES]={
 {50,24,15,18},{57,24,15,18},{67,57,18,21},{74,60,26,24},
 {76,60,28,25},{60,42,22,20},{50,30,19,19},{83,84,35,33},
 {88,87,35,35},{65,63,22,25},{45,30,15,15},{37,21,10,10}
};
static void shaped_capture(VoiceCapture *c,int alternate,
                           const uint8_t positions[VOICE_BINS]){
 const uint8_t (*source)[VOICE_FEATURES]=alternate?
                                    voice_shape_b:voice_shape_a;
 unsigned i;
 voice_capture_reset(c);
 c->state=VOICE_READY;c->frames=VOICE_BINS;
 for(i=0u;i<VOICE_BINS;i++){
  unsigned at=positions?positions[i]:i;
  assert(at<VOICE_BINS);
  memcpy(c->measurements[i],source[at],VOICE_FEATURES);
 }
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
 /* Natural speaking rates change different syllables by different
  * amounts. Constrained 12-bin time warping should align either version
  * while preserving the clear margin against a distinct trained phrase. */
 {
  static const uint8_t quicker[VOICE_BINS]={0,1,3,4,4,5,6,7,7,9,10,11};
  static const uint8_t slower[VOICE_BINS]={0,1,1,2,3,4,5,6,7,8,10,11};
  VoiceProfile p2,invalid;
  voice_profile_reset(&p2);
  shaped_capture(&c,0,0);assert(voice_train(&p2,"COME HERE",&c)==1);
  shaped_capture(&c,1,0);assert(voice_train(&p2,"GO AWAY",&c)==1);
  shaped_capture(&c,0,quicker);
  assert(voice_recognize(&p2,&c,&score)==0&&score<=8u);
  shaped_capture(&c,0,slower);
  assert(voice_recognize(&p2,&c,&score)==0&&score<=8u);
  shaped_capture(&c,1,0);
  assert(voice_recognize(&p2,&c,&score)==1&&score<=8u);
  /* A wildly different acoustic shape is neither known label. */
  voice_capture_reset(&c);c.state=VOICE_READY;c.frames=VOICE_BINS;
  for(unsigned t=0;t<VOICE_BINS;t++){
   c.measurements[t][0]=40u;c.measurements[t][1]=252u;
   c.measurements[t][2]=200u;c.measurements[t][3]=200u;
  }
  assert(voice_recognize(&p2,&c,&score)==-1&&score>55u);
  /* A wrong-but-checksummed 0..63 feature profile is rejected on load. */
  invalid=p2;invalid.entries[0].models[0][0][0]=64u;
  assert(!voice_profile_valid(&invalid));
  /* Different labels taught with identical sounds are ambiguous.
   * Explicit USE must remain unavailable instead of guessing. */
  voice_profile_reset(&p2);
  shaped_capture(&c,0,0);
  assert(voice_train(&p2,"MUM",&c)==1);
  assert(voice_train(&p2,"MOM",&c)==1);
  assert(voice_recognize(&p2,&c,&score)==-1&&score==0u);
 }
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
 puts("PASS: voice capture, 24 labels, timed syllable warping, near-word ambiguity, noise rejection and bounds");
 return 0;
}
