#ifndef MBAI_MIC_INPUT_H
#define MBAI_MIC_INPUT_H
#include "voice.h"
/* ARM7 double-buffer recording; no continuous listening in the nursery.
 * Returns 0 if a hardware/emulator mic service cannot be started. */
int mic_input_start(VoiceCapture *capture);
void mic_input_stop(void);
int mic_input_is_active(void);
#endif
