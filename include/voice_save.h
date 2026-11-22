#ifndef MBAI_VOICE_SAVE_H
#define MBAI_VOICE_SAVE_H
#include "game.h"
#include "voice.h"
/* Optional exact-v1-Game-bound voice companion. No raw audio retained. */
int voice_save_write(const char *base,const Game *saved,const VoiceProfile *profile);
int voice_save_load(const char *base,const Game *saved,VoiceProfile *out);
int voice_save_erase(const char *base);
#endif
