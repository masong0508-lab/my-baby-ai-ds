#ifndef MBAI_SFX_H
#define MBAI_SFX_H
/* One DS PSG channel; no PCM bank, heap, or streaming.
 * Driven at the existing game tick rate and safely silenced between cues. */
typedef enum {
 SFX_NONE=0,SFX_SLAP,SFX_KICK,SFX_BOUNCE,SFX_CRACK,
 SFX_UNSEAL,SFX_PULSE,SFX_SCARE
} SfxCue;
void sfx_init(void);
void sfx_play(SfxCue cue);
void sfx_stop(void);
void sfx_tick(void);
void sfx_set_enabled(int enabled);
int sfx_is_enabled(void);
int sfx_is_playing(void);
/* Native hardware adapters live in sfx_nds.c. Host tests mock these. */
void sfx_backend_init(void);
void sfx_backend_apply(unsigned hz,unsigned volume);
#endif
