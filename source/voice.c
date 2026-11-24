#include "voice.h"
#include <string.h>
_Static_assert(sizeof(VoiceProfile)<3500u,"offline voice profile too large");
_Static_assert(sizeof(VoiceCapture)<256u,"speech capture must be bounded");
static unsigned abs_i(int a){return a<0?(unsigned)-a:(unsigned)a;}
static uint8_t cap63(unsigned a){return (uint8_t)(a>63u?63u:a);}
static int label_normalize(char out[VOICE_TEXT+1u],const char *src){
 unsigned n=0,seen=0;int space=0;
 if(!src)return 0;
 while(*src){
  unsigned c=(unsigned char)*src++;
  if(c==' '||c=='\t'){if(n)space=1;continue;}
  if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
       (c>='0'&&c<='9')||c=='-'||c=='\''||c=='?'||c=='!'))return 0;
  if(space){if(n>=VOICE_TEXT)return 0;out[n++]=' ';space=0;}
  if(n>=VOICE_TEXT)return 0;
  if(c>='a'&&c<='z')c-=32u;
  out[n++]=(char)c;seen++;
 }
 out[n]='\0';return seen>0;
}
void voice_profile_reset(VoiceProfile *p){if(p)memset(p,0,sizeof(*p));}
int voice_profile_valid(const VoiceProfile *p){
 unsigned i,j,count=0;
 char normalized[VOICE_TEXT+1u];
 if(!p||p->used>VOICE_SLOTS||p->next>=VOICE_SLOTS)return 0;
 for(i=0;i<VOICE_SLOTS;i++){
  const VoiceEntry *e=&p->entries[i];
  if(e->samples>2u||e->next_sample>1u)return 0;
  /* Check acoustic features even when loading an independently
   * checksummed sidecar. Each stored model bin is quantized to 0..63. */
  {
   unsigned sample,bin,feature;
   for(sample=0u;sample<2u;sample++)
    for(bin=0u;bin<VOICE_BINS;bin++)
     for(feature=0u;feature<VOICE_FEATURES;feature++)
      if(e->models[sample][bin][feature]>63u)return 0;
  }
  if(!e->samples){if(e->label[0])return 0;continue;}
  if(!memchr(e->label,0,sizeof(e->label)))return 0;
  if(!label_normalize(normalized,e->label)||strcmp(normalized,e->label))return 0;
  for(j=0;j<i;j++)if(p->entries[j].samples&&strcmp(e->label,p->entries[j].label)==0)return 0;
  count++;
 }
 return count==p->used;
}
void voice_capture_reset(VoiceCapture *c){if(c)memset(c,0,sizeof(*c));}
void voice_capture_pcm16(VoiceCapture *c,const int16_t *pcm,unsigned samples){
 unsigned i,sum=0,change=0,zcr=0,slow=0,energy,hit;
 int32_t dc=0;
 int prev=0,lag[8]={0};
 uint8_t row[VOICE_FEATURES];
 if(!c||!pcm||c->state>=VOICE_READY||samples!=VOICE_FRAME_SAMPLES)return;
 /* Mic hardware and emulators can return a fixed nonzero ADC baseline
  * even in silence. Measuring abs(raw) would falsely classify this as
  * speech and light the entire input meter. Remove the block mean first.
  * 256 * 32768 fits safely in signed 32-bit accumulation. */
 for(i=0;i<samples;i++)dc+=(int32_t)pcm[i];
 dc/=(int32_t)samples;
 for(i=0;i<samples;i++){
  int x=((int32_t)pcm[i]-dc)/256;
  sum+=abs_i(x);
  if(i){
   change+=abs_i(x-prev);
   if((x>=4&&prev<=-4)||(x<=-4&&prev>=4))zcr++;
  }
  if(i>=8u)slow+=abs_i(x-lag[i&7u]);
  lag[i&7u]=x;prev=x;
 }
 energy=sum/samples;
 row[0]=(uint8_t)(energy>255u?255u:energy);
 row[1]=(uint8_t)(zcr>255u?255u:zcr);
 row[2]=(uint8_t)((change/samples)>255u?255u:(change/samples));
 row[3]=(uint8_t)((slow/samples)>255u?255u:(slow/samples));
 c->level=row[0];
 if(c->blocks<UINT16_MAX)c->blocks++;
 if(c->state==VOICE_CALIBRATE){
  /* Average rather than accumulate into an 8-bit sum: four blocks of
   * moderate fan/hiss can exceed 255 despite a completely steady room.
   * The old saturated mean under-reported noise and falsely heard words. */
  unsigned heard=(unsigned)c->calibration;
  c->noise=(uint8_t)(((unsigned)c->noise*heard+energy+
                       heard/2u)/(heard+1u));
  if(++c->calibration==4u)c->state=VOICE_WAIT;
  return;
 }
 /* Display activity ABOVE the calibrated room floor, not the raw AC
  * level: a loud steady fan is not a full-screen "voice" meter. */
 c->level=(uint8_t)(energy>(unsigned)c->noise?
                       energy-(unsigned)c->noise:0u);
 /* A quietly spoken vowel can have mean energy below 9 even in a
  * quiet room. Use separate onset/release thresholds, relative to
  * ambient noise, to retain its softer trailing syllables. The
  * two-consecutive-block onset and four-block release remain intact.
  * In a loud room the relative margin grows with the noise floor. */
 {
  unsigned floor=(unsigned)c->noise;
  unsigned margin=c->state==VOICE_CAPTURING?1u:3u;
  unsigned minimum=c->state==VOICE_CAPTURING?3u:4u;
  hit=energy>=minimum&&energy>=floor+margin+floor/8u;
 }
 if(c->state==VOICE_WAIT){
  if(!hit){
   /* Follow slow room-noise changes without teaching the classifier
    * that a sudden voice burst is the new baseline. Integer low-pass,
    * applied ONLY to sub-threshold samples, with no extra session data. */
   c->noise=(uint8_t)(((unsigned)c->noise*7u+energy+4u)/8u);
   c->tail=0u;
   if(c->blocks>=VOICE_MAX_WAIT)c->state=VOICE_NO_SPEECH;
   return;
  }
  /* One loud 31ms frame can be a mic pop or a dropped cartridge touch.
   * Keep the first frame in the existing scratch buffer, require a
   * consecutive second frame, then enter CAPTURING with both frames
   * included. No extra PCM storage or loss of the word's first syllable. */
  if(!c->tail){
   memcpy(c->measurements[0],row,sizeof(row));
   c->tail=1u;
   return;
  }
  memcpy(c->measurements[1],row,sizeof(row));
  c->frames=2u;c->tail=0u;
  c->state=VOICE_CAPTURING;
  return;
 }
 if(c->state!=VOICE_CAPTURING)return;
 if(c->frames>=VOICE_RAW_FRAMES){c->state=c->frames>=4u?VOICE_READY:VOICE_TOO_SHORT;return;}
 memcpy(c->measurements[c->frames],row,sizeof(row));c->frames++;
 if(hit)c->tail=0;
 else c->tail++;
 if(c->tail>=4u||c->frames==VOICE_RAW_FRAMES){
  c->frames=(uint8_t)(c->frames-c->tail);
  c->state=c->frames>=4u?VOICE_READY:VOICE_TOO_SHORT;
 }
}
static int pattern(const VoiceCapture *c,uint8_t out[VOICE_BINS][VOICE_FEATURES]){
 unsigned i,j,maximum=0;
 if(!c||c->state!=VOICE_READY||c->frames<4u||c->frames>VOICE_RAW_FRAMES)return 0;
 for(i=0;i<c->frames;i++)if(c->measurements[i][0]>maximum)maximum=c->measurements[i][0];
 if(maximum<4u)return 0;
 for(i=0;i<VOICE_BINS;i++){
  unsigned frame=(i*(c->frames-1u)+(VOICE_BINS-1u)/2u)/(VOICE_BINS-1u);
  const uint8_t *src=c->measurements[frame];
  out[i][0]=cap63((unsigned)src[0]*63u/maximum);
  out[i][1]=cap63((unsigned)src[1]/3u);
  out[i][2]=cap63((unsigned)src[2]*47u/maximum);
  out[i][3]=cap63((unsigned)src[3]*47u/maximum);
  for(j=0;j<VOICE_FEATURES;j++)if(out[i][j]>63u)return 0;
 }
 return 1;
}
int voice_find(const VoiceProfile *p,const char *label){
 unsigned i;
 char normalized[VOICE_TEXT+1u];
 if(!p||!label_normalize(normalized,label))return -1;
 for(i=0u;i<VOICE_SLOTS;i++)
  if(p->entries[i].samples&&strcmp(p->entries[i].label,normalized)==0)
   return (int)i;
 return -1;
}
int voice_cycle(const VoiceProfile *p,int current,int direction){
 unsigned i,at;
 if(!p||!p->used||!voice_profile_valid(p)||!direction)return -1;
 /* Invalid selection starts at the first/last occupied slot. The
  * ordinary next/previous cycle includes the current slot when it is
  * the only remaining lesson. */
 at=current>=0&&current<(int)VOICE_SLOTS?(unsigned)current:
    direction>0?VOICE_SLOTS-1u:0u;
 for(i=0u;i<VOICE_SLOTS;i++){
  at=direction>0?(at+1u)%VOICE_SLOTS:
                 (at+VOICE_SLOTS-1u)%VOICE_SLOTS;
  if(p->entries[at].samples)return (int)at;
 }
 return -1;
}
int voice_forget(VoiceProfile *p,unsigned slot){
 if(!p||slot>=VOICE_SLOTS||!voice_profile_valid(p)||
    !p->entries[slot].samples)return 0;
 memset(&p->entries[slot],0,sizeof(p->entries[slot]));
 p->used--;
 /* Reuse an explicitly vacated slot before touching another lesson.
  * Leave all other labels and their templates byte-for-byte intact. */
 p->next=p->used?(uint8_t)slot:0u;
 return 1;
}
int voice_train(VoiceProfile *p,const char *label,const VoiceCapture *c){
 unsigned i,k,seek;
 char text[VOICE_TEXT+1u];
 uint8_t sample[VOICE_BINS][VOICE_FEATURES];
 VoiceEntry *entry;
 int result=1;
 if(!p||!voice_profile_valid(p)||!label_normalize(text,label)||!pattern(c,sample))return 0;
 i=VOICE_SLOTS;
 for(seek=0u;seek<VOICE_SLOTS;seek++)
  if(p->entries[seek].samples&&strcmp(p->entries[seek].label,text)==0){
   i=seek;break;
  }
 if(i<VOICE_SLOTS)result=2;
 else{
  /* Explicit retention: a full library cannot silently destroy the
   * oldest voice memory. The player can forget a chosen label in MIC LAB
   * and train a new phrase in the resulting free slot. */
  if(p->used==VOICE_SLOTS)return 3;
  for(seek=0u;seek<VOICE_SLOTS;seek++){
   unsigned candidate=(p->next+seek)%VOICE_SLOTS;
   if(!p->entries[candidate].samples){i=candidate;break;}
  }
  if(i==VOICE_SLOTS)return 0;
  p->used++;
  memset(&p->entries[i],0,sizeof(p->entries[i]));
  memcpy(p->entries[i].label,text,strlen(text)+1u);
  p->next=(uint8_t)((i+1u)%VOICE_SLOTS);
 }
 entry=&p->entries[i];k=entry->next_sample;
 memcpy(entry->models[k],sample,sizeof(sample));
 if(entry->samples<2u)entry->samples++;
 entry->next_sample=(uint8_t)((k+1u)%2u);
 return result;
}
/* Bounded dynamic time warping improves tolerance for nonuniform
 * syllable lengths. The 12x12 band is only +/-3 bins, so it cannot
 * reorder phonemes or stretch a tiny noise burst into an entire word.
 * No allocations, trained-template changes or per-frame work. */
