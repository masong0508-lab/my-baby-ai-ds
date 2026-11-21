#ifndef MBAI_UI_H
#define MBAI_UI_H
#include <stdint.h>
#include "game.h"
/* Bottom-screen native bitmap UI: no heap, no asset files, no extra framebuffer. */
void ui_init(void *vram);
void ui_draw(const Game *s,const Arcade *a,int view,int selected,
             int ending,int reset_step,int can_save,const char *message);
int ui_nursery_touch(int x,int y);
int ui_prank_entry_touch(int x,int y);
int ui_roughhouse_touch(int x,int y); /* 1 slap, 2 kick, 3 back */
/* Film: 1 skip, 2 pause/resume, 3 next act. The readout is tappable. */
int ui_egg_cinematic_touch(int x,int y);
int ui_home_navigation(int x,int y);
int ui_arcade_select_page(int x,int y,int selected);
int ui_arcade_page_touch(int x,int y); /* -1 previous, +1 next */
int ui_album_page_touch(int x,int y); /* -1 previous, +1 next */
int ui_arcade_touch(int x,int y);
int ui_diaper_touch(int x,int y); /* legacy button accessibility */
int ui_rattle_touch(int x,int y);
int ui_kart_touch(int x,int y);
int ui_sock_touch(int x,int y);
int ui_nap_touch(int x,int y);
int ui_crawl_touch(int x,int y);
int ui_cereal_touch(int x,int y);
int ui_blocks_touch(int x,int y);
int ui_audit_touch(int x,int y);
int ui_garden_touch(int x,int y);
int ui_court_touch(int x,int y);
int ui_ceo_touch(int x,int y);
int ui_beatbox_touch(int x,int y);
void ui_touch_observe(int down,int x,int y,uint32_t count);
void ui_touch_diag_update(void);
void ui_virtual_cursor_update(int enabled,int x,int y,int pressed);
int ui_arcade_navigation(int x,int y); /* 1 play, 2 buy, 3 back */
#endif
