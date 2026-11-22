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
  c->noise=(uint8_t)((unsigned)c->noise+energy>255u?255u:c->noise+energy);
  if(++c->calibration==4u){c->noise=(uint8_t)(c->noise/4u);c->state=VOICE_WAIT;}
  return;
 }
 hit=energy>=9u && energy>(unsigned)c->noise+6u;
 if(c->state==VOICE_WAIT){
  if(!hit){
   if(c->blocks>=VOICE_MAX_WAIT)c->state=VOICE_NO_SPEECH;
   return;
  }
  c->state=VOICE_CAPTURING;
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
 if(maximum<9u)return 0;
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
int voice_train(VoiceProfile *p,const char *label,const VoiceCapture *c){
 unsigned i,k;
 char text[VOICE_TEXT+1u];
 uint8_t sample[VOICE_BINS][VOICE_FEATURES];
 VoiceEntry *entry;
 int result=1;
 if(!p||!voice_profile_valid(p)||!label_normalize(text,label)||!pattern(c,sample))return 0;
 for(i=0;i<VOICE_SLOTS;i++)if(p->entries[i].samples&&strcmp(p->entries[i].label,text)==0)break;
 if(i<VOICE_SLOTS)result=2;
 else{
  i=p->next;
  if(!p->entries[i].samples)p->used++;
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
static unsigned distance(const uint8_t *a,const uint8_t *b){
 unsigned i,j,sum=0;
 for(i=0;i<VOICE_BINS;i++)for(j=0;j<VOICE_FEATURES;j++){
  unsigned k=i*VOICE_FEATURES+j;
  unsigned d=a[k]>b[k]?a[k]-b[k]:b[k]-a[k];
  sum+=d*(j==0u?2u:1u);
 }
 return sum/VOICE_BINS;
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
