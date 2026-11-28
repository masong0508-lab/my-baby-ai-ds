#include "perf.h"
#include <limits.h>
#include <stddef.h>
#include <string.h>

static uint16_t rate10(uint64_t frames,uint64_t ticks){
 uint64_t result;
 if(!ticks)return 0;
 result=(frames*((uint64_t)PERF_CLOCK_HZ*10u)+ticks/2u)/ticks;
 return (uint16_t)(result>9999u?9999u:result);
}
void perf_init(PerfTracker *p){
 if(!p)return;
 memset(p,0,sizeof(*p));
 p->scene=PERF_INACTIVE;
}
unsigned perf_scene(unsigned view,unsigned cabinet){
 if(view==16u)return PERF_OPTIONS_SCENE;
 if(view>=PERF_VIEW_COUNT)return PERF_INACTIVE;
 if(view==2u)return cabinet<16u?PERF_VIEW_COUNT+cabinet:PERF_INACTIVE;
 return view;
}
const PerfScene *perf_current(const PerfTracker *p){
 return p&&p->scene<PERF_SCENE_COUNT?&p->scenes[p->scene]:NULL;
}
int perf_frame(PerfTracker *p,uint32_t now,unsigned scene){
 uint32_t elapsed;
 PerfScene *s;
 if(!p)return 0;
 if(scene>=PERF_SCENE_COUNT)scene=PERF_INACTIVE;
 if(scene!=p->scene){
  p->scene=scene;p->last_tick=now;p->window_ticks=0;p->window_frames=0;
  return 1;
 }
 if(scene==PERF_INACTIVE)return 0;
 elapsed=now-p->last_tick; /* unsigned wraps at ~128 seconds */
 p->last_tick=now;
 /* A long sleep is not a completed gameplay interval. Reset an
  * interrupted sample instead of attributing suspend time to a scene. */
 if(elapsed>PERF_CLOCK_HZ*30u){
  p->window_ticks=0;p->window_frames=0;
  return 0;
 }
 if(!elapsed)return 0;
 p->window_ticks+=elapsed;
 p->window_frames++;
 if(p->window_ticks<PERF_CLOCK_HZ)return 0;
 s=&p->scenes[scene];
 s->current10=rate10(p->window_frames,p->window_ticks);
 if(!s->total_frames||s->current10<s->minimum10)s->minimum10=s->current10;
 if(s->current10>s->maximum10)s->maximum10=s->current10;
 /* This can occur only after years of continuous gameplay at 60 Hz.
  * Decay both totals together to keep their weighted ratio bounded. */
 if(UINT32_MAX-s->total_frames<p->window_frames){
  s->total_frames/=2u;s->total_ticks/=2u;
 }
 s->total_frames+=p->window_frames;
 s->total_ticks+=p->window_ticks;
 s->average10=rate10(s->total_frames,s->total_ticks);
 p->window_ticks=0;p->window_frames=0;
 return 1;
}
