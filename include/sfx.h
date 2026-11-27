#ifndef MBAI_SFX_H
#define MBAI_SFX_H
/* One DS PSG channel; no PCM bank, heap, or streaming.
 * Driven at the existing game tick rate and safely silenced between cues. */
typedef enum {
 SFX_NONE=0,SFX_SLAP,SFX_KICK,SFX_BOUNCE,SFX_CRACK,
 SFX_UNSEAL,SFX_PULSE,SFX_SCARE,
 /* Short original one-channel musical reactions for ordinary play. */
 SFX_FEED,SFX_CHAT,SFX_PLAY,SFX_NAP,SFX_ROOM_SWITCH,SFX_MOBILE,
 SFX_ARCADE_PROGRESS,SFX_ARCADE_MISS,SFX_ARCADE_WIN,SFX_ARCADE_LOSS,
 SFX_LULLABY,SFX_BEAT_BOOM,SFX_BEAT_CLAP,SFX_BEAT_BEEP,SFX_BEAT_POP
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
