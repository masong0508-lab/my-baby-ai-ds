#ifndef MBAI_OBSERVER_H
#define MBAI_OBSERVER_H
#include <stdint.h>
#include "game.h"

#define OBSERVER_PHRASES 4u
#define OBSERVER_PHRASE_CHARS 24u
#define OBSERVER_REPLY_CAP 160u
#define OBSERVER_WINDUP_TICKS 60u
#define OBSERVER_COOLDOWN_TICKS 240u

typedef enum {
 OBSERVER_FOUND=0,OBSERVER_WATCHING,OBSERVER_HOSTILE,OBSERVER_REVEALED
} ObserverStage;
typedef enum {OBSERVER_MONITOR=0,OBSERVER_BABY=1} ObserverSpeaker;
typedef enum {
 OBSERVER_EVENT_NONE=0,OBSERVER_EVENT_FEED,OBSERVER_EVENT_CHAT,
 OBSERVER_EVENT_PLAY,OBSERVER_EVENT_NAP,OBSERVER_EVENT_ARCADE_WIN,
 OBSERVER_EVENT_ARCADE_LOSS,OBSERVER_EVENT_QUIET_REST,
 OBSERVER_EVENT_DAY,OBSERVER_EVENT_SLAP,OBSERVER_EVENT_KICK,
 OBSERVER_EVENT_WINDUP,OBSERVER_EVENT_HIT,OBSERVER_EVENT_BLOCK,
 OBSERVER_EVENT_REPAIR,OBSERVER_EVENT_TEACH,OBSERVER_EVENT_REVEAL
} ObserverEvent;
typedef enum {
 OBSERVER_TICK_NONE=0,OBSERVER_TICK_WINDUP,OBSERVER_TICK_HIT,
 OBSERVER_TICK_BLOCK
} ObserverTick;
typedef struct {
 char text[OBSERVER_PHRASE_CHARS+1u];
 uint8_t confidence;
} ObserverPhrase;
/* Saved only in the optional exact-Game-bound .watch companion.
 * Caretaker integrity is deliberately separate from Game care meters. */
typedef struct {
 uint32_t last_day,last_event_day;
 uint16_t exposure,care[4],arcade_wins,arcade_losses,quiet_rests,days;
 uint16_t slaps,kicks,attacks,hits,blocks,repairs,teachings;
 uint8_t integrity,last_event,last_detail,last_effect;
 uint8_t phrase_count,phrase_next,known_reveal;
 ObserverPhrase phrases[OBSERVER_PHRASES];
} ObserverProfile;
/* Paused outside active CARE/TOYBOX. Session counters are not a transcript.
 * Reset on new baby or successful load; no Game RNG is consumed. */
typedef struct {
 char reply[OBSERVER_REPLY_CAP];
 uint32_t variation,seen_care_rng,seen_wins,seen_losses,seen_quiet_rng;
 uint16_t cooldown,notice_ticks;
 uint8_t windup,notice,focus,speaker,quote_cursor,echo_ready;
} ObserverSession;

void observer_profile_reset(ObserverProfile *profile,const Game *state);
int observer_profile_valid(const ObserverProfile *profile);
void observer_session_reset(ObserverSession *session,const Game *state);
unsigned observer_stage(const ObserverProfile *profile);
const char *observer_stage_name(unsigned stage);

/* Call exactly once after actual game_care. Identical/repeated before/after
 * samples are rejected when identical or immediately replayed. effect
 * stores the real hunger/energy/mood delta. */
int observer_record_care(ObserverProfile *profile,ObserverSession *session,
                         const Game *before,const Game *after,Care action);
/* Public events: completed ARCADE_WIN/LOSS (detail is cabinet id), an
 * actual successful QUIET_REST, DAY after game.days advances, or a
 * successful SLAP/KICK physics input, or REVEAL after actual END_EGG.
 * Windup/hit/block are tick-owned.
 * Successful SLAP/KICK also counters an active windup immediately. */
int observer_record_event(ObserverProfile *profile,ObserverSession *session,
                          const Game *state,ObserverEvent event,
                          unsigned detail);
ObserverTick observer_tick(ObserverProfile *profile,ObserverSession *session,
                           const Game *state,int active,int guard);
const char *observer_notice(const ObserverProfile *profile,
                            const ObserverSession *session);

/* All returned text belongs to session->reply. Generated fictional prose
 * is kept separate from guarded factual needs/status/memory/calculation.
 * TEACH stores data only; SAY/WHAT DID I TEACH quotes that data literally. */
const char *observer_report(const ObserverProfile *profile,
                            ObserverSession *session,const Game *state);
const char *observer_answer(ObserverProfile *profile,ObserverSession *session,
                            const Game *state,unsigned speaker,
                            const char *input);
/* 0 rejected, 1 new phrase, 2 reinforced duplicate, 3 replaced oldest slot. */
int observer_teach(ObserverProfile *profile,const char *text);
#endif
