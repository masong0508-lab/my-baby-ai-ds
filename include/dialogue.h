#ifndef MBAI_DIALOGUE_H
#define MBAI_DIALOGUE_H
#include "game.h"
/* Six output intents from a tiny, hand-tuned two-layer integer classifier.
 * No runtime training, no internet, no dynamically allocated model. */
const char *dialogue_reply(Game *state,Care action);
int dialogue_intent(const Game *state,Care action);
#endif
