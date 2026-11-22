#ifndef MBAI_CONVERSATION_H
#define MBAI_CONVERSATION_H
#include <stdint.h>
#include "game.h"
#define CONVERSATION_STYLE_COUNT 3u
#define CONVERSATION_FEATURE_COUNT 12u
#define CONVERSATION_ADAPTER_LIMIT 12
#define CONVERSATION_FACT_TTL 12u
#define CONVERSATION_HISTORY 8u
typedef enum {
 CONVERSATION_HOW=0,CONVERSATION_NEED,CONVERSATION_REMEMBER,
 CONVERSATION_MORE,CONVERSATION_LIKE,CONVERSATION_ELSE
} ConversationPrompt;
typedef enum {
 CONVERSATION_WARM=0,CONVERSATION_PLAYFUL,CONVERSATION_CORPORATE
} ConversationStyle;
typedef enum {
 CONVERSATION_EVENT_FEED=1,CONVERSATION_EVENT_CHAT,
 CONVERSATION_EVENT_PLAY,CONVERSATION_EVENT_NAP,
 CONVERSATION_EVENT_ARCADE_WIN,CONVERSATION_EVENT_ARCADE_LOSS,
 CONVERSATION_EVENT_QUIET_REST
} ConversationEvent;
typedef enum {
 CONVERSATION_FEEL_OK=0,CONVERSATION_FEEL_HUNGRY,
 CONVERSATION_FEEL_TIRED,CONVERSATION_FEEL_PLAYFUL,
 CONVERSATION_FEEL_GLITCH,CONVERSATION_FEEL_CHAOTIC,
 CONVERSATION_NEED_FOOD,CONVERSATION_NEED_REST,
 CONVERSATION_NEED_COMPANY,CONVERSATION_NEED_QUIET,
 CONVERSATION_NEED_NONE,CONVERSATION_MEMORY_FEED,
 CONVERSATION_MEMORY_CHAT,CONVERSATION_MEMORY_PLAY,
 CONVERSATION_MEMORY_NAP,CONVERSATION_MEMORY_WIN,
 CONVERSATION_MEMORY_LOSS,CONVERSATION_MEMORY_QUIET,
 CONVERSATION_FULL_FEED,CONVERSATION_FULL_NAP,
 CONVERSATION_TIRED_PLAY,CONVERSATION_REPEAT_CARE,
 CONVERSATION_NO_MEMORY,CONVERSATION_IMAGINE_TEDDY,
 CONVERSATION_IMAGINE_DREAMS,CONVERSATION_IMAGINE_WORLD,
 CONVERSATION_ACK_LIKE,CONVERSATION_ACK_RATED,
 CONVERSATION_ASK_FIRST,CONVERSATION_CARE_THANKS,
 CONVERSATION_TOPIC_COUNT
} ConversationTopic;
/* Exactly 40 bytes. Only explicit LIKE/ELSE feedback trains this profile.
 * Root persistence may save it separately; Game's v1 bytes are unchanged. */
typedef struct {
 int8_t adapter[CONVERSATION_STYLE_COUNT][CONVERSATION_FEATURE_COUNT];
 uint16_t likes,changes;
} ConversationProfile;
/* effect is the actual useful care delta, not an inferred preference. */
typedef struct {uint8_t kind,age,detail,effect;} ConversationFact;
typedef struct {
 uint32_t day,variation;
 ConversationFact facts[4];
 uint16_t history[CONVERSATION_HISTORY],last_reply;
 int8_t last_features[CONVERSATION_FEATURE_COUNT];
 uint8_t fact_count,last_action,care_streak,topic,focus_event;
 uint8_t style,can_rate,feedback_used,history_cursor,more_count;
 uint8_t requested_care,request_met,request_age,last_prompt;
} ConversationSession;
/* Original ROM text; explicit newlines divide at most three 36-char rows.
 * Care responses use only entries with at most two rows. */
typedef struct {
 const char *text;
 uint16_t id;
 uint8_t topic,style;
} ConversationReply;
void conversation_profile_reset(ConversationProfile *profile);
int conversation_profile_valid(const ConversationProfile *profile);
void conversation_session_reset(ConversationSession *session,uint32_t day,
                                 uint32_t seed);
void conversation_record_care(ConversationSession *session,const Game *before,
                               const Game *after,Care action);
/* Only completed outcomes and successful quiet rest belong here. detail
 * may be the actual cabinet id; it never alters or unlocks gameplay. */
void conversation_record_event(ConversationSession *session,
                                ConversationEvent event,uint8_t detail,
                                uint32_t day);
const ConversationReply *conversation_respond(ConversationProfile *profile,
                                              ConversationSession *session,
                                              const Game *state,
                                              ConversationPrompt prompt);
/* Call after record_care. FEED/PLAY/NAP only; CHAT keeps its legacy path.
 * No stat mutation or shared RNG advance, including when rating a reply. */
const ConversationReply *conversation_care_response(ConversationProfile *profile,
                                                    ConversationSession *session,
                                                    const Game *state,Care action);
/* Stable catalog inspection for text validation and authored-content tools. */
unsigned conversation_catalog_count(void);
const ConversationReply *conversation_catalog_reply(unsigned id);
#endif
