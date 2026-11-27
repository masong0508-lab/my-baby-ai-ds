#include "observer_save.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define WATCH_MAGIC 0x5743424du
#define WATCH_VERSION 2u
#define WATCH_V1_PROFILE_SIZE 152u
#define WATCH_V1_PHRASES 4u
typedef struct {
 uint32_t magic;
 uint16_t version,size;
 uint32_t sequence,game_hash;
 ObserverProfile profile;
 uint32_t hash;
} WatchRecord;
typedef struct {
 uint32_t magic;
 uint16_t version,size;
 uint32_t sequence,game_hash;
 unsigned char profile[WATCH_V1_PROFILE_SIZE];
 uint32_t hash;
} WatchRecordV1;
_Static_assert(sizeof(WatchRecordV1)==172u,"observer v1 record layout");
_Static_assert(sizeof(ObserverProfile)==256u,"observer v2 profile layout");
_Static_assert(sizeof(WatchRecord)==276u,"observer v2 record layout");
_Static_assert(offsetof(ObserverProfile,phrases)+
 WATCH_V1_PHRASES*sizeof(ObserverPhrase)==151u,"legacy profile prefix layout");

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
 n=snprintf(out,cap,"%s.watch.%u",base,slot);
 return n>0&&(size_t)n<cap;
}
int observer_save_load(const char *base,const Game *saved,ObserverProfile *out){
 char name[192];
 WatchRecord record;
 FILE *file;
 size_t read;
 int extra,failed,closed;
 if(!saved||!out||!path(name,sizeof(name),base,saved->sequence&1u))return 0;
 file=fopen(name,"rb");
 if(!file)return 0;
 read=fread(&record,1,sizeof(record),file);
 extra=fgetc(file);failed=ferror(file);closed=fclose(file);
 if(extra!=EOF||failed||closed)return 0;
 if(read==sizeof(WatchRecordV1)){
  WatchRecordV1 legacy;
  ObserverProfile upgraded;
  memcpy(&legacy,&record,sizeof(legacy));
  if(legacy.magic!=WATCH_MAGIC||legacy.version!=1u||
     legacy.size!=WATCH_V1_PROFILE_SIZE||legacy.sequence!=saved->sequence||
     legacy.game_hash!=digest(saved,sizeof(*saved))||
     legacy.hash!=digest(&legacy,offsetof(WatchRecordV1,hash)))return 0;
  /* The old profile's final byte is padding, not part of slot five.
   * Copy only the meaningful prefix; initialize new slots to empty. */
  memset(&upgraded,0,sizeof(upgraded));
  memcpy(&upgraded,legacy.profile,offsetof(ObserverProfile,phrases)+
         WATCH_V1_PHRASES*sizeof(ObserverPhrase));
  if(upgraded.phrase_count>WATCH_V1_PHRASES||
     upgraded.phrase_next>=WATCH_V1_PHRASES||
     !observer_profile_valid(&upgraded))return 0;
  *out=upgraded;
  return 1;
 }
 if(read!=sizeof(record)||
    record.magic!=WATCH_MAGIC||record.version!=WATCH_VERSION||
    record.size!=sizeof(ObserverProfile)||record.sequence!=saved->sequence||
    record.game_hash!=digest(saved,sizeof(*saved))||
    record.hash!=digest(&record,offsetof(WatchRecord,hash))||
    !observer_profile_valid(&record.profile))return 0;
 *out=record.profile;
 return 1;
}
int observer_save_write(const char *base,const Game *saved,
                         const ObserverProfile *profile){
 char name[192];
 WatchRecord record;
 FILE *file;
 if(!saved||!observer_profile_valid(profile)||
    !path(name,sizeof(name),base,saved->sequence&1u))return 0;
 memset(&record,0,sizeof(record));
 record.magic=WATCH_MAGIC;record.version=WATCH_VERSION;
 record.size=(uint16_t)sizeof(ObserverProfile);
 record.sequence=saved->sequence;record.game_hash=digest(saved,sizeof(*saved));
 record.profile=*profile;
 record.hash=digest(&record,offsetof(WatchRecord,hash));
 file=fopen(name,"wb");
 if(!file)return 0;
 if(fwrite(&record,1,sizeof(record),file)!=sizeof(record)||fflush(file)!=0){
  fclose(file);return 0;
 }
 return fclose(file)==0;
}
int observer_save_erase(const char *base){
 char name[192];
 unsigned slot;
 for(slot=0;slot<2u;slot++){
  if(!path(name,sizeof(name),base,slot))return 0;
  if(remove(name)!=0&&errno!=ENOENT)return 0;
 }
 return 1;
}
