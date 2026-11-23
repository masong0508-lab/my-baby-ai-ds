#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "voice_save.h"
#include "save.h"
static char directory[100],base[128];
static void rehash_record(unsigned char *data,size_t size){
 uint32_t hash=2166136261u;
 size_t i;
 assert(size>=4u);
 for(i=0u;i<size-4u;i++){hash^=data[i];hash*=16777619u;}
 memcpy(data+size-4u,&hash,sizeof(hash));
}
static void name(char *out,const Game *g){
 assert(snprintf(out,192,"%s.voice.%u",base,g->sequence&1u)>0);
}
static void rejects(const Game *g){
 VoiceProfile untouched,after;
 memset(&untouched,0x55,sizeof(untouched));after=untouched;
 assert(!voice_save_load(base,g,&after));
 assert(memcmp(&untouched,&after,sizeof(after))==0);
}
int main(void){
 Game g,loaded,older;VoiceProfile p,out;char fn[192];
 assert(snprintf(directory,sizeof(directory),"/tmp/baby-voice-%ld",(long)getpid())>0);
 assert(mkdir(directory,0700)==0);
 assert(snprintf(base,sizeof(base),"%s/game",directory)>0);
 game_init(&g);voice_profile_reset(&p);
 assert(save_write(base,&g));older=g;rejects(&g);
 /* Templates, never raw audio, survive across two version-1 slots. */
 strcpy(p.entries[0].label,"HELLO");p.entries[0].samples=1;p.used=1;
 for(unsigned n=0;n<VOICE_BINS;n++)p.entries[0].models[0][n][0]=(uint8_t)n;
 assert(voice_profile_valid(&p));
 assert(voice_save_write(base,&g,&p));
 assert(voice_save_load(base,&g,&out));assert(memcmp(&p,&out,sizeof(p))==0);
 loaded=g;loaded.hunger=(uint8_t)(loaded.hunger+1u);rejects(&loaded);
 loaded=g;loaded.sequence+=2u;rejects(&loaded);
 name(fn,&g);
 FILE *f=fopen(fn,"ab");assert(f&&fputc(0,f)!=EOF&&fclose(f)==0);rejects(&g);
 assert(voice_save_write(base,&g,&p));
 f=fopen(fn,"rb+");assert(f);assert(fputc('x',f)!=EOF&&fclose(f)==0);rejects(&g);
 assert(voice_save_write(base,&g,&p));
 /* A crafted, checksummed profile with out-of-range acoustic bins must
  * also be rejected. Its Game and normal learned voice record survive. */
 {
  unsigned char binary[3600];
  size_t size,at=16u+offsetof(VoiceProfile,entries)+
                      offsetof(VoiceEntry,models);
  f=fopen(fn,"rb");assert(f);
  size=fread(binary,1,sizeof(binary),f);
  assert(!ferror(f)&&fclose(f)==0&&size>at+4u);
  assert(binary[at]<64u);
  binary[at]=64u;rehash_record(binary,size);
  f=fopen(fn,"wb");assert(f);
  assert(fwrite(binary,1,size,f)==size&&fclose(f)==0);
  rejects(&g);
  assert(voice_save_write(base,&g,&p));
  assert(voice_save_load(base,&g,&out));
  assert(memcmp(&p,&out,sizeof(p))==0);
 }
 (void)game_care(&g,FEED);assert(save_write(base,&g));
 strcpy(p.entries[1].label,"WORLD");p.entries[1].samples=1;p.used=2;
 assert(voice_save_write(base,&g,&p));
 assert(voice_save_load(base,&g,&out));assert(out.used==2);
 assert(voice_save_load(base,&older,&out));assert(out.used==1);
 name(fn,&g);assert(remove(fn)==0);
 if(access("/dev/full",W_OK)==0){
  assert(symlink("/dev/full",fn)==0);
  assert(!voice_save_write(base,&g,&p));
  assert(unlink(fn)==0);
 }
 assert(voice_save_write(base,&g,&p));
 /* An explicitly forgotten spoken word survives disk round-trip as
  * absent, while unrelated words and Game progress stay untouched. */
 {
  VoiceProfile erased=p;
  assert(voice_forget(&erased,0u));
  assert(erased.used==1u&&voice_find(&erased,"HELLO")==-1);
  assert(voice_save_write(base,&g,&erased));
  assert(voice_save_load(base,&g,&out));
  assert(memcmp(&erased,&out,sizeof(out))==0);
  assert(voice_find(&out,"WORLD")==1);
  assert(voice_forget(&erased,1u));
  assert(erased.used==0u&&voice_profile_valid(&erased));
  assert(voice_save_write(base,&g,&erased));
  assert(voice_save_load(base,&g,&out));
  assert(out.used==0u&&voice_cycle(&out,-1,1)==-1);
 }
 assert(voice_save_erase(base));rejects(&g);rejects(&older);
 assert(save_load(base,&loaded));
 assert(!voice_save_write(0,&g,&p));assert(!voice_save_load(0,&g,&p));
 assert(!voice_save_write(base,0,&p));assert(!voice_save_write(base,&g,0));
 assert(save_erase(base));assert(rmdir(directory)==0);
 puts("PASS: voice lesson sidecars, exact-Game binding, corruption/EOF, old-slot recovery and I/O failure");
 return 0;
}
