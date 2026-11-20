#ifndef MBAI_GAME_H
#define MBAI_GAME_H
#include <stdint.h>
#define MBAI_GAMES 16
#define MBAI_ROUNDS 6
typedef enum { FEED=0, CHAT=1, PLAY=2, NAP=3 } Care;
typedef enum { END_OK=0, END_BOTTLE=1, END_CEO=2, END_REBOOT=3, END_INTERN=4 } Ending;
typedef struct {
 uint32_t sequence, rng, days, wins, losses;
 uint16_t dollars, favors, unlocked, ending_mask, bad_endings;
 uint8_t hunger, mood, energy, chaos, corruption, safe_visuals;
} Game;
typedef struct {
 uint8_t active, id, round, mistakes, direction, reverse, finished;
 uint16_t timer;
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
void game_arcade_start(Game *s,Arcade *a,int id);
int game_arcade_expected(const Arcade *a);
void game_arcade_input(Game *s,Arcade *a,int dir);
void game_arcade_tick(Game *s,Arcade *a);
#endif
