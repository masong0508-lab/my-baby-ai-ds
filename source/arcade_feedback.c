#include <string.h>
#include "arcade_feedback.h"
void arcade_feedback_reset(ArcadeFeedback *feedback){
 if(feedback)memset(feedback,0,sizeof(*feedback));
}
static void snapshot(ArcadeFeedback *feedback,const Arcade *arcade){
 if(arcade->round>feedback->round)feedback->round=arcade->round;
 feedback->mistakes=arcade->mistakes;
 feedback->stage=arcade->stage;
 feedback->phase=arcade->phase;
 feedback->taped=arcade->taped;
}
void arcade_feedback_begin(ArcadeFeedback *feedback,const Arcade *arcade){
 if(!feedback)return;
 arcade_feedback_reset(feedback);
 if(!arcade||!arcade->active||arcade->finished||arcade->id>=MBAI_GAMES)
  return;
 feedback->tracking=1;feedback->id=arcade->id;
 snapshot(feedback,arcade);
}
static ArcadeFeedbackEvent emit(ArcadeFeedback *feedback,
                                ArcadeFeedbackEvent event){
 feedback->event=(uint8_t)event;
 feedback->remaining=ARCADE_FEEDBACK_TICKS;
 return event;
}
ArcadeFeedbackEvent arcade_feedback_observe(ArcadeFeedback *feedback,
                                           const Arcade *arcade){
 ArcadeFeedbackEvent event=ARCADE_FEEDBACK_NONE;
 if(!feedback||!arcade)return event;
 if(arcade->id>=MBAI_GAMES){arcade_feedback_reset(feedback);return event;}
 if(!feedback->tracking||feedback->id!=arcade->id){
  /* A new session needs a baseline, never an inferred reward. Keep a
   * finished result visible if this is merely another observation. */
  if(arcade->active||feedback->id!=arcade->id)
   arcade_feedback_begin(feedback,arcade);
  return event;
 }
 if(!arcade->active){
  feedback->tracking=0;
  if(arcade->finished==1)return emit(feedback,ARCADE_FEEDBACK_WIN);
  if(arcade->finished==2)return emit(feedback,ARCADE_FEEDBACK_LOSS);
  arcade_feedback_reset(feedback); /* Menu exit is not a loss. */
  return event;
 }
 if(arcade->mistakes>feedback->mistakes)event=ARCADE_FEEDBACK_MISS;
 else if(arcade->round>feedback->round)event=ARCADE_FEEDBACK_PROGRESS;
 else if(arcade->id==4&&feedback->phase&&arcade->phase&&
         arcade->stage>feedback->stage)event=ARCADE_FEEDBACK_PROGRESS;
 else if(arcade->id==14&&arcade->stage>feedback->stage)
  event=ARCADE_FEEDBACK_PROGRESS;
 else if(arcade->id==1&&feedback->stage==3&&arcade->stage==3&&
         arcade->taped>feedback->taped)event=ARCADE_FEEDBACK_PROGRESS;
 /* Stage means a chosen lane/posture or a new question in other games;
  * progress/hold/elapsed also change during continuous animation. None
  * of those changes alone is a successful player action. */
 snapshot(feedback,arcade);
 return event==ARCADE_FEEDBACK_NONE?event:emit(feedback,event);
}
int arcade_feedback_tick(ArcadeFeedback *feedback){
 if(!feedback||!feedback->remaining)return 0;
 feedback->remaining--;
 if(feedback->remaining)return 0;
 feedback->event=ARCADE_FEEDBACK_NONE;
 return 1;
}
ArcadeFeedbackEvent arcade_feedback_current(const ArcadeFeedback *feedback){
 if(!feedback||!feedback->remaining||
    feedback->event>ARCADE_FEEDBACK_LOSS)return ARCADE_FEEDBACK_NONE;
 return (ArcadeFeedbackEvent)feedback->event;
}
