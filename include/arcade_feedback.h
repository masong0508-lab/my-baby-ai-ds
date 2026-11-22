#ifndef MBAI_ARCADE_FEEDBACK_H
#define MBAI_ARCADE_FEEDBACK_H
#include <stdint.h>
#include "game.h"
typedef enum {
 ARCADE_FEEDBACK_NONE=0,ARCADE_FEEDBACK_PROGRESS,ARCADE_FEEDBACK_MISS,
 ARCADE_FEEDBACK_WIN,ARCADE_FEEDBACK_LOSS
} ArcadeFeedbackEvent;
#define ARCADE_FEEDBACK_TICKS 30u
/* Session-only transition observer, separate from Arcade and saved Game.
 * Round keeps a high-water mark so Bottle wobble cannot replay a milestone.
 * No gameplay/RNG mutation, text buffer, heap or global session state. */
typedef struct {
 uint8_t tracking,id,round,mistakes,stage,phase,taped;
 uint8_t event,remaining;
} ArcadeFeedback;
void arcade_feedback_reset(ArcadeFeedback *feedback);
/* Begin AFTER game_arcade_start. An already-finished game emits nothing. */
void arcade_feedback_begin(ArcadeFeedback *feedback,const Arcade *arcade);
/* Observe once after input + simulation, before automatic leave_game.
 * Completed result > mistake > meaningful progress. Reobserving the same
 * state, leaving manually, and ordinary animation ticks emit nothing. */
ArcadeFeedbackEvent arcade_feedback_observe(ArcadeFeedback *feedback,
                                           const Arcade *arcade);
/* Advance once per existing main-loop tick; returns 1 only on expiry so
 * the caller can repaint the ordinary footer without a per-frame redraw. */
int arcade_feedback_tick(ArcadeFeedback *feedback);
ArcadeFeedbackEvent arcade_feedback_current(const ArcadeFeedback *feedback);
#endif
