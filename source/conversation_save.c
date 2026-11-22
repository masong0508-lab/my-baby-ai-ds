#include "conversation_save.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MIND_MAGIC 0x4e4e424du
#define MIND_VERSION 1u
typedef struct {
 uint32_t magic;
 uint16_t version,size;
 uint32_t sequence,game_hash;
 ConversationProfile profile;
 uint32_t hash;
} MindRecord;

static uint32_t digest(const void *data,size_t size){
 const unsigned char *p=data;
 uint32_t h=2166136261u;
 size_t i;
 for(i=0;i<size;i++){h^=p[i];h*=16777619u;}
 return h;
}
static int path(char *out,size_t cap,const char *base,unsigned slot){
 int n;
 if(!base||!base[0]||slot>1u)return 0;
 n=snprintf(out,cap,"%s.mind.%u",base,slot);
 return n>0&&(size_t)n<cap;
}
int conversation_save_load(const char *base,const Game *saved,
                           ConversationProfile *out){
 char name[192];
 MindRecord record;
 FILE *file;
 size_t read;
 int extra,failed,closed;
 if(!saved||!out||!path(name,sizeof(name),base,saved->sequence&1u))return 0;
 file=fopen(name,"rb");
 if(!file)return 0;
 read=fread(&record,1,sizeof(record),file);
 extra=fgetc(file);failed=ferror(file);closed=fclose(file);
 if(read!=sizeof(record)||extra!=EOF||failed||closed||
    record.magic!=MIND_MAGIC||record.version!=MIND_VERSION||
    record.size!=sizeof(ConversationProfile)||
    record.sequence!=saved->sequence||
    record.game_hash!=digest(saved,sizeof(*saved))||
    record.hash!=digest(&record,offsetof(MindRecord,hash))||
    !conversation_profile_valid(&record.profile))return 0;
 *out=record.profile;
 return 1;
}
int conversation_save_write(const char *base,const Game *saved,
                            const ConversationProfile *profile){
 char name[192];
 MindRecord record;
 FILE *file;
 if(!saved||!conversation_profile_valid(profile)||
    !path(name,sizeof(name),base,saved->sequence&1u))return 0;
 memset(&record,0,sizeof(record));
 record.magic=MIND_MAGIC;record.version=MIND_VERSION;
 record.size=(uint16_t)sizeof(ConversationProfile);
 record.sequence=saved->sequence;
 record.game_hash=digest(saved,sizeof(*saved));
 record.profile=*profile;
 record.hash=digest(&record,offsetof(MindRecord,hash));
 file=fopen(name,"wb");
 if(!file)return 0;
 if(fwrite(&record,1,sizeof(record),file)!=sizeof(record)||fflush(file)!=0){
  fclose(file);return 0;
 }
 return fclose(file)==0;
}
int conversation_save_erase(const char *base){
 char name[192];
 unsigned slot;
 for(slot=0;slot<2u;slot++){
  if(!path(name,sizeof(name),base,slot))return 0;
  if(remove(name)!=0&&errno!=ENOENT)return 0;
 }
 return 1;
}
