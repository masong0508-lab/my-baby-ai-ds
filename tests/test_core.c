#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "game.h"
#include "save.h"
/* Guidance never changes the save or consumes the RNG. A critical
 * exhausted baby needs a NAP first even if also hungry. Helping the
 * worst need removes the hint without a permanently blinking warning. */
static void test_care_hint(void){
 Game s,original;
 game_init(&s);
 assert(game_care_hint(NULL)==-1);
 assert(game_care_hint(&s)==-1);
 original=s;
 assert(game_care_hint(&s)==-1);
 assert(memcmp(&s,&original,sizeof(s))==0);
 s.hunger=80;
 assert(game_care_hint(&s)==FEED);
 s.energy=10;
 assert(game_care_hint(&s)==NAP);
 s.energy=30;
 assert(game_care_hint(&s)==FEED);
 s.hunger=20;
 assert(game_care_hint(&s)==NAP);
 s.energy=90;s.chaos=79;
 assert(game_care_hint(&s)==NAP);
 s.chaos=15;s.mood=25;
 assert(game_care_hint(&s)==CHAT);
 s.mood=75;
 assert(game_care_hint(&s)==-1);
 /* The actual FEED/NAP calls lower the corresponding need and the
  * hint updates immediately, including through the existing save. */
 s.hunger=76;
 assert(game_care_hint(&s)==FEED);
 (void)game_care(&s,FEED);
 assert(s.hunger==46&&game_care_hint(&s)==-1);
 s.energy=12;
 assert(game_care_hint(&s)==NAP);
 (void)game_care(&s,NAP);
 assert(s.energy==37&&game_care_hint(&s)==-1);
}
static void test_peek_horror(void){
 Game g,original;
 NurseryPeek p;
 unsigned hits=0;
 game_init(&g);original=g;
 assert(game_peek_start(&g,&p));
 assert(!game_peek_scare_roll(&p,0u)); /* early tap cannot trigger */
 while(p.phase==PEEK_WAIT)(void)game_peek_tick(&p);
 /* Exhaust all 20-bit outcomes. Exactly one may trigger. */
 for(uint32_t draw=0u;draw<(1u<<20);draw++){
  NurseryPeek candidate=p;
  if(game_peek_scare_roll(&candidate,draw)){
   hits++;assert(candidate.phase==PEEK_SCARE);
  }else assert(candidate.phase==PEEK_OPEN);
 }
 assert(hits==1u);
 assert(!game_peek_scare_roll(&p,1u));
 assert(game_peek_scare_roll(&p,1u<<20));
 assert(p.phase==PEEK_SCARE&&p.ticks==PEEK_SCARE_TICKS);
 assert(!game_peek_tap(&g,&p)&&!game_peek_scare_roll(&p,0u));
 for(unsigned i=0;i<PEEK_SCARE_TICKS;i++)(void)game_peek_tick(&p);
 assert(p.phase==PEEK_CORRUPT&&p.ticks==PEEK_CORRUPT_TICKS);
 for(unsigned i=0;i<PEEK_CORRUPT_TICKS;i++)(void)game_peek_tick(&p);
 assert(p.phase==PEEK_LOCKED);
 for(unsigned i=0;i<1000u;i++){
  assert(!game_peek_tick(&p)&&!game_peek_tap(&g,&p));
  assert(p.phase==PEEK_LOCKED);
 }
 assert(memcmp(&g,&original,sizeof(g))==0);
}
static void test_peekaboo(void){
 Game g,before;NurseryPeek p;
 game_init(&g);before=g;
 assert(game_peek_start(&g,&p)&&p.phase==PEEK_WAIT);
 assert(memcmp(&g,&before,sizeof(g))==0);
 /* An early tap is a strike, never a successful reveal. */
 assert(game_peek_tap(&g,&p)&&p.misses==1u&&p.round==0u);
 assert(memcmp(&g,&before,sizeof(g))==0);
 for(unsigned round=0;round<3u;round++){
  unsigned ticks=0;
  while(p.phase==PEEK_WAIT&&ticks++<120u)(void)game_peek_tick(&p);
  assert(p.phase==PEEK_OPEN&&ticks<=120u);
  assert(game_peek_tap(&g,&p)&&p.round==round+1u);
  if(round<2u)assert(memcmp(&g,&before,sizeof(g))==0);
 }
 assert(p.phase==PEEK_WON&&g.mood==87u&&g.energy==67u&&g.hunger==41u&&g.chaos==16u);
 assert(g.rng!=before.rng&&g.wins==before.wins&&g.dollars==before.dollars&&g.days==before.days);
 before=g;assert(!game_peek_tap(&g,&p)&&!game_peek_tick(&p));
 assert(memcmp(&g,&before,sizeof(g))==0);
 assert(game_peek_start(&g,&p));
 assert(game_peek_tap(&g,&p)&&game_peek_tap(&g,&p)&&p.phase==PEEK_LOST);
 assert(memcmp(&g,&before,sizeof(g))==0);
 assert(game_peek_start(&g,&p));
 for(unsigned i=0;i<400u&&p.phase!=PEEK_LOST;i++)(void)game_peek_tick(&p);
 assert(p.phase==PEEK_LOST&&p.misses==2u);
 assert(memcmp(&g,&before,sizeof(g))==0);
 g.hunger=70;before=g;assert(!game_peek_start(&g,&p));
 assert(memcmp(&g,&before,sizeof(g))==0);
 g.hunger=20;g.energy=24;assert(!game_peek_start(&g,&p));
 g.energy=80;g.chaos=75;assert(!game_peek_start(&g,&p));
 assert(!game_peek_start(NULL,&p)&&!game_peek_start(&g,NULL));
}
static void test_room_lullaby(void){
 Game s,before;
 game_init(&s);s.mood=70;s.chaos=23;s.energy=50;s.hunger=30;
 before=s;
 assert(game_room_soothe(&s));
 assert(s.mood==76&&s.chaos==15&&s.energy==47&&s.hunger==32);
 assert(s.rng!=before.rng&&s.days==before.days&&s.dollars==before.dollars);
 assert(s.corruption==before.corruption&&s.ending_mask==before.ending_mask);
 s.mood=99;s.chaos=2;assert(game_room_soothe(&s));
 assert(s.mood==100&&s.chaos==0);
 before=s;assert(!game_room_soothe(&s));assert(memcmp(&s,&before,sizeof(s))==0);
 s.hunger=70;s.chaos=40;before=s;
 assert(!game_room_soothe(&s));assert(memcmp(&s,&before,sizeof(s))==0);
 s.hunger=20;s.energy=15;before=s;
 assert(!game_room_soothe(&s));assert(memcmp(&s,&before,sizeof(s))==0);
 assert(!game_room_soothe(NULL));
 /* A player cannot repeat the activity indefinitely without other care. */
 game_init(&s);s.chaos=100;s.energy=16;s.hunger=68;
 assert(game_room_soothe(&s));assert(s.energy==13&&s.hunger==70);
 before=s;assert(!game_room_soothe(&s));assert(memcmp(&s,&before,sizeof(s))==0);
}
static void test_room_quiet_rest(void){
 Game s,original;
 game_init(&s);
 s.energy=50;s.chaos=45;
 original=s;
 assert(!game_room_quiet_rest(&s,1,0));
 assert(!game_room_quiet_rest(&s,0,1));
 assert(!game_room_quiet_rest(NULL,0,0));
 assert(memcmp(&s,&original,sizeof(s))==0);
 /* CRIB uses the ordinary care path, then gets one small extra night
  * bonus only when the player actually switches the lamp off. */
 (void)game_care(&s,NAP);
 assert(s.energy==75&&s.chaos==40);
 assert(game_room_quiet_rest(&s,0,0));
 assert(s.energy==83&&s.chaos==36);
 assert(s.corruption==0); /* resting removes two ordinary glitches */
 /* Values must saturate safely in saved uint8 game state. */
 s.energy=98;s.chaos=2;s.corruption=1;
 assert(game_room_quiet_rest(&s,0,0));
 assert(s.energy==100&&s.chaos==0&&s.corruption==0);
 /* Even maximum corruption is recoverable using real quiet nights,
  * while saved bad-ending counters and discovered story are preserved. */
 s.corruption=100;s.bad_endings=2;s.ending_mask=(1u<<END_BOTTLE);
 {
  int n;
  for(n=0;n<50;n++)assert(game_room_quiet_rest(&s,0,0));
 }
 assert(s.corruption==0);
 assert(s.bad_endings==2&&s.ending_mask==(1u<<END_BOTTLE));
 original=s;
 assert(!game_room_quiet_rest(&s,0,1));
 assert(memcmp(&s,&original,sizeof(s))==0);
}
/* End Day is now a real life-sim clock tick. Without care, the baby
 * becomes hungry, sleepy, less cheerful and slightly restless.
 * Ending selection is based on the day JUST COMPLETED, so a threshold
 * crossed overnight applies on the next review rather than causing a
 * surprise ending one frame after the player presses the button. */
