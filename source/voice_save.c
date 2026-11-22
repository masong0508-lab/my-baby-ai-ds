#include "voice_save.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define VOICE_MAGIC 0x4356414du
#define VOICE_VERSION 1u
typedef struct {
 uint32_t magic;
 uint16_t version,size;
 uint32_t sequence,game_hash;
 VoiceProfile profile;
 uint8_t reserved[2];
 uint32_t hash;
} VoiceRecord;
_Static_assert(sizeof(VoiceProfile)<3500u,"voice model budget");
_Static_assert(sizeof(VoiceRecord)<3600u,"voice saved record budget");
static uint32_t digest(const void *data,size_t size){
 const unsigned char *src=data;size_t i;uint32_t h=2166136261u;
 for(i=0;i<size;i++){h^=src[i];h*=16777619u;}
 return h;
}
static int path(char *out,size_t cap,const char *base,unsigned slot){
 int n;
 if(!base||!base[0]||slot>1u)return 0;
 n=snprintf(out,cap,"%s.voice.%u",base,slot);
 return n>0&&(size_t)n<cap;
}
int voice_save_write(const char *base,const Game *saved,const VoiceProfile *profile){
 VoiceRecord record;FILE *file;char filename[192];
 if(!saved||!voice_profile_valid(profile)||
    !path(filename,sizeof(filename),base,saved->sequence&1u))return 0;
 memset(&record,0,sizeof(record));
 record.magic=VOICE_MAGIC;record.version=VOICE_VERSION;
 record.size=(uint16_t)sizeof(VoiceProfile);
 record.sequence=saved->sequence;record.game_hash=digest(saved,sizeof(*saved));
 record.profile=*profile;
 record.hash=digest(&record,offsetof(VoiceRecord,hash));
 file=fopen(filename,"wb");if(!file)return 0;
 if(fwrite(&record,1,sizeof(record),file)!=sizeof(record)||fflush(file)!=0){
  fclose(file);return 0;
 }
 return fclose(file)==0;
}
int voice_save_load(const char *base,const Game *saved,VoiceProfile *out){
 VoiceRecord record;FILE *file;char filename[192];size_t n;int extra,bad,closed;
 if(!saved||!out||!path(filename,sizeof(filename),base,saved->sequence&1u))return 0;
 file=fopen(filename,"rb");if(!file)return 0;
 n=fread(&record,1,sizeof(record),file);
 extra=fgetc(file);bad=ferror(file);closed=fclose(file);
 if(n!=sizeof(record)||extra!=EOF||bad||closed||
    record.magic!=VOICE_MAGIC||record.version!=VOICE_VERSION||
    record.size!=sizeof(VoiceProfile)||record.sequence!=saved->sequence||
    record.game_hash!=digest(saved,sizeof(*saved))||
    record.hash!=digest(&record,offsetof(VoiceRecord,hash))||
    !voice_profile_valid(&record.profile))return 0;
 *out=record.profile;return 1;
}
int voice_save_erase(const char *base){
 unsigned slot;char filename[192];
 for(slot=0;slot<2;slot++){
  if(!path(filename,sizeof(filename),base,slot))return 0;
  if(remove(filename)!=0&&errno!=ENOENT)return 0;
 }
 return 1;
}
