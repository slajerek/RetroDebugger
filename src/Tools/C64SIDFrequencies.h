#ifndef _C64_SID_FREQUENCIES_H_
#define _C64_SID_FREQUENCIES_H_

#include "SYS_Defs.h"

//
typedef struct sid_frequency_s
{
	const char *name;
	u8 note;
	float frequency;
	int sidValue;
} sid_frequency_t;

void SID_FrequenciesInit();
const sid_frequency_t *SidFrequencyToNote(u16 sidFrequency);
const sid_frequency_t *SidNoteToFrequency(u16 sidNote);
const sid_frequency_t *FrequencyToSidFrequency(float freq);

#endif