static void test_day_need_cycle(void){
 Game s;
 uint32_t day;
 int i;
 game_init(&s);
 day=s.days;
 for(i=0;i<6;i++){
  assert(game_end_day(&s)==END_OK);
  assert(s.days==day+(uint32_t)i+1u);
  assert(s.hunger==(uint8_t)(35+8*(i+1)));
  assert(s.energy==(uint8_t)(75-6*(i+1)));
  assert(s.mood==(uint8_t)(75-2*(i+1)));
  assert(s.chaos==(uint8_t)(12+2*(i+1)));
 }
 assert(s.hunger>=78&&game_care_hint(&s)==FEED);
 assert(game_end_day(&s)==END_BOTTLE);
 assert(s.bad_endings==1u);
 assert(s.hunger<78); /* recovery ends the feedback loop */
 assert(s.days==day+7u);
 /* The existing care actions can reverse the daily deterioration. */
 s.hunger=75;s.energy=50;
 (void)game_care(&s,FEED);
 (void)game_care(&s,NAP);
 assert(s.hunger==53&&s.energy==75);
 /* Exhaustive 101x101 boundary matrix: no softlock, negative energy,
  * overflow or instant ending just because a threshold crossed overnight.
  * Original bad-ending recovery happens before the new-day depletion. */
 {
  int h,e;
  for(h=0;h<=100;h++)for(e=0;e<=100;e++){
   int ending,expected_energy;
   game_init(&s);
   s.hunger=(uint8_t)h;s.energy=(uint8_t)e;
   s.mood=50;s.chaos=10;
   ending=game_end_day(&s);
   assert(s.days==2u);
   if(h<78){
    expected_energy=e>6?e-6:0;
    assert(ending==END_OK);
    assert(s.hunger==(uint8_t)(h+8>100?100:h+8));
    assert(s.energy==(uint8_t)expected_energy);
    assert(s.mood==48&&s.chaos==12);
   }else{
    /* A bad bottle ending reduces hunger then starts a fresh night.
     * This recovery must survive both zero and full energy values. */
    expected_energy=(e+30>100?100:e+30)-6;
    assert(ending==END_BOTTLE);
    assert(s.hunger==(uint8_t)(h-35+8));
    assert(s.energy==(uint8_t)expected_energy);
    assert(s.mood==78&&s.chaos==2);
   }
  }
 }
 /* Valid save numbers saturate rather than wrap to forbidden day zero.
  * Exercise the REAL two-slot serializer, not only the in-memory count. */
 {
  const char *base="/tmp/my_baby_ai_max_day";
  Game loaded;
  assert(save_erase(base));
  game_init(&s);s.days=UINT32_MAX;
  assert(game_end_day(&s)==END_OK);
  assert(s.days==UINT32_MAX);
  assert(save_write(base,&s));
  assert(save_load(base,&loaded));
  assert(loaded.days==UINT32_MAX);
  assert(save_erase(base));
 }
}
/* 360 days of actual player-directed care and day progression.
 * The hint is NEVER an autopilot in the game; this harness deliberately
 * chooses the recommended action to prove a healthy player strategy can
 * remain viable long-term, and that saves can round-trip along the way. */
