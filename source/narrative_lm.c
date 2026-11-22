#include "narrative_lm.h"
#include <stdint.h>
#include <string.h>

#include "narrative_lm_weights.inc"

/* Three learned word embeddings and one joint speaker/topic/stage embedding
 * feed a clipped ReLU hidden layer. Scores are actual next-word neural
 * inference. Words are assembled only after scoring; no sentence table or
 * sentence IDs exist here. All temporaries are bounded stack storage.
 *
 * Embeddings and weights use Q5 signed int8, biases use Q10 int16.
 * Accumulation stays int32: 52*112*112+12288 < 2^20 for the first layer,
 * 48*112*112+12288 < 2^20 for the second. Hidden activation is Q5.
 */
#define LM_CHOICES 4u

static uint32_t next_random(uint32_t *state){
 uint32_t x=*state;
 x^=x<<13;x^=x>>17;x^=x<<5;
 *state=x;
 return x;
}

static const char *word_at(unsigned token){
 return lm_words+lm_word_offsets[token];
}

static int punctuation(unsigned token){
 const char *word=word_at(token);
 return word[1]=='\0'&&(word[0]=='.'||word[0]==','||
                        word[0]=='?'||word[0]=='!');
}

static int terminal(unsigned token){
 const char *word=word_at(token);
 return word[1]=='\0'&&(word[0]=='.'||word[0]=='?'||word[0]=='!');
}

static int permitted(unsigned previous,unsigned context,unsigned candidate){
 unsigned bit=candidate&7u;
 return ((lm_allowed[previous*LM_MASK_BYTES+candidate/8u]>>bit)&1u)&&
        ((lm_context_allowed[context*LM_MASK_BYTES+candidate/8u]>>bit)&1u);
}

/* Reusing a complete three-token segment is unnecessary in these brief
 * replies. Discourage a loop without prohibiting useful words such as I,
 * THE or LITTLE elsewhere in the sentence. This is a decoding penalty,
 * not a factual rule or a replacement language template. */
static int repeated_triple(const uint16_t *history,unsigned length,
                            unsigned candidate){
 unsigned i;
 if(length<3u)return 0;
 for(i=0;i+2u<length;i++)
  if(history[i]==history[length-2u]&&
     history[i+1u]==history[length-1u]&&history[i+2u]==candidate)return 1;
 return 0;
}

static unsigned predict(const uint16_t previous[3],unsigned context,
                         const uint16_t *emitted,unsigned emitted_count,
                         unsigned word_count,uint32_t *rng){
 int16_t input[LM_INPUT],hidden[LM_HIDDEN];
 int32_t best_score[LM_CHOICES]={INT32_MIN,INT32_MIN,INT32_MIN,INT32_MIN};
 uint16_t best_token[LM_CHOICES]={0,0,0,0};
 uint32_t weights[LM_CHOICES],total=0,choice;
 unsigned i,j,n,found=0;
 for(i=0;i<3u;i++)for(j=0;j<LM_EMBED;j++)
  input[i*LM_EMBED+j]=lm_token_embedding[(unsigned)previous[i]*LM_EMBED+j];
 for(j=0;j<LM_CONTEXT;j++)
  input[3u*LM_EMBED+j]=lm_context_embedding[context*LM_CONTEXT+j];
 for(i=0;i<LM_HIDDEN;i++){
  int32_t sum=lm_hidden_bias[i];
  for(j=0;j<LM_INPUT;j++)sum+=(int32_t)input[j]*lm_input_weight[j*LM_HIDDEN+i];
  sum=sum>0?sum/LM_SCALE:0;
  hidden[i]=(int16_t)(sum>LM_HIDDEN_CLIP?LM_HIDDEN_CLIP:sum);
 }
 for(i=0;i<LM_VOCAB;i++){
  int32_t score;
  if(i==1u||!permitted(previous[2],context,i))continue;
  if(i==0u&&word_count<4u)continue;
  score=lm_output_bias[i];
  for(j=0;j<LM_HIDDEN;j++)score+=(int32_t)hidden[j]*lm_output_weight[j*LM_VOCAB+i];
  if(repeated_triple(emitted,emitted_count,i))score-=8192;
  /* Toward the end of the token budget prefer a legal sentence stop.
   * No punctuation can be introduced where its learned bigram is absent. */
  if(emitted_count>=18u&&i>1u&&terminal(i))
   score+=(int32_t)(emitted_count-17u)*512;
  for(j=0;j<LM_CHOICES;j++)if(score>best_score[j]){
   for(n=LM_CHOICES-1u;n>j;n--){
    best_score[n]=best_score[n-1u];best_token[n]=best_token[n-1u];
   }
   best_score[j]=score;best_token[j]=(uint16_t)i;
   if(found<LM_CHOICES)found++;
   break;
  }
 }
 if(!found)return 0u;
 for(i=0;i<found;i++){
  unsigned difference=(unsigned)(best_score[0]-best_score[i]);
  unsigned slot=difference/64u;
  /* Exclude a tiny tail of off-distribution continuations. A lower
   * probability word remains selectable when the neural model gives it
   * at least exp(-4) of the best candidate's relative mass. */
  weights[i]=slot<=32u?lm_exp[slot]:0u;
  total+=weights[i];
 }
 /* The best candidate always has weight65535, so total cannot be zero. */
 choice=next_random(rng)%total;
 for(i=0;i<found;i++){
  if(choice<weights[i])return best_token[i];
  choice-=weights[i];
 }
 return best_token[0];
}

