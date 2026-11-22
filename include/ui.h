#ifndef MBAI_UI_H
#define MBAI_UI_H
#include <stdint.h>
#include "game.h"
#include "arcade_feedback.h"
/* Bottom-screen native bitmap UI: no heap, no asset files, no extra framebuffer. */
void ui_init(void *vram);
void ui_draw(const Game *s,const Arcade *a,int view,int selected,
             int ending,int reset_step,int can_save,const char *message);
/* Optional arcade footer pass AFTER ui_draw; no-op for NONE/invalid.
 * Only y=178..191 changes, preserving puzzle, hitboxes and status rows.
 * Redraw the normal UI when the caller's feedback timer expires. */
void ui_arcade_feedback(ArcadeFeedbackEvent event);
/* Higher-priority footer for a failed save; draw after arcade feedback.
 * Caller retains failure state until a successful write/load/new baby. */
void ui_save_warning(void);
/* Optional learning-profile write failed; learning remains in RAM.
 * Same footer-only pass; caller chooses precedence after drawing. */
void ui_learning_save_warning(void);
/* Explicit title-only SAVE/LOAD touch target. The boot-time storage probe
 * remains opt-in; touching START never initializes FAT. */
int ui_title_save_touch(int x,int y);
int ui_nursery_touch(int x,int y);
/* Nursery speech panel, separate from ROOM and the four care cards. */
int ui_conversation_entry_touch(int x,int y);
/* TALK view 11: 0..5 ask a prompt, 6 BACK, 7 ASK selected; -1 inert.
 * ui_draw's selected argument identifies the highlighted prompt 0..5. */
int ui_conversation_touch(int x,int y);
/* Typed shelter terminal, independently drawn instead of ui_draw(view=12).
 * speaker: 0 WARDEN/MONITOR, nonzero BABY; selected key: 0..29.
 * Key bit 5 selects the numeric page (encoded keys 32..61).
 * Input is clipped to 32 source bytes, reply to 144 source bytes/four rows.
 * No state, input, or reply buffer is modified by these drawing helpers. */
void ui_observer_draw(const Game *s,int can_save,unsigned speaker,
                       const char *input,const char *reply,unsigned key,
                       unsigned stage,unsigned integrity);
/* Two 30-key pages; space is literal, '<' erases, invalid index is NUL. */
char ui_observer_key(unsigned key);
/* 0..29 keyboard; 30 BACK, 31 TARGET, 32 TEACH, 33 SEND,
 * 34 123/ABC page switch; -1 inert. Caller supplies key page bit. */
int ui_observer_touch(int x,int y);
/* CARE/TALK-only visible CPU badge, separate from status readouts. */
int ui_observer_entry_touch(int x,int y);
/* Footer-only WARDEN notice. Caller applies save warnings afterward. */
void ui_observer_alert(const char *text);
void ui_observer_guard_prompt(unsigned integrity);
void ui_observer_save_warning(void);
void ui_voice_save_warning(void);
/* Explicit microphone lab (view 13): no implied voice control, no raw
 * samples in the UI. Touch 1 TRAIN, 2 LISTEN, 3 USE, 4 BACK. */
int ui_mic_touch(int x,int y);
void ui_mic_draw(const Game *s,int can_save,const char *label,
                 const char *status,const char *match,unsigned level,
                 unsigned learned,int listening);
int ui_room_entry_touch(int x,int y);
int ui_room_touch(int x,int y); /* 0 window, 1 lamp, 2 teddy, 3 crib, 4 back, 5 sound, 6 use */
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
/* Live DS geometry submissions, updated only on the optional diagnostics
 * page. Counts are not hardware FPS or RAM measurements. */
void ui_touch_diag_update(unsigned vertices,unsigned polygons);
void ui_virtual_cursor_update(int enabled,int x,int y,int pressed);
int ui_arcade_navigation(int x,int y); /* 1 play, 2 buy, 3 back */
#endif
