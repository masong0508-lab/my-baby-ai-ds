#include "save.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define MAGIC 0x4941424du
typedef struct {uint32_t magic;uint16_t version,size;Game game;uint32_t hash;} Record;
static uint32_t digest(const Record *r){
 const unsigned char *p=(const unsigned char *)r;uint32_t h=2166136261u;size_t i;
 for(i=0;i<offsetof(Record,hash);i++){h^=p[i];h*=16777619u;}return h;
}
static int path(char *out,size_t cap,const char *base,int slot){
 int n;if(!base||!base[0]||(slot!=0&&slot!=1))return 0;
 n=snprintf(out,cap,"%s.%d",base,slot);return n>0&&(size_t)n<cap;
}
static int read_one(const char *base,int slot,Record *r){
 char fn[192];FILE *f;size_t sz;
 if(!path(fn,sizeof(fn),base,slot))return 0;
 f=fopen(fn,"rb");if(!f)return 0;
 sz=fread(r,1,sizeof(*r),f);fclose(f);
 return sz==sizeof(*r)&&r->magic==MAGIC&&r->version==1&&r->size==sizeof(Game)&&r->hash==digest(r)&&
        (r->game.unlocked&255u)==255u&&r->game.hunger<=100&&r->game.mood<=100&&r->game.corruption<=100;
}
int save_load(const char *base,Game *out){
 Record a,b;int x,y;
 if(!out)return 0;
 x=read_one(base,0,&a);y=read_one(base,1,&b);if(!x&&!y)return 0;
 *out=(y&&(!x||b.game.sequence>=a.game.sequence))?b.game:a.game;return 1;
}
int save_write(const char *base,Game *s){
 Record a,b,r;int x,y,slot;char fn[192];FILE *f;
 if(!s)return 0;
 x=read_one(base,0,&a);y=read_one(base,1,&b);
 slot=!x?0:!y?1:a.game.sequence<=b.game.sequence?0:1;
 if(!path(fn,sizeof(fn),base,slot))return 0;
 memset(&r,0,sizeof(r));r.magic=MAGIC;r.version=1;r.size=sizeof(Game);r.game=*s;
 r.game.sequence=s->sequence+1;r.hash=digest(&r);
 f=fopen(fn,"wb");if(!f)return 0;
 if(fwrite(&r,1,sizeof(r),f)!=sizeof(r)||fflush(f)!=0){fclose(f);return 0;}
 if(fclose(f)!=0)return 0;
 s->sequence=r.game.sequence;
 return 1;
}
int save_erase(const char *base){
 int i;for(i=0;i<2;i++){char fn[192];FILE *f;if(!path(fn,sizeof(fn),base,i))return 0;
 f=fopen(fn,"rb");if(!f)continue;fclose(f);if(remove(fn)!=0)return 0;}
 for(i=0;i<2;i++){char fn[192];FILE *f;if(!path(fn,sizeof(fn),base,i))return 0;
 f=fopen(fn,"rb");if(f){fclose(f);return 0;}}
 return 1;
}