static void test_long_term_rehabilitation(void){
 const char *base="/tmp/my_baby_ai_360_day_cycle";
 Game s,reloaded;
 unsigned day,total_care=0,normal_days=0;
 assert(save_erase(base));
 game_init(&s);
 for(day=0;day<360u;day++){
  int steps;
  for(steps=0;steps<30;steps++){
   int hint=game_care_hint(&s);
   if(hint<0)break;
   assert(hint==FEED||hint==CHAT||hint==NAP);
   (void)game_care(&s,(Care)hint);
   total_care++;
  }
  assert(steps<30); /* Player can recover ordinary needs. */
  assert(s.hunger<70&&s.energy>35&&s.mood>40&&s.chaos<70);
  {
   uint32_t before=s.days;
   int ending=game_end_day(&s);
   assert(ending>=END_OK&&ending<=END_SECOND_CHANCE);
   normal_days+=(ending==END_OK);
   assert(s.days==before+1u);
  }
  assert(s.hunger<=100&&s.energy<=100&&s.mood<=100);
  assert(s.chaos<=100&&s.corruption<=100);
  if(day%30u==0u){
   assert(save_write(base,&s));
   assert(save_load(base,&reloaded));
   assert(memcmp(&s,&reloaded,sizeof(s))==0);
  }
 }
 assert(s.days==361u);
 assert(total_care>100u&&normal_days>300u);
 assert(save_erase(base));
}
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
/* A bad run cannot permanently strand the player: all eight starter
 * cabinets remain free. Earning wins and favors by actually completing
 * free rounds must eventually unlock every paid cabinet, even at maximum
 * corruption and with no initial cash. Do NOT hand-set the unlock mask. */