static unsigned voice_frame_cost(const uint8_t *a,const uint8_t *b){
 unsigned k,sum=0;
 for(k=0;k<VOICE_FEATURES;k++){
  unsigned d=a[k]>b[k]?a[k]-b[k]:b[k]-a[k];
  sum+=d*(k==0u?2u:1u);
 }
 return sum;
}
static unsigned distance(const uint8_t *a,const uint8_t *b){
 enum {WARP_BAND=3u,WARP_PENALTY=9u};
 uint16_t costs[2][VOICE_BINS+1u];
 uint8_t steps[2][VOICE_BINS+1u];
 unsigned i,j;
 for(j=0u;j<=VOICE_BINS;j++){costs[0][j]=UINT16_MAX;steps[0][j]=0u;}
 costs[0][0]=0u;
 for(i=1u;i<=VOICE_BINS;i++){
  unsigned now=i&1u,prev=(i-1u)&1u;
  costs[now][0]=UINT16_MAX;steps[now][0]=0u;
  for(j=1u;j<=VOICE_BINS;j++){
   unsigned best,count,alternate,frame_cost;
   if(i>j+WARP_BAND||j>i+WARP_BAND){
    costs[now][j]=UINT16_MAX;steps[now][j]=0u;
    continue;
   }
   best=costs[prev][j-1u];count=steps[prev][j-1u];
   alternate=costs[prev][j];
   if(alternate!=UINT16_MAX&&alternate+WARP_PENALTY<best){
    best=alternate+WARP_PENALTY;count=steps[prev][j];
   }
   alternate=costs[now][j-1u];
   if(alternate!=UINT16_MAX&&alternate+WARP_PENALTY<best){
    best=alternate+WARP_PENALTY;count=steps[now][j-1u];
   }
   if(best==UINT16_MAX){
    costs[now][j]=UINT16_MAX;steps[now][j]=0u;
    continue;
   }
   frame_cost=voice_frame_cost(a+(i-1u)*VOICE_FEATURES,
                               b+(j-1u)*VOICE_FEATURES);
   costs[now][j]=(uint16_t)(best+frame_cost);
   steps[now][j]=(uint8_t)(count+1u);
  }
 }
 /* Path-normalized units remain identical to the old threshold scale;
  * matching the same identical recording still scores exactly zero. */
 return steps[VOICE_BINS&1u][VOICE_BINS]?
  (unsigned)costs[VOICE_BINS&1u][VOICE_BINS]/
             steps[VOICE_BINS&1u][VOICE_BINS]:UINT16_MAX;
}
int voice_recognize(const VoiceProfile *p,const VoiceCapture *c,unsigned *score){
 unsigned i,j,best=UINT16_MAX,second=UINT16_MAX;int winner=-1;
 uint8_t test[VOICE_BINS][VOICE_FEATURES];
 if(score)*score=UINT16_MAX;
 if(!p||!voice_profile_valid(p)||!pattern(c,test))return -1;
 for(i=0;i<VOICE_SLOTS;i++)if(p->entries[i].samples){
  unsigned d=UINT16_MAX;
  for(j=0;j<p->entries[i].samples;j++){
   unsigned check=distance(&test[0][0],&p->entries[i].models[j][0][0]);
   if(check<d)d=check;
  }
  if(d<best){second=best;best=d;winner=(int)i;}
  else if(d<second)second=d;
 }
 if(score)*score=best;
 /* Unknown speakers/noise must not silently issue typed commands.
  * Recognized labels always require an explicit USE confirmation. */
 if(winner<0||best>55u||(second!=UINT16_MAX&&second-best<9u))return -1;
 return winner;
}
const char *voice_label(const VoiceProfile *p,unsigned slot){
 return p&&slot<VOICE_SLOTS&&p->entries[slot].samples?p->entries[slot].label:"";
}
