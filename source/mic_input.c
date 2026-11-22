#include "mic_input.h"
#include <nds.h>
/* The ARM7 microphone owns these two 512-byte 32-byte-aligned buffers.
 * The ARM9 invalidates just the completed half before sampling it. */
static uint8_t mic_pcm[VOICE_FRAME_SAMPLES*2u*2u]
 __attribute__((aligned(32)));
static VoiceCapture *active_capture;
static void mic_received(void *data,int bytes){
 if(!active_capture||!data||bytes!=(int)(VOICE_FRAME_SAMPLES*2u))return;
 DC_InvalidateRange(data,(u32)bytes);
 voice_capture_pcm16(active_capture,(const int16_t *)data,VOICE_FRAME_SAMPLES);
}
int mic_input_start(VoiceCapture *capture){
 if(!capture||active_capture)return 0;
 voice_capture_reset(capture);
 /* Low 8192 Hz capture limits ARM7 timer/touch contention on DS Lite. */
 active_capture=capture;
 if(!soundMicRecord(mic_pcm,sizeof(mic_pcm),MicFormat_12Bit,
                    VOICE_SAMPLE_RATE,mic_received)){
  active_capture=0;return 0;
 }
 return 1;
}
void mic_input_stop(void){
 if(active_capture){
  active_capture=0;
  soundMicOff();
 }
}
int mic_input_is_active(void){return active_capture!=0;}