static void test_arcade_economy_recovery(void){
 Game g;
 Arcade a;
 unsigned req_wins,req_cash,req_favors,total_wins=0;
 int id,i,retries;
 game_init(&g);
 g.dollars=0;g.favors=0;
 g.hunger=100;g.energy=0;g.mood=0;g.chaos=100;g.corruption=100;
 assert(!game_unlock_requirements(0,&req_wins,&req_cash,&req_favors));
 assert(!game_unlock_requirements(7,&req_wins,&req_cash,&req_favors));
 assert(!game_unlock_requirements(16,&req_wins,&req_cash,&req_favors));
 for(id=8;id<MBAI_GAMES;id++){
  assert(game_unlock_requirements(id,&req_wins,&req_cash,&req_favors));
  assert(req_wins==(unsigned)(id-7)*2u);
  assert(req_cash==(unsigned)(10+(id-8)*2));
  assert(req_favors==(unsigned)((id-8)/2+1));
  for(retries=0;retries<256&&!game_unlock(&g,id);retries++){
   /* Sock Market is one of the original free cabinets. All its rounds
    * are fully played through the real gameplay engine. */
   assert(game_unlocked(&g,5));
   game_arcade_start(&g,&a,5);
   assert(a.active);
   for(i=0;i<MBAI_ROUNDS;i++){
    assert(a.active);
    game_arcade_input(&g,&a,game_arcade_expected(&a));
   }
   assert(!a.active&&a.finished==1);
   total_wins++;
  }
  assert(retries<256&&game_unlocked(&g,id));
 }
 assert(g.unlocked==0xffffu && total_wins>=32u);
 assert(g.wins==total_wins);
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
/* A RAM-only START deliberately does not load the old baby. Enabling
 * storage later must nevertheless make the CURRENT baby the next load,
 * with the previous saved baby retained as the two-slot crash fallback. */
static void test_save_live_baby(void){
 const char *base="/tmp/my_baby_ai_live_save";
 const struct {
  uint32_t disk_sequence,ram_sequence;
  unsigned writes,remove_first;
 } cases[]={
  {99u,0u,1u,0u}, /* only slot zero exists */
  {99u,0u,2u,0u}, /* slot one is newest */
  {99u,0u,3u,0u}, /* slot zero is newest */
  {99u,0u,2u,1u}, /* only slot one survives */
  {UINT32_MAX-2u,0u,2u,0u}, /* write wraps disk MAX -> zero */
  {UINT32_MAX-1u,0u,2u,0u}, /* disk already wrapped to zero */
  {0x80000000u,0u,2u,0u}, /* not comparable with fresh RAM zero */
  {99u,0x40000000u,2u,0u} /* stale RAM counter must not supersede disk */
 };
 unsigned i,j;
 for(i=0;i<sizeof(cases)/sizeof(cases[0]);i++){
  Game old_baby,live_baby,expected,loaded;
  char fn[192];FILE *f;
  assert(save_erase(base));
  game_init(&old_baby);
  old_baby.days=40;old_baby.dollars=321;
  old_baby.sequence=cases[i].disk_sequence;
  for(j=0;j<cases[i].writes;j++)assert(save_write(base,&old_baby));
  if(cases[i].remove_first)assert(remove("/tmp/my_baby_ai_live_save.0")==0);
  game_init(&live_baby);
  live_baby.days=7;live_baby.dollars=11;
  live_baby.sequence=cases[i].ram_sequence;
  expected=live_baby;
  expected.sequence=old_baby.sequence+1u;
  assert(save_write(base,&live_baby));
  assert(memcmp(&live_baby,&expected,sizeof(expected))==0);
  assert(save_load(base,&loaded));
  assert(memcmp(&loaded,&live_baby,sizeof(loaded))==0);
  /* Damage the just-written slot. The previous disk state remains fully
   * recoverable even though the new baby's counter began elsewhere. */
  assert(snprintf(fn,sizeof(fn),"%s.%u",base,cases[i].writes%2u)>0);
  f=fopen(fn,"wb");assert(f);
  assert(fputs("interrupted",f)>=0);assert(fclose(f)==0);
  assert(save_load(base,&loaded));
  assert(memcmp(&loaded,&old_baby,sizeof(loaded))==0);
  /* Retrying repairs the damaged slot; ordinary later care persists. */
  live_baby.days=8;
  assert(save_write(base,&live_baby));
  assert(save_load(base,&loaded)&&loaded.days==8u);
  live_baby.days=9;
  assert(save_write(base,&live_baby));
  assert(save_load(base,&loaded)&&loaded.days==9u);
  assert(loaded.sequence==old_baby.sequence+2u);
  assert(save_erase(base));
 }
 puts("PASS: enabling saves mid-game retains current baby and backup across sequence rollover");
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
 /* An inaccessible save target cannot falsely claim a successful write,
  * nor advance the in-memory sequence used by the two-slot recovery logic. */
 {
  Game original=s;
  assert(!save_write(NULL,&s));
  assert(memcmp(&s,&original,sizeof(s))==0);
 }
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
 test_room_quiet_rest();
 test_room_lullaby();
 test_peekaboo();
 test_peek_horror();
 test_care_hint();
 test_day_need_cycle();
 test_long_term_rehabilitation();
 test_good_endings();
 test_arcade_economy_recovery();
 test_save_integrity();
 test_save_live_baby();
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
 /* New cabinet 12 verifies accounting approval/flag rules at high glitch. */
 g.unlocked|=(uint16_t)(1u<<12);
 g.corruption=80;game_arcade_start(&g,&a,12);assert(a.active);
 assert(a.timer==125&&game_arcade_expected(&a)==a.target);
 for(i=0;i<MBAI_ROUNDS;i++){
  int should_flag=a.stage!=a.direction;
  assert(a.target==(unsigned)should_flag);
  game_arcade_input(&g,&a,game_arcade_expected(&a));
 }
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
