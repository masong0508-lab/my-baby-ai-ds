#ifndef MBAI_CONVERSATION_SAVE_H
#define MBAI_CONVERSATION_SAVE_H
#include "conversation.h"

/* Optional learning companion to an ALREADY saved v1 Game. The sequence
 * and complete Game digest bind learning to the exact restored baby.
 * Missing/corrupt companions do not invalidate ordinary progress saves.
 * Loading never changes out on failure; writing never changes Game. */
int conversation_save_load(const char *base,const Game *saved,
                           ConversationProfile *out);
int conversation_save_write(const char *base,const Game *saved,
                            const ConversationProfile *profile);
/* Only the two named learning companions are erased. */
int conversation_save_erase(const char *base);
#endif
