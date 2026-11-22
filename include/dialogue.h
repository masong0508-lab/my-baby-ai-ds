#ifndef MBAI_DIALOGUE_H
#define MBAI_DIALOGUE_H
#include "game.h"
/* Six output intents from a tiny, hand-tuned two-layer integer classifier.
 * No runtime training, no internet, no dynamically allocated model. */
const char *dialogue_reply(Game *state,Care action);
int dialogue_intent(const Game *state,Care action);
/* Explicit, session-only memory; never part of the serialized Game.
 * A small set of distinct recent events avoids duplicate memories when
 * repeatedly using one care button. Ages count interactions, not frames. */
typedef enum {
 DIALOGUE_EVENT_FEED=1, DIALOGUE_EVENT_PLAY, DIALOGUE_EVENT_NAP,
 DIALOGUE_EVENT_ARCADE_WIN, DIALOGUE_EVENT_ARCADE_LOSS,
 DIALOGUE_EVENT_QUIET_REST
} DialogueEvent;
typedef struct {uint8_t kind,age,spoken;} DialogueMemoryEntry;
typedef struct {
 uint32_t day;
 DialogueMemoryEntry recent[4];
 uint8_t count,last_reply;
} DialogueMemory;
void dialogue_memory_reset(DialogueMemory *memory,uint32_t day);
/* Record only completed events. QUIET_REST replaces its preceding NAP. */
void dialogue_memory_event(DialogueMemory *memory,DialogueEvent event,
                           uint32_t day);
/* Call after game_care, passing its reply. No additional RNG or gameplay
 * mutation: healthy CHAT may recall one actual event; all care replies
 * avoid immediately repeating the preceding known authored line. */
const char *dialogue_contextual_reply(DialogueMemory *memory,
                                      const Game *state,Care action,
                                      const char *fallback);
#endif
