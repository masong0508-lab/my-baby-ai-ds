#include "save.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define MAGIC 0x4941424du
typedef struct {uint32_t magic;uint16_t version,size;Game game;uint32_t hash;} Record;
/* Sequence numbers wrap after UINT32_MAX. Serial-number arithmetic keeps
 * the newest of two alternating slots correct across that rollover. */
static int newer(uint32_t a,uint32_t b){
 return a!=b&&(uint32_t)(a-b)<0x80000000u;
}
static const Record *latest_valid(const Record *a,int x,const Record *b,int y){
 if(y&&(!x||newer(b->game.sequence,a->game.sequence)||
        b->game.sequence==a->game.sequence))return b;
 return x?a:NULL;
}
static int valid_game(const Game *g){
 return g->days!=0&&g->rng!=0&&
        (g->unlocked&255u)==255u&&
        (g->ending_mask&~0x03feu)==0&&
        g->hunger<=100&&g->mood<=100&&g->energy<=100&&
        g->chaos<=100&&g->corruption<=100&&g->safe_visuals<=1;
}
static uint32_t digest(const Record *r){
 const unsigned char *p=(const unsigned char *)r;uint32_t h=2166136261u;size_t i;
 for(i=0;i<offsetof(Record,hash);i++){h^=p[i];h*=16777619u;}return h;
}
static int path(char *out,size_t cap,const char *base,int slot){
 int n;if(!base||!base[0]||(slot!=0&&slot!=1))return 0;
 n=snprintf(out,cap,"%s.%d",base,slot);return n>0&&(size_t)n<cap;
}
static int read_one(const char *base,int slot,Record *r){
 char fn[192];FILE *f;size_t sz;int extra,io_error,closed;
 if(!path(fn,sizeof(fn),base,slot))return 0;
 f=fopen(fn,"rb");if(!f)return 0;
 sz=fread(r,1,sizeof(*r),f);
 extra=fgetc(f);io_error=ferror(f);closed=fclose(f);
 /* Reject truncated or oversized records, bad checksums and implausible
  * state. A damaged newer slot must never mask the intact older one. */
 return sz==sizeof(*r)&&extra==EOF&&!io_error&&!closed&&
        r->magic==MAGIC&&r->version==1&&r->size==sizeof(Game)&&
        r->hash==digest(r)&&valid_game(&r->game);
}
int save_load(const char *base,Game *out){
 Record a,b;const Record *latest;int x,y;
 if(!out)return 0;
 x=read_one(base,0,&a);y=read_one(base,1,&b);
 latest=latest_valid(&a,x,&b,y);if(!latest)return 0;
 *out=latest->game;return 1;
}
int save_write(const char *base,Game *s){
 Record a,b,r;const Record *latest;int x,y,slot;char fn[192];FILE *f;
 if(!s||!valid_game(s))return 0;
 x=read_one(base,0,&a);y=read_one(base,1,&b);
 latest=latest_valid(&a,x,&b,y);
 slot=!x?0:!y?1:latest==&a?1:0;
 if(!path(fn,sizeof(fn),base,slot))return 0;
 memset(&r,0,sizeof(r));r.magic=MAGIC;r.version=1;r.size=sizeof(Game);r.game=*s;
 /* Room SAVE may first enable storage after starting a NEW baby in RAM.
  * Its sequence need not match disk. Advance the latest valid disk record
  * so a successful write reloads this baby, while retaining the old slot
  * as the fallback. Copy no old gameplay state into the current baby. */
 r.game.sequence=(latest?latest->game.sequence:s->sequence)+1u;
 r.hash=digest(&r);
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
