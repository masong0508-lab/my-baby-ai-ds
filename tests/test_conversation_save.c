#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "conversation_save.h"
#include "save.h"

static char directory[100],base[128];
static char filename[192];
static void name_for(const char *suffix){
 int n=snprintf(filename,sizeof(filename),"%s%s",base,suffix);
 assert(n>0&&(size_t)n<sizeof(filename));
}
static size_t read_bytes(unsigned char *bytes,size_t cap){
 FILE *file=fopen(filename,"rb");
 size_t size;
 assert(file);
 size=fread(bytes,1,cap,file);
 assert(!ferror(file));assert(fclose(file)==0);
 return size;
}
static void write_bytes(const unsigned char *bytes,size_t size){
 FILE *file=fopen(filename,"wb");
 assert(file);assert(fwrite(bytes,1,size,file)==size);
 assert(fclose(file)==0);
}
static uint32_t hash(const unsigned char *bytes,size_t size){
 uint32_t value=2166136261u;
 size_t i;
 for(i=0;i<size;i++){value^=bytes[i];value*=16777619u;}
 return value;
}
static void rehash(unsigned char *bytes,size_t size){
 uint32_t value=hash(bytes,size-sizeof(value));
 memcpy(bytes+size-sizeof(value),&value,sizeof(value));
}
static void rejects_without_changing(const Game *game){
 ConversationProfile before,after;
 memset(&before,0x55,sizeof(before));after=before;
 assert(!conversation_save_load(base,game,&after));
 assert(memcmp(&before,&after,sizeof(after))==0);
}
static void cleanup(void){
 assert(conversation_save_erase(base));
 assert(save_erase(base));
}
int main(void){
 Game game,first,second,loaded,unchanged;
 ConversationProfile profile,older,newer,out;
 unsigned char bytes[256],original[256];
 size_t size;
 int n=snprintf(directory,sizeof(directory),"/tmp/baby-neural-save-%ld",(long)getpid());
 assert(n>0&&(size_t)n<sizeof(directory));
 assert(mkdir(directory,0700)==0);
 n=snprintf(base,sizeof(base),"%s/baby",directory);
 assert(n>0&&(size_t)n<sizeof(base));

 game_init(&game);conversation_profile_reset(&profile);
 assert(conversation_profile_valid(&profile));
 /* Existing v1 records work without any learning companion. */
 assert(save_write(base,&game));first=game;
 assert(save_load(base,&loaded));assert(memcmp(&loaded,&game,sizeof(game))==0);
 rejects_without_changing(&loaded);
 name_for(".0");size=read_bytes(bytes,sizeof(bytes));
 assert(size>8u&&bytes[4]==1u&&bytes[5]==0u);

 profile.adapter[0][0]=1;profile.likes=7;profile.changes=2;
 older=profile;unchanged=game;
 assert(conversation_save_write(base,&game,&profile));
 assert(memcmp(&game,&unchanged,sizeof(game))==0);
 assert(conversation_save_load(base,&game,&out));
 assert(memcmp(&out,&profile,sizeof(out))==0);
 name_for(".mind.1");size=read_bytes(original,sizeof(original));
 assert(size==60u); /*40-byte model,16-byte binding/header,4-byte digest. */

 /* Identical sequence alone must never attach another baby's learning. */
 loaded=game;loaded.rng^=1u;rejects_without_changing(&loaded);
 loaded=game;loaded.hunger++;rejects_without_changing(&loaded);
 loaded=game;loaded.days++;rejects_without_changing(&loaded);

 /* Damage and valid-checksum invalid records fail without touching output. */
 write_bytes(original,size-1u);rejects_without_changing(&game);
 memcpy(bytes,original,size);bytes[size]=0;write_bytes(bytes,size+1u);
 rejects_without_changing(&game);
 memcpy(bytes,original,size);bytes[size-1u]^=1u;write_bytes(bytes,size);
 rejects_without_changing(&game);
 memcpy(bytes,original,size);bytes[0]^=1u;rehash(bytes,size);write_bytes(bytes,size);
 rejects_without_changing(&game);
 memcpy(bytes,original,size);bytes[4]=2u;rehash(bytes,size);write_bytes(bytes,size);
 rejects_without_changing(&game);
 memcpy(bytes,original,size);bytes[6]++;rehash(bytes,size);write_bytes(bytes,size);
 rejects_without_changing(&game);
 memcpy(bytes,original,size);bytes[16]=127u;rehash(bytes,size);write_bytes(bytes,size);
 rejects_without_changing(&game);
 write_bytes(original,size);
 assert(conversation_save_load(base,&game,&out));

 /* If the newer ordinary save is damaged, its older Game restores the
  * correspondingly older learning slot, not the newer personality. */
 (void)game_care(&game,FEED);
 assert(save_write(base,&game));second=game;
 profile.adapter[1][0]=-1;profile.likes=11;newer=profile;
 assert(conversation_save_write(base,&game,&profile));
 assert(save_load(base,&loaded));assert(loaded.sequence==second.sequence);
 assert(conversation_save_load(base,&loaded,&out));
 assert(memcmp(&out,&newer,sizeof(out))==0);
 name_for(".1");write_bytes((const unsigned char *)"broken",6);
 assert(save_load(base,&loaded));assert(memcmp(&loaded,&first,sizeof(first))==0);
 assert(conversation_save_load(base,&loaded,&out));
 assert(memcmp(&out,&older,sizeof(out))==0);

 /* A profile I/O error cannot roll back or mutate successfully saved Game. */
 assert(save_write(base,&game));unchanged=game;
 name_for((game.sequence&1u)?".mind.1":".mind.0");
 assert(remove(filename)==0);
 if(access("/dev/full",W_OK)==0){
  assert(symlink("/dev/full",filename)==0);
  assert(!conversation_save_write(base,&game,&profile));
  assert(memcmp(&game,&unchanged,sizeof(game))==0);
  assert(save_load(base,&loaded));assert(memcmp(&loaded,&game,sizeof(game))==0);
  assert(unlink(filename)==0);
 }
 assert(conversation_save_write(base,&game,&profile));

 /* Learning erasure is scoped; the v1 game and unrelated files survive. */
 name_for(".mind.notes");write_bytes((const unsigned char *)"keep",4);
 assert(conversation_save_erase(base));
 assert(save_load(base,&loaded));rejects_without_changing(&loaded);
 name_for(".mind.notes");assert(read_bytes(bytes,sizeof(bytes))==4u);
 assert(memcmp(bytes,"keep",4)==0);assert(remove(filename)==0);
 cleanup();

 /* Serial rollover remains legal; sequence zero has a real bound model. */
 game_init(&game);game.sequence=UINT32_MAX;
 assert(save_write(base,&game));assert(game.sequence==0u);
 assert(conversation_save_write(base,&game,&profile));
 assert(save_load(base,&loaded));assert(conversation_save_load(base,&loaded,&out));
 assert(memcmp(&out,&profile,sizeof(out))==0);
 assert(save_write(base,&game));assert(game.sequence==1u);
 assert(conversation_save_write(base,&game,&profile));
 assert(conversation_save_load(base,&game,&out));

 {
  char too_long[256];
  memset(too_long,'x',sizeof(too_long));too_long[sizeof(too_long)-1u]=0;
  assert(!conversation_save_write(too_long,&game,&profile));
  assert(!conversation_save_load(too_long,&game,&out));
  assert(!conversation_save_erase(too_long));
  assert(!conversation_save_write(NULL,&game,&profile));
  assert(!conversation_save_write(base,NULL,&profile));
  assert(!conversation_save_write(base,&game,NULL));
  assert(!conversation_save_load(base,NULL,&out));
  assert(!conversation_save_load(base,&game,NULL));
  profile.adapter[0][0]=127;
  assert(!conversation_save_write(base,&game,&profile));
 }
 cleanup();assert(rmdir(directory)==0);
 puts("PASS: v1 progress compatibility, bound learning slots, corruption, I/O failure, recovery and rollover");
 return 0;
}
