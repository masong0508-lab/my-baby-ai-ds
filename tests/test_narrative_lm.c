#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "narrative_lm.h"

static unsigned count_words(const char *text){
 unsigned count=0;int in_word=0;
 for(;*text;text++){
  int letter=(*text>='A'&&*text<='Z')||*text=='\'';
  if(letter&&!in_word)count++;
  in_word=letter;
 }
 return count;
}

static void valid_reply(const char *text,unsigned words){
 size_t n=strlen(text),i;
 unsigned columns=0,lines=1;
 assert(n>0u&&n<=NARRATIVE_LM_MAX_CHARS);
 assert(words>=4u&&words<=NARRATIVE_LM_MAX_TOKENS);
 assert(count_words(text)==words);
 assert(text[0]>='A'&&text[0]<='Z');
 assert(text[n-1u]=='.'||text[n-1u]=='?'||text[n-1u]=='!');
 for(i=0;i<n;i++){
  char c=text[i];
  assert((c>='A'&&c<='Z')||c==' '||c=='.'||c==','||
         c=='?'||c=='!'||c=='\'');
  assert(!(c==' '&&(i==0u||text[i-1u]==' ')));
  if(c=='.'||c==','||c=='?'||c=='!'){
   assert(i>0u&&text[i-1u]!=' ');
   if(i+1u<n)assert(text[i+1u]==' ');
  }
 }
 /* The terminal has four native 36-character rows. Count actual words,
  * including their punctuation, rather than trusting a byte-length cap. */
 for(i=0;i<n;){
  size_t end=i;
  while(end<n&&text[end]!=' ')end++;
  assert(end-i<=36u);
  if(columns&&columns+1u+(unsigned)(end-i)>36u){lines++;columns=0;}
  if(columns)columns++;
  columns+=(unsigned)(end-i);
  i=end<n?end+1u:end;
 }
 assert(lines<=4u);
}

