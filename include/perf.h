#ifndef MBAI_PERF_H
#define MBAI_PERF_H
#include <stdint.h>
/* Native DS timer frequency; cpuGetTiming returns bus-clock ticks in
 * both supported runtimes. Statistics count submitted game frames,
 * not host wall-clock speed or independently measured LCD refreshes. */
#define PERF_CLOCK_HZ 33513982u
#define PERF_VIEW_COUNT 16u
#define PERF_OPTIONS_SCENE 32u
#define PERF_SCENE_COUNT 33u
#define PERF_INACTIVE PERF_SCENE_COUNT
typedef struct {
 uint64_t total_ticks;
 uint32_t total_frames;
 uint16_t current10,minimum10,average10,maximum10;
} PerfScene;
typedef struct {
 PerfScene scenes[PERF_SCENE_COUNT];
 uint32_t last_tick,window_ticks,window_frames;
 unsigned scene;
} PerfTracker;
void perf_init(PerfTracker *p);
unsigned perf_scene(unsigned view,unsigned cabinet);
/* One call after each completed frame, using unsigned rollover-safe
 * elapsed timer ticks. A scene switch resets the incomplete window. */
int perf_frame(PerfTracker *p,uint32_t now,unsigned scene);
const PerfScene *perf_current(const PerfTracker *p);
#endif
