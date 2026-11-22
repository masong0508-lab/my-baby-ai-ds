#ifndef BABY_NARRATIVE_LM_H
#define BABY_NARRATIVE_LM_H

#include <stddef.h>
#include <stdint.h>

/* Original, offline fictional word generator. No Game pointer, file I/O,
 * allocation, shared RNG or mutable learned weights exist in this module. */
enum NarrativeLMSpeaker {
 NARRATIVE_MONITOR = 0,
 NARRATIVE_BABY = 1
};
enum NarrativeLMTopic {
 NARRATIVE_GREETING = 0,
 NARRATIVE_WORLD = 1,
 NARRATIVE_SELF = 2,
 NARRATIVE_PLAY = 3,
 NARRATIVE_THREAT = 4,
 NARRATIVE_CALM = 5,
 NARRATIVE_DREAM = 6,
 NARRATIVE_UNKNOWN = 7
};

#define NARRATIVE_LM_OUTPUT_CAP 160u
#define NARRATIVE_LM_MAX_CHARS 144u
#define NARRATIVE_LM_MAX_TOKENS 24u

/* Write at most min(cap-1,144) bytes and a terminator. Returns the number
 * of complete word tokens written (punctuation is not counted). cap=0,
 * cap=1 or a null output returns zero. Invalid speaker becomes MONITOR;
 * invalid topic becomes UNKNOWN; stage above three is clamped to three.
 * A caller-supplied seed owns all variation, so exact input replays exactly.
 * A small cap can shorten an answer; no partial word is ever appended.
 * Generated flavor is not an observation or an instruction to change Game.
 */
unsigned narrative_lm_generate(char *out, size_t cap, unsigned speaker,
                               unsigned topic, unsigned stage, uint32_t seed);

/* Compiled footprint and architecture diagnostics, not measured DS FPS. */
size_t narrative_lm_rom_bytes(void);
unsigned narrative_lm_vocabulary_size(void);
unsigned narrative_lm_parameter_count(void);

#endif