int main(void){
 char out[160],again[160],other[160];
 unsigned speaker,topic,stage,seed,generated=0,varied=0,staged=0;
 uint32_t transcript=2166136261u;
 assert(narrative_lm_rom_bytes()<=100u*1024u);
 assert(narrative_lm_vocabulary_size()>=128u&&narrative_lm_vocabulary_size()<=512u);
 assert(narrative_lm_parameter_count()>10000u);
 /* Every combination uses the production quantized C neural inference.
  * Seeds replay exactly and are unrelated to Game's random stream. */
 for(speaker=0;speaker<2u;speaker++)for(topic=0;topic<8u;topic++)
  for(stage=0;stage<4u;stage++){
   unsigned differing=0;
   narrative_lm_generate(other,sizeof(other),speaker,topic,stage,0u);
   for(seed=0;seed<32u;seed++){
    unsigned n=narrative_lm_generate(out,sizeof(out),speaker,topic,stage,seed);
    unsigned replay=narrative_lm_generate(again,sizeof(again),speaker,topic,stage,seed);
    const char *p=out;
    valid_reply(out,n);assert(n==replay&&strcmp(out,again)==0);
    if(strcmp(out,other))differing++;
    for(;*p;p++)transcript=(transcript^(unsigned char)*p)*16777619u;
    generated++;
   }
   assert(differing>0u);varied++;
  }
 /* Conditioning must affect the actual rendered words, not only an
  * invisible score or a decorative speaker label. */
 for(speaker=0;speaker<2u;speaker++)for(topic=0;topic<8u;topic++){
  narrative_lm_generate(out,sizeof(out),speaker,topic,0u,12345u);
  narrative_lm_generate(again,sizeof(again),speaker,topic,3u,12345u);
  if(strcmp(out,again))staged++;
 }
 assert(staged>=14u);
 /* Reviewed compositions absent from every base and singly augmented
  * training sentence. These exercise the exported INTEGER weights and
  * sequential word decoder, not a sentence lookup or Python surrogate. */
 narrative_lm_generate(out,sizeof(out),NARRATIVE_MONITOR,NARRATIVE_GREETING,3u,13584u);
 assert(strcmp(out,"WARDEN ONLINE. A LITTLE ROOM CAN HOLD A BIG FEAR.")==0);
 narrative_lm_generate(out,sizeof(out),NARRATIVE_BABY,NARRATIVE_SELF,0u,12345u);
 assert(strcmp(out,"I AM A SMALL PERSON WITH A BIG IMAGINATION.")==0);
 {
  unsigned distinct_topics=0,warden=0,world=0;
  narrative_lm_generate(out,sizeof(out),NARRATIVE_BABY,NARRATIVE_GREETING,0u,12345u);
  for(topic=1;topic<8u;topic++){
   narrative_lm_generate(again,sizeof(again),NARRATIVE_BABY,topic,0u,12345u);
   if(strcmp(out,again))distinct_topics++;
  }
  assert(distinct_topics==7u);
  for(seed=0;seed<32u;seed++){
   narrative_lm_generate(out,sizeof(out),NARRATIVE_MONITOR,NARRATIVE_SELF,0u,seed);
   if(strstr(out,"WARDEN"))warden++;
   narrative_lm_generate(again,sizeof(again),NARRATIVE_BABY,NARRATIVE_WORLD,0u,seed);
   if(strstr(again,"CITY")||strstr(again,"SHELTER")||strstr(again,"WORLD"))world++;
  }
  assert(warden>0u&&world>=24u);
 }
 /* Untrusted enum/range inputs are sanitized before indexing embeddings.
  * An invalid topic remains explicitly outside the model's domain. */
 narrative_lm_generate(out,sizeof(out),UINT_MAX,UINT_MAX,UINT_MAX,0u);
 narrative_lm_generate(again,sizeof(again),NARRATIVE_MONITOR,NARRATIVE_UNKNOWN,3u,0u);
 assert(strcmp(out,again)==0);
 narrative_lm_generate(out,sizeof(out),NARRATIVE_BABY,NARRATIVE_DREAM,1u,UINT32_MAX);
 valid_reply(out,count_words(out));
 /* Exhaust every useful output capacity, including oversized requests.
  * Surrounding bytes remain untouched and partial words never appear. */
 {
  unsigned cap;
  for(cap=0;cap<=192u;cap++){
   struct {unsigned char before[16];char bytes[192];unsigned char after[16];} guard;
   unsigned n;size_t used;unsigned i;
   memset(&guard,0xa5,sizeof(guard));
   n=narrative_lm_generate(guard.bytes,cap,NARRATIVE_BABY,NARRATIVE_DREAM,3u,17u);
   for(i=0;i<16u;i++)assert(guard.before[i]==0xa5u&&guard.after[i]==0xa5u);
   if(!cap){for(i=0;i<192u;i++)assert((unsigned char)guard.bytes[i]==0xa5u);continue;}
   used=strlen(guard.bytes);assert(used<cap&&used<=NARRATIVE_LM_MAX_CHARS);
   assert(count_words(guard.bytes)==n);
   for(i=cap;i<192u;i++)assert((unsigned char)guard.bytes[i]==0xa5u);
   if(cap==1u)assert(!n&&!used);
  }
  memset(out,0xa5,sizeof(out));
  assert(narrative_lm_generate(NULL,160u,0u,0u,0u,0u)==0u);
  assert(narrative_lm_generate(NULL,0u,0u,0u,0u,0u)==0u);
  narrative_lm_generate(out,SIZE_MAX,NARRATIVE_BABY,NARRATIVE_WORLD,2u,0u);
  assert(strlen(out)<=144u);
  /* Even the xorshift zero-state seed is repaired deterministically. */
  narrative_lm_generate(out,sizeof(out),0u,0u,0u,0x9e3779b9u);
  valid_reply(out,count_words(out));
 }
 printf("Narrative LM: %u conditioned replies/replays, %u varied contexts, "
        "%u distinct stage pairs, %u tokens, %u parameters, %zu ROM bytes; "
        "transcript %08lx; PASS\n",generated,varied,staged,
        narrative_lm_vocabulary_size(),narrative_lm_parameter_count(),
        narrative_lm_rom_bytes(),(unsigned long)transcript);
 return 0;
}
