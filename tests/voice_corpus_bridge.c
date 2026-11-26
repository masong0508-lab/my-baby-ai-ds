/* Host-only bridge: call the exact game microphone engine from Python.
 * No corpus recordings or host-only code are linked into the DS ROM. */
#include "voice.h"

static VoiceProfile profile;
static VoiceCapture capture;

void bench_reset(void){voice_profile_reset(&profile);}
void bench_begin(void){voice_capture_reset(&capture);}
void bench_push(const int16_t *pcm,unsigned n){
 voice_capture_pcm16(&capture,pcm,n);
}
unsigned bench_state(void){return capture.state;}
unsigned bench_frames(void){return capture.frames;}
int bench_train(const char *label){return voice_train(&profile,label,&capture);}
int bench_match(unsigned *score){return voice_recognize(&profile,&capture,score);}
const char *bench_label(unsigned slot){return voice_label(&profile,slot);}
