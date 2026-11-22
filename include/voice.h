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
#define VOICE_RAW_FRAMES 32u
#define VOICE_FRAME_SAMPLES 256u
#define VOICE_SAMPLE_RATE 8192u
#define VOICE_MAX_WAIT 80u

enum {VOICE_CALIBRATE=0,VOICE_WAIT,VOICE_CAPTURING,
      VOICE_READY,VOICE_NO_SPEECH,VOICE_TOO_SHORT};
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
/* Returns 0 for rejected input, 1 new lesson/slot, 2 reinforced. */
int voice_train(VoiceProfile *profile,const char *label,const VoiceCapture *capture);
/* Returns -1 on uncertainty/no trained match. No automatic gameplay action. */
int voice_recognize(const VoiceProfile *profile,const VoiceCapture *capture,unsigned *score);
const char *voice_label(const VoiceProfile *profile,unsigned slot);
#endif
