#ifndef MBAI_GAME_H
#define MBAI_GAME_H
#include <stdint.h>
#define MBAI_GAMES 16
#define MBAI_ROUNDS 6
typedef enum { FEED=0, CHAT=1, PLAY=2, NAP=3 } Care;
typedef enum {
 END_OK=0, END_BOTTLE=1, END_CEO=2, END_REBOOT=3, END_INTERN=4,
 END_GRADUATION=5, END_ARCADE_CHAMP=6, END_GLITCH_TAMER=7, END_SECOND_CHANCE=8
} Ending;
typedef struct {
 uint32_t sequence, rng, days, wins, losses;
 uint16_t dollars, favors, unlocked, ending_mask, bad_endings;
 uint8_t hunger, mood, energy, chaos, corruption, safe_visuals;
} Game;
typedef struct {
 uint8_t active, id, round, mistakes, direction, reverse, finished;
 uint16_t timer;
 /* Per-session state (not serialized in Game saves). */
 int16_t balance;
 int8_t steering;
 uint16_t hold_frames,elapsed;
 uint8_t stage,target;
 /* Diaper stylus gestures: transient state, never written to save files. */
 uint8_t touch_active,progress,taped;
 int16_t touch_start_x,touch_start_y,touch_last_x,touch_last_y;
} Arcade;
extern const char *const game_names[MBAI_GAMES];
void game_init(Game *s);
const char *game_care(Game *s,Care action);
unsigned game_glitch_tier(const Game *s);
int game_unlocked(const Game *s,int id);
int game_unlock(Game *s,int id);
int game_end_day(Game *s);
const char *game_ending_name(int ending);
int game_reset_unlocked(const Game *s);
int game_is_good_ending(int ending);
int game_good_ending_count(const Game *s);
void game_arcade_start(Game *s,Arcade *a,int id);
int game_arcade_expected(const Arcade *a);
void game_arcade_input(Game *s,Arcade *a,int dir);
void game_arcade_steer(Arcade *a,int direction);
/* Continuous stylus input for the four-step illustrated diaper activity. */
int game_arcade_diaper_touch(Game *s,Arcade *a,int touching,int x,int y);
void game_arcade_tick(Game *s,Arcade *a);
#endif
