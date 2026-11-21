#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "game.h"
#include "save.h"
static void test_good_endings(void){
 Game s,loaded;unsigned day, dollars, favors;
 const char *base="/tmp/my_baby_ai_good_endings";
 save_erase(base);game_init(&s);
 /* A positive nursery finale is recorded once, gives a one-time reward,
  * and never unlocks the save-wiping secret ending. */
 s.days=3;s.hunger=10;s.mood=92;s.energy=82;s.chaos=12;
 assert(game_end_day(&s)==END_GRADUATION);
 assert(s.days==4&&s.dollars==16&&s.favors==2);
 assert(game_good_ending_count(&s)==1&&!game_reset_unlocked(&s));
 dollars=s.dollars;favors=s.favors;day=s.days;
 assert(game_end_day(&s)==END_OK);
 assert(s.days==day+1&&s.dollars==dollars&&s.favors==favors);
 /* Signposted glitches can lead to an unexpectedly positive story. */
 s.corruption=52;s.chaos=12;s.mood=80;s.energy=75;
 assert(game_end_day(&s)==END_GLITCH_TAMER);
 assert(game_good_ending_count(&s)==2&&!game_reset_unlocked(&s));
 s.wins=6;s.dollars=45;s.mood=85;s.hunger=10;s.chaos=10;
 assert(game_end_day(&s)==END_ARCADE_CHAMP);
 assert(game_good_ending_count(&s)==3&&!game_reset_unlocked(&s));
 /* More good endings may be found AFTER a bad ending. */
 s.hunger=90;
 assert(game_end_day(&s)==END_BOTTLE&&s.bad_endings==1);
 assert(!game_reset_unlocked(&s)); /* Count only distinct bad endings. */
 s.days=10;s.hunger=12;s.mood=90;s.energy=86;s.chaos=10;
 assert(game_end_day(&s)==END_SECOND_CHANCE);
 assert(game_good_ending_count(&s)==4&&!game_reset_unlocked(&s));
 assert(game_is_good_ending(END_SECOND_CHANCE)&&!game_is_good_ending(END_BOTTLE));
 assert(game_ending_name(END_SECOND_CHANCE)!=NULL);
 /* Good ending never terminates the run: care and arcade still work. */
 day=s.days;(void)game_care(&s,CHAT);
 assert(s.days==day);
 game_arcade_start(&s,&(Arcade){0},0);
 assert(save_write(base,&s)&&save_load(base,&loaded));
 assert(loaded.days==s.days&&loaded.ending_mask==s.ending_mask);
 assert(loaded.bad_endings==s.bad_endings&&loaded.dollars==s.dollars);
 assert(save_erase(base));
 /* A pile of good endings alone cannot unlock a save wipe. */
 game_init(&s);s.ending_mask=(uint16_t)(0x1e0u);
 assert(game_good_ending_count(&s)==4&&!game_reset_unlocked(&s));
 /* Currency rewards must saturate rather than wrap. */
 game_init(&s);s.days=3;s.hunger=10;s.mood=92;s.energy=82;s.chaos=12;
 s.dollars=65533;s.favors=65535;
 assert(game_end_day(&s)==END_GRADUATION);
 assert(s.dollars==65535&&s.favors==65535);
}
/* Forge checksum-correct but invalid state, simulating logical corruption
 * that a simple digest check would otherwise accept. */
