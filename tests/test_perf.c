#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "perf.h"
static uint32_t sample(PerfTracker *p,uint32_t now,unsigned scene,unsigned frames){
 for(unsigned i=1;i<=frames;i++){
  uint32_t tick=now+(uint32_t)((uint64_t)PERF_CLOCK_HZ*i/frames);
  assert(perf_frame(p,tick,scene)==(i==frames));
 }
 return now+PERF_CLOCK_HZ;
}
int main(void){
 PerfTracker p;
 uint32_t now=0xffff0000u;
 perf_init(&p);
 assert(!perf_current(&p));
 assert(perf_scene(0,0)==0u&&perf_scene(14,0)==14u);
 assert(perf_scene(2,0)==16u&&perf_scene(2,15)==31u);
 assert(perf_scene(2,16)==PERF_INACTIVE&&perf_scene(16,0)==PERF_INACTIVE);
 assert(perf_scene(15,0)==15u);
 assert(perf_frame(&p,now,0));
 now=sample(&p,now,0,30);
 const PerfScene *s=perf_current(&p);
 assert(s&&s->current10==300&&s->minimum10==300&&s->average10==300);
 now=sample(&p,now,0,60);
 assert(s->current10==600&&s->minimum10==300&&s->maximum10==600&&s->average10==450);
 assert(perf_frame(&p,now,perf_scene(2,3)));
 now=sample(&p,now,19,20);
 s=perf_current(&p);
 assert(s&&s->current10==200&&s->minimum10==200&&s->average10==200);
 assert(perf_frame(&p,now,0));
 s=perf_current(&p);
 assert(s&&s->total_frames==90&&s->average10==450);
 /* Ignore partial windows on a scene transition or an inactive pause.
  * No lock-screen frames or idle time contaminate another view. */
 assert(!perf_frame(&p,now+PERF_CLOCK_HZ/2u,0));
 assert(perf_frame(&p,now+PERF_CLOCK_HZ/2u,PERF_INACTIVE));
 assert(!perf_current(&p));
 now+=PERF_CLOCK_HZ;
 assert(perf_frame(&p,now,0));
 now=sample(&p,now,0,15);
 s=perf_current(&p);
 assert(s->current10==150&&s->minimum10==150&&s->average10==350);
 assert(!perf_frame(NULL,now,0));
 perf_init(NULL);
 puts("PASS: per-scene submitted FPS, independent arcade samples, rollover, min/max and weighted average");
 return 0;
}