unsigned narrative_lm_generate(char *out,size_t cap,unsigned speaker,
                               unsigned topic,unsigned stage,uint32_t seed){
 uint16_t previous[3]={1,1,1},emitted[NARRATIVE_LM_MAX_TOKENS];
 uint32_t rng;
 size_t length=0,limit,last_stop=0;
 unsigned count=0,words=0,words_at_stop=0,context;
 int truncated=0;
 if(!out||!cap)return 0u;
 out[0]='\0';
 if(cap==1u)return 0u;
 if(speaker>1u)speaker=NARRATIVE_MONITOR;
 if(topic>7u)topic=NARRATIVE_UNKNOWN;
 if(stage>3u)stage=3u;
 context=(speaker*8u+topic)*4u+stage;
 rng=seed^0x9e3779b9u^(context*0x85ebca6bu);
 if(!rng)rng=0x6d2b79f5u;
 limit=cap-1u;
 if(limit>NARRATIVE_LM_MAX_CHARS)limit=NARRATIVE_LM_MAX_CHARS;
 while(count<NARRATIVE_LM_MAX_TOKENS){
  unsigned token=predict(previous,context,emitted,count,words,&rng);
  const char *word;
  size_t word_length,separator;
  int is_punctuation;
  if(!token)break;
  word=word_at(token);word_length=strlen(word);
  is_punctuation=punctuation(token);
  separator=length&&!is_punctuation?1u:0u;
  if(word_length+separator>limit-length){truncated=1;break;}
  if(separator)out[length++]=' ';
  memcpy(out+length,word,word_length);length+=word_length;
  out[length]='\0';
  if(!is_punctuation)words++;
  if(terminal(token)){last_stop=length;words_at_stop=words;}
  emitted[count++]=(uint16_t)token;
  previous[0]=previous[1];previous[1]=previous[2];previous[2]=(uint16_t)token;
 }
 if(count==NARRATIVE_LM_MAX_TOKENS)truncated=1;
 if(truncated&&last_stop){length=last_stop;words=words_at_stop;}
 if(length&&out[length-1u]!= '.'&&out[length-1u]!='?'&&out[length-1u]!='!'){
  if(out[length-1u]==',')out[length-1u]='.';
  else if(length<limit)out[length++]='.';
 }
 out[length]='\0';
 return words;
}

size_t narrative_lm_rom_bytes(void){
 return sizeof(lm_token_embedding)+sizeof(lm_context_embedding)+
        sizeof(lm_input_weight)+sizeof(lm_hidden_bias)+
        sizeof(lm_output_weight)+sizeof(lm_output_bias)+
        sizeof(lm_allowed)+sizeof(lm_context_allowed)+
        sizeof(lm_word_offsets)+sizeof(lm_words)+sizeof(lm_exp);
}
unsigned narrative_lm_vocabulary_size(void){return LM_VOCAB;}
unsigned narrative_lm_parameter_count(void){return LM_PARAMETERS;}

_Static_assert(LM_INPUT==3u*LM_EMBED+LM_CONTEXT,"input embedding shape");
_Static_assert(LM_VOCAB<=512u,"bounded decoder vocabulary");
_Static_assert(NARRATIVE_LM_MAX_TOKENS<=32u,"bounded emitted token history");
