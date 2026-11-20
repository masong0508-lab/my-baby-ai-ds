#ifndef MBAI_UI_H
#define MBAI_UI_H
#include <stdint.h>
#include "game.h"
/* Bottom-screen native bitmap UI: no heap, no asset files, no extra framebuffer. */
void ui_init(void *vram);
void ui_draw(const Game *s,const Arcade *a,int view,int selected,
             int ending,int reset_step,int can_save,const char *message);
int ui_nursery_touch(int x,int y);
int ui_home_navigation(int x,int y);
int ui_arcade_select(int x,int y);
int ui_arcade_touch(int x,int y);
int ui_diaper_touch(int x,int y);
void ui_touch_observe(int down,int x,int y,uint32_t count);
void ui_touch_diag_update(void);
int ui_arcade_navigation(int x,int y); /* 1 play, 2 buy, 3 back */
#endif
