#ifndef MBAI_VOICE_H
#define MBAI_VOICE_H
#include <stdint.h>
#include <stddef.h>
/* Original DS: 8192 Hz signed PCM16, 256 samples (~31 ms) per callback.
 * Only compact features leave the callback. Never store player raw audio. */
#define VOICE_SLOTS 24u
#define VOICE_TEXT 24u
#define VOICE_BINS 12u
#define VOICE_FEATURES 4u
#define VOICE_RAW_FRAMES 56u /* 1.75 s, feature frames only */
#define VOICE_FRAME_SAMPLES 256u
#define VOICE_SAMPLE_RATE 8192u
#define VOICE_MAX_WAIT 80u

enum {VOICE_CALIBRATE=0,VOICE_WAIT,VOICE_CAPTURING,
      VOICE_READY,VOICE_NO_SPEECH,VOICE_TOO_SHORT,VOICE_TOO_LONG};
typedef struct {
 char label[VOICE_TEXT+1u];
 uint8_t samples,next_sample;
 uint8_t models[2][VOICE_BINS][VOICE_FEATURES];
} VoiceEntry;
typedef struct {
 uint8_t used,next;
 VoiceEntry entries[VOICE_SLOTS];
} VoiceProfile;
typedef struct {
 volatile uint8_t state;
 uint8_t frames,tail,level,noise,calibration;
 uint16_t blocks;
 uint8_t measurements[VOICE_RAW_FRAMES][VOICE_FEATURES];
} VoiceCapture;

void voice_profile_reset(VoiceProfile *profile);
int voice_profile_valid(const VoiceProfile *profile);
void voice_capture_reset(VoiceCapture *capture);
void voice_capture_pcm16(VoiceCapture *capture,const int16_t *pcm,unsigned samples);
/* 0 invalid sample/label, 1 new lesson, 2 existing lesson updated,
 * 3 memory full (never silently erase an older taught word). */
int voice_train(VoiceProfile *profile,const char *label,const VoiceCapture *capture);
/* Explicitly managed 24-label memory; all functions operate on the
 * existing unchanged profile/save layout, never raw audio or Game state.
 * voice_find: -1 if absent. voice_cycle: -1 if empty, otherwise next
 * trained slot (wrapping and skipping holes), forward=+1 or backward=-1.
 * voice_forget: 1 only when a valid occupied slot was cleared. */
int voice_find(const VoiceProfile *profile,const char *label);
int voice_cycle(const VoiceProfile *profile,int current,int direction);
int voice_forget(VoiceProfile *profile,unsigned slot);
/* Returns -1 on uncertainty/no trained match. No automatic gameplay action. */
/* A more useful rejection reason for the player; neither function
 * returns a usable label for an ambiguous or distant match. */
typedef enum {
 VOICE_MATCH_INVALID=0,VOICE_MATCH_EMPTY,VOICE_MATCH_DISTANT,
 VOICE_MATCH_AMBIGUOUS,VOICE_MATCH_CONFIRMED
} VoiceMatchStatus;
int voice_recognize_detailed(const VoiceProfile *profile,const VoiceCapture *capture,
                             unsigned *score,VoiceMatchStatus *status);
int voice_recognize(const VoiceProfile *profile,const VoiceCapture *capture,unsigned *score);
const char *voice_label(const VoiceProfile *profile,unsigned slot);
#endif
