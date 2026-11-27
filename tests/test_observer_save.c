#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "observer_save.h"
#include "save.h"

static char directory[100],base[128],filename[192];
static void name_for(const char *suffix){
 int n=snprintf(filename,sizeof(filename),"%s%s",base,suffix);
 assert(n>0&&(size_t)n<sizeof(filename));
}
static size_t read_bytes(unsigned char *bytes,size_t cap){
 FILE *file=fopen(filename,"rb");
 size_t n;
 assert(file);n=fread(bytes,1,cap,file);
 assert(!ferror(file));assert(fclose(file)==0);return n;
}
static void write_bytes(const unsigned char *bytes,size_t size){
 FILE *file=fopen(filename,"wb");
 assert(file);assert(fwrite(bytes,1,size,file)==size);assert(fclose(file)==0);
}
static void rehash(unsigned char *bytes,size_t size){
 uint32_t h=2166136261u;
 size_t i;
 for(i=0;i<size-4u;i++){h^=bytes[i];h*=16777619u;}
 memcpy(bytes+size-4u,&h,sizeof(h));
}
static void rejects_unchanged(const Game *g){
 ObserverProfile before,out;
 memset(&before,0x55,sizeof(before));out=before;
 assert(!observer_save_load(base,g,&out));
 assert(memcmp(&before,&out,sizeof(out))==0);
}
int main(void){
 Game g,first,loaded,unchanged;
 ObserverProfile p,older,newer,out;
 ObserverSession s;
 unsigned char bytes[512],original[512];
 size_t size;
 int n=snprintf(directory,sizeof(directory),"/tmp/baby-observer-save-%ld",(long)getpid());
 assert(n>0&&(size_t)n<sizeof(directory));assert(mkdir(directory,0700)==0);
 n=snprintf(base,sizeof(base),"%s/baby",directory);
 assert(n>0&&(size_t)n<sizeof(base));

 game_init(&g);observer_profile_reset(&p,&g);observer_session_reset(&s,&g);
 assert(save_write(base,&g));first=g;
 assert(save_load(base,&loaded));assert(memcmp(&g,&loaded,sizeof(g))==0);
 rejects_unchanged(&loaded); /* An old v1 Game needs no watch file. */
 name_for(".0");size=read_bytes(bytes,sizeof(bytes));
 assert(size>8u&&bytes[4]==1u&&bytes[5]==0u);
 assert(observer_teach(&p,"TINY BOSS")==1);
 assert(observer_teach(&p,"TINY BOSS")==2);
 p.exposure=65u;p.integrity=92u;p.attacks=2u;p.hits=1u;
 p.blocks=1u;p.slaps=2u;p.last_event=OBSERVER_EVENT_HIT;
 p.last_event_day=g.days;p.last_effect=8u;
 older=p;unchanged=g;
 assert(observer_save_write(base,&g,&p));
 assert(memcmp(&unchanged,&g,sizeof(g))==0);
 assert(observer_save_load(base,&g,&out));assert(memcmp(&p,&out,sizeof(p))==0);
 observer_session_reset(&s,&g);
 assert(strcmp(observer_answer(&out,&s,&g,1,"SAY"),"YOU TAUGHT ME: TINY BOSS")==0);
 assert(out.phrases[0].confidence==2u&&out.integrity==92u&&out.hits==1u);
 name_for(".watch.1");size=read_bytes(original,sizeof(original));
 assert(size==276u);

 /* Matching sequence alone cannot import another baby's words/history. */
 loaded=g;loaded.rng^=1u;rejects_unchanged(&loaded);
 loaded=g;loaded.days++;rejects_unchanged(&loaded);
 loaded=g;loaded.hunger++;rejects_unchanged(&loaded);
 loaded=g;loaded.sequence+=2u;rejects_unchanged(&loaded);

 /* Exact EOF, headers/checksum and canonical phrase/profile validation. */
 write_bytes(original,size-1u);rejects_unchanged(&g);
 memcpy(bytes,original,size);bytes[size]=0;write_bytes(bytes,size+1u);rejects_unchanged(&g);
 memcpy(bytes,original,size);bytes[size-1u]^=1u;write_bytes(bytes,size);rejects_unchanged(&g);
 memcpy(bytes,original,size);bytes[0]^=1u;rehash(bytes,size);write_bytes(bytes,size);rejects_unchanged(&g);
 memcpy(bytes,original,size);bytes[4]=3u;rehash(bytes,size);write_bytes(bytes,size);rejects_unchanged(&g);
 memcpy(bytes,original,size);bytes[6]++;rehash(bytes,size);write_bytes(bytes,size);rejects_unchanged(&g);
 memcpy(bytes,original,size);bytes[16u+offsetof(ObserverProfile,integrity)]=101u;
 rehash(bytes,size);write_bytes(bytes,size);rejects_unchanged(&g);
 memcpy(bytes,original,size);bytes[16u+offsetof(ObserverProfile,phrase_count)]=4u;
 rehash(bytes,size);write_bytes(bytes,size);rejects_unchanged(&g);
 memcpy(bytes,original,size);bytes[16u+offsetof(ObserverProfile,phrases)+25u]=16u;
 rehash(bytes,size);write_bytes(bytes,size);rejects_unchanged(&g);
 memcpy(bytes,original,size);
 memset(bytes+16u+offsetof(ObserverProfile,phrases),'A',25u);
 rehash(bytes,size);write_bytes(bytes,size);rejects_unchanged(&g);
 write_bytes(original,size);assert(observer_save_load(base,&g,&out));
 /* Reconstruct the exact v1 wire layout from the common prefix.
  * The 152nd profile byte used to be padding. It must not become slot 5. */
 memset(bytes,0,172u);
 memcpy(bytes,original,16u+151u);
 bytes[4]=1u;bytes[5]=0u;bytes[6]=152u;bytes[7]=0u;
 bytes[167]=0xA5u;
 rehash(bytes,172u);write_bytes(bytes,172u);
 assert(observer_save_load(base,&g,&out));
 assert(memcmp(&p,&out,sizeof(p))==0);
 assert(out.phrases[4].confidence==0u&&out.phrases[4].text[0]==0);
 bytes[16u+offsetof(ObserverProfile,phrase_next)]=4u;
 rehash(bytes,172u);write_bytes(bytes,172u);rejects_unchanged(&g);
 write_bytes(original,size);assert(observer_save_load(base,&g,&out));

 /* An older v1 fallback gets exactly its older observer profile. */
 (void)game_care(&g,FEED);assert(save_write(base,&g));
 p.integrity=80u;assert(observer_teach(&p,"HELLO NURSERY")==1);newer=p;
 assert(observer_save_write(base,&g,&p));
 assert(save_load(base,&loaded));assert(observer_save_load(base,&loaded,&out));
 assert(memcmp(&out,&newer,sizeof(out))==0);
 name_for(".1");write_bytes((const unsigned char *)"broken",6u);
 assert(save_load(base,&loaded));assert(memcmp(&loaded,&first,sizeof(first))==0);
 assert(observer_save_load(base,&loaded,&out));assert(memcmp(&out,&older,sizeof(out))==0);

 /* Real host write/flush failure cannot corrupt the already saved Game.
  * The caller can load normal progress and reset this optional profile. */
 assert(save_write(base,&g));unchanged=g;
 name_for((g.sequence&1u)?".watch.1":".watch.0");
 assert(remove(filename)==0);
 if(access("/dev/full",W_OK)==0){
  assert(symlink("/dev/full",filename)==0);
  assert(!observer_save_write(base,&g,&p));
  assert(memcmp(&unchanged,&g,sizeof(g))==0);
  assert(save_load(base,&loaded));assert(memcmp(&loaded,&g,sizeof(g))==0);
  rejects_unchanged(&loaded);assert(unlink(filename)==0);
 }
 assert(observer_save_write(base,&g,&p));

 /* Erase is scoped to two .watch records. v1, .mind and unrelated notes
  * remain available; the engine never reaches into another game's files. */
 name_for(".watch.notes");write_bytes((const unsigned char *)"keep",4u);
 name_for(".mind.0");write_bytes((const unsigned char *)"mind",4u);
 assert(observer_save_erase(base));assert(save_load(base,&loaded));rejects_unchanged(&loaded);
 name_for(".watch.notes");assert(read_bytes(bytes,sizeof(bytes))==4u);
 assert(memcmp(bytes,"keep",4u)==0);assert(remove(filename)==0);
 name_for(".mind.0");assert(read_bytes(bytes,sizeof(bytes))==4u);
 assert(memcmp(bytes,"mind",4u)==0);assert(remove(filename)==0);
 assert(save_erase(base));

 /* Sequence wrap to zero still has an exact-Game bound optional record. */
 game_init(&g);g.sequence=UINT32_MAX;
 assert(save_write(base,&g));assert(g.sequence==0u);
 assert(observer_save_write(base,&g,&p));
 assert(save_load(base,&loaded));assert(observer_save_load(base,&loaded,&out));
 assert(memcmp(&p,&out,sizeof(p))==0);
 assert(save_write(base,&g));assert(g.sequence==1u);
 assert(observer_save_write(base,&g,&p));assert(observer_save_load(base,&g,&out));
 /* Persist a deliberately removed lesson without changing the watch
  * record size or losing the surviving lesson. */
 assert(observer_forget_phrase(&p,0u));
 assert(p.phrase_count==1u&&observer_profile_valid(&p));
 assert(save_write(base,&g));
 assert(observer_save_write(base,&g,&p));assert(observer_save_load(base,&g,&out));
 assert(memcmp(&p,&out,sizeof(p))==0&&out.phrases[0].confidence==0u);
 assert(strcmp(out.phrases[1].text,"HELLO NURSERY")==0);
 assert(observer_teach(&out,"NEW LESSON")==1);
 assert(strcmp(out.phrases[0].text,"NEW LESSON")==0);
 for(unsigned j=2u;j<8u;j++){
  char label[16];snprintf(label,sizeof(label),"LESSON %u",j+1u);
  assert(observer_teach(&out,label)==1);
 }
 assert(out.phrase_count==8u);
 assert(observer_save_write(base,&g,&out));
 {ObserverProfile roundtrip;
  assert(observer_save_load(base,&g,&roundtrip));
  assert(memcmp(&out,&roundtrip,sizeof(out))==0);
 }
 {
  char long_base[256];
  memset(long_base,'x',sizeof(long_base));long_base[sizeof(long_base)-1u]=0;
  assert(!observer_save_write(long_base,&g,&p));
  assert(!observer_save_load(long_base,&g,&out));assert(!observer_save_erase(long_base));
  assert(!observer_save_write(NULL,&g,&p));assert(!observer_save_load(NULL,&g,&out));
  assert(!observer_save_erase(NULL));assert(!observer_save_erase(""));
  assert(!observer_save_write(base,NULL,&p));assert(!observer_save_write(base,&g,NULL));
  assert(!observer_save_load(base,NULL,&out));assert(!observer_save_load(base,&g,NULL));
  p.integrity=101;assert(!observer_save_write(base,&g,&p));
 }
 assert(observer_save_erase(base));assert(save_erase(base));assert(rmdir(directory)==0);
 puts("PASS: observer phrases/history persistence, exact-Game binding, EOF/corruption rejection, v1 recovery, write failure and rollover");
 return 0;
}