typedef struct {
 uint32_t magic;
 uint16_t version,size;
 Game game;
 uint32_t hash;
} TestRecord;
static uint32_t record_hash(const TestRecord *r){
 const unsigned char *p=(const unsigned char *)r;
 uint32_t h=2166136261u;size_t i;
 for(i=0;i<offsetof(TestRecord,hash);i++){h^=p[i];h*=16777619u;}
 return h;
}
static void forge_bad_stat(const char *filename,int field){
 TestRecord r;FILE *f=fopen(filename,"r+b");
 assert(f);
 assert(fread(&r,1,sizeof(r),f)==sizeof(r));
 if(field==0)r.game.energy=255;
 else if(field==1)r.game.chaos=255;
 else r.game.safe_visuals=255;
 r.hash=record_hash(&r);
 rewind(f);
 assert(fwrite(&r,1,sizeof(r),f)==sizeof(r));
 assert(fclose(f)==0);
}
static void test_save_integrity(void){
 const char *base="/tmp/my_baby_ai_integrity";
 Game s,loaded;FILE *f;
 assert(save_erase(base));game_init(&s);
 /* Two-slot rotation must survive UINT32_MAX -> 0 -> 1. */
 s.sequence=UINT32_MAX-2u;s.dollars=101;
 assert(save_write(base,&s)&&s.sequence==UINT32_MAX-1u);
 s.dollars=102;
 assert(save_write(base,&s)&&s.sequence==UINT32_MAX);
 s.dollars=103;
 assert(save_write(base,&s)&&s.sequence==0u);
 assert(save_load(base,&loaded)&&loaded.sequence==0u&&loaded.dollars==103);
 s.dollars=104;
 assert(save_write(base,&s)&&s.sequence==1u);
 assert(save_load(base,&loaded)&&loaded.sequence==1u&&loaded.dollars==104);
 assert(save_erase(base));
 /* A valid hash must not excuse impossible in-game stats. */
 game_init(&s);s.dollars=17;assert(save_write(base,&s));
 s.dollars=27;assert(save_write(base,&s));
 forge_bad_stat("/tmp/my_baby_ai_integrity.1",0);
 assert(save_load(base,&loaded)&&loaded.dollars==17);
 forge_bad_stat("/tmp/my_baby_ai_integrity.0",1);
 loaded.dollars=444;
 assert(!save_load(base,&loaded)&&loaded.dollars==444);
 assert(save_erase(base));
 game_init(&s);
 s.safe_visuals=3;
 assert(!save_write(base,&s)); /* Do not persist invalid states. */
 s.safe_visuals=0;
 assert(save_write(base,&s));
 /* Reject appended garbage instead of treating an oversized file as valid. */
 f=fopen("/tmp/my_baby_ai_integrity.0","ab");
 assert(f);assert(fputs("extra",f)>=0);assert(fclose(f)==0);
 assert(!save_load(base,&loaded));
 assert(save_erase(base));
 puts("PASS: rollover-safe saves, full state validation, extra bytes, two-slot recovery");
}
int main(void){
 Game g,h;Arcade a;int i;
 const char *base="/tmp/my_baby_ai_testslot";
 test_good_endings();
 test_save_integrity();
 save_erase(base);game_init(&g);
 assert(g.unlocked==255&&!game_reset_unlocked(&g));
 assert(game_unlocked(&g,0)&&game_unlocked(&g,7));
 assert(!game_unlocked(&g,-1)&&!game_unlocked(&g,16)&&!game_unlocked(&g,8));
 assert(!game_unlock(&g,8));
 g.hunger=90;assert(game_end_day(&g)==END_BOTTLE);
 g.hunger=20;g.mood=10;assert(game_end_day(&g)==END_CEO);
 g.mood=60;g.corruption=90;g.chaos=75;
 assert(game_end_day(&g)==END_REBOOT);
 assert(g.bad_endings==3&&game_reset_unlocked(&g));
 /* The first eight cabinets are bespoke now; keep old arrow rules tested
  * against locked cabinet 8 via an isolated unlock flag. */
 g.unlocked|=(uint16_t)(1u<<8);
 g.corruption=80;game_arcade_start(&g,&a,8);assert(a.active);
 for(i=0;i<2;i++)game_arcade_input(&g,&a,game_arcade_expected(&a));
 assert(a.round==2&&a.reverse&&game_arcade_expected(&a)==(a.direction^2));
 for(i=2;i<MBAI_ROUNDS;i++)game_arcade_input(&g,&a,game_arcade_expected(&a));
 assert(a.finished==1&&g.wins==1);
 assert(save_write(base,&g));g.dollars=203;assert(save_write(base,&g));
 assert(save_load(base,&h)&&h.dollars==203);
 {FILE *f=fopen("/tmp/my_baby_ai_testslot.1","wb");assert(f);fputs("damaged",f);fclose(f);}
 assert(save_load(base,&h)&&h.dollars!=203);
 assert(save_erase(base)&&!save_load(base,&h));
 game_init(&g);assert(!game_reset_unlocked(&g));
 puts("PASS: good+bad endings, continuing play, one-time rewards, safe reset gating, save recovery");
 return 0;
}
