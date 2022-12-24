#include "SYS_Main.h"
#include "C64SIDFrequencies.h"
#include "FUN_IntervalTree.h"
#include <float.h>

// SID frequencies, based on 440Hz A-4  (should it be based on 435Hz?)
#define NUM_SID_FREQUENCIES		98
static const sid_frequency_t sidFrequencies[NUM_SID_FREQUENCIES] =
{
	{ "   ",	 0,	 	0.0	,	0 },
	{ "C-0",	 0,		16.4	,	0x0115 },
	{ "C#0",	 1,		17.3	,	0x0125 },
	{ "D-0",	 2,		18.4	,	0x0137 },
	{ "D#0",	 3,		19.4	,	0x0149 },
	{ "E-0",	 4,		20.6	,	0x015D },
	{ "F-0",	 5,		21.8	,	0x0172 },
	{ "F#0",	 6,		23.1	,	0x0188 },
	{ "G-0",	 7,		24.5	,	0x019F },
	{ "G#0",	 8,		26.0	,	0x01B8 },
	{ "A-0",	 9,		27.5	,	0x01D2 },
	{ "A#0",	10,		29.1	,	0x01EE },
	{ "B-0",	11,		30.9	,	0x020B },
	{ "C-1",	12,		32.7	,	0x022A },
	{ "C#1",	13,		34.6	,	0x024B },
	{ "D-1",	14,		36.7	,	0x026E },
	{ "D#1",	15,		38.9	,	0x0293 },
	{ "E-1",	16,		41.2	,	0x02BA },
	{ "F-1",	17,		43.7	,	0x02E3 },
	{ "F#1",	18,		46.3	,	0x030F },
	{ "G-1",	19,		49.0	,	0x033E },
	{ "G#1",	20,		51.9	,	0x036F },
	{ "A-1",	21,		55.0	,	0x03A4 },
	{ "A#1",	22,		58.3	,	0x03DB },
	{ "B-1",	23,		61.7	,	0x0416 },
	{ "C-2",	24,		65.4	,	0x0454 },
	{ "C#2",	25,		69.3	,	0x0496 },
	{ "D-2",	26,		73.4	,	0x04DC },
	{ "D#2",	27,		77.8	,	0x0526 },
	{ "E-2",	28,		82.4	,	0x0574 },
	{ "F-2",	29,		87.3	,	0x05C7 },
	{ "F#2",	30,		92.5	,	0x061F },
	{ "G-2",	31,		98.0	,	0x067C },
	{ "G#2",	32,		103.8	,	0x06DF },
	{ "A-2",	33,		110.0	,	0x0747 },
	{ "A#2",	34,		116.5	,	0x07B6 },
	{ "B-2",	35,		123.5	,	0x082C },
	{ "C-3",	36,		130.8	,	0x08A8 },
	{ "C#3",	37,		138.6	,	0x092C },
	{ "D-3",	38,		146.8	,	0x09B7 },
	{ "D#3",	39,		155.6	,	0x0A4B },
	{ "E-3",	40,		164.8	,	0x0AE8 },
	{ "F-3",	41,		174.6	,	0x0B8E },
	{ "F#3",	42,		185.0	,	0x0C3E },
	{ "G-3",	43,		196.0	,	0x0CF8 },
	{ "G#3",	44,		207.7	,	0x0DBE },
	{ "A-3",	45,		220.0	,	0x0E8F },
	{ "A#3",	46,		233.1	,	0x0F6C },
	{ "B-3",	47,		246.9	,	0x1057 },
	{ "C-4",	48,		261.6	,	0x1150 },
	{ "C#4",	49,		277.2	,	0x1258 },
	{ "D-4",	50,		293.7	,	0x136F },
	{ "D#4",	51,		311.1	,	0x1496 },
	{ "E-4",	52,		329.6	,	0x15D0 },
	{ "F-4",	53,		349.2	,	0x171C },
	{ "F#4",	54,		370.0	,	0x187C },
	{ "G-4",	55,		392.0	,	0x19F0 },
	{ "G#4",	56,		415.3	,	0x1B7B },
	{ "A-4",	57,		440.0	,	0x1D1E },
	{ "A#4",	58,		466.2	,	0x1ED9 },
	{ "B-4",	59,		493.9	,	0x20AE },
	{ "C-5",	60,		523.3	,	0x22A0 },
	{ "C#5",	61,		554.4	,	0x24AF },
	{ "D-5",	62,		587.3	,	0x26DD },
	{ "D#5",	63,		622.3	,	0x292D },
	{ "E-5",	64,		659.3	,	0x2BA0 },
	{ "F-5",	65,		698.5	,	0x2E38 },
	{ "F#5",	66,		740.0	,	0x30F8 },
	{ "G-5",	67,		784.0	,	0x33E1 },
	{ "G#5",	68,		830.6	,	0x36F6 },
	{ "A-5",	69,		880.0	,	0x3A3B },
	{ "A#5",	70,		932.3	,	0x3DB2 },
	{ "B-5",	71,		987.8	,	0x415C },
	{ "C-6",	72,		1046.5	,	0x4540 },
	{ "C#6",	73,		1108.8	,	0x495E },
	{ "D-6",	74,		1174.7	,	0x4DBB },
	{ "D#6",	75,		1244.5	,	0x525A },
	{ "E-6",	76,		1318.5	,	0x573F },
	{ "F-6",	77,		1396.9	,	0x5C6F },
	{ "F#6",	78,		1480.0	,	0x61EF },
	{ "G-6",	79,		1568.0	,	0x67C2 },
	{ "G#6",	80,		1661.2	,	0x6DED },
	{ "A-6",	81,		1760.0	,	0x7476 },
	{ "A#6",	82,		1864.7	,	0x7B63 },
	{ "B-6",	83,		1975.5	,	0x82B9 },
	{ "C-7",	84,		2093.0	,	0x8A7F },
	{ "C#7",	85,		2217.5	,	0x92BC },
	{ "D-7",	86,		2349.3	,	0x9B75 },
	{ "D#7",	87,		2489.0	,	0xA4B4 },
	{ "E-7",	88,		2637.0	,	0xAE7F },
	{ "F-7",	89,		2793.8	,	0xB8DF },
	{ "F#7",	90,		2960.0	,	0xC3DE },
	{ "G-7",	91,		3136.0	,	0xCF84 },
	{ "G#7",	92,		3322.4	,	0xDBD9 },
	{ "A-7",	93,		3520.0	,	0xE8ED },
	{ "A#7",	94,		3729.3	,	0xF6C6 },
	{ "B-7",	95,		3951.0	,	0xFFFF },
	{ "   ",    96,		   0.0	,	0x10000 },
	
};

static const sid_frequency_t *sidFrequenciesTable[0x10000];

void SID_FrequenciesInit()
{
	LOGD("SID_FrequenciesInit");
	
	// getting SID frequency is called 3x for each SID in frame, we can use interval tree which is fast
	// but faster will be just to keep 64kB lookup table as it is not that much

	// TODO: update / change values to allow de-tuning (select middle between notes)

	int currentFreqIndex = 0;
	int thisFreqVal = sidFrequencies[currentFreqIndex  ].sidValue;
	int nextFreqVal = sidFrequencies[currentFreqIndex+1].sidValue;
	int midFreqVal  = nextFreqVal; ///(int) ((float)(thisFreqVal+nextFreqVal)*0.75f);

	for (int i = 0; i < 0x10000; i++)
	{
		if (i < midFreqVal)
		{
			sidFrequenciesTable[i] = &(sidFrequencies[currentFreqIndex  ]);
		}
		else
		{
			sidFrequenciesTable[i] = &(sidFrequencies[currentFreqIndex+1]);
		}

		if (i == nextFreqVal)
		{
			thisFreqVal = nextFreqVal;
			currentFreqIndex++;
			nextFreqVal = sidFrequencies[currentFreqIndex+1].sidValue;
			midFreqVal  = nextFreqVal; //(int) ((float)(thisFreqVal+nextFreqVal)*0.75f);
		}
	}
}

const sid_frequency_t *SidFrequencyToNote(u16 sidValue)
{
	return sidFrequenciesTable[sidValue];
}

const sid_frequency_t *SidNoteToFrequency(u16 sidNote)
{
	if (sidNote >= 0 && sidNote < NUM_SID_FREQUENCIES-1)
	{
		//LOGD("sidNote=%d", sidNote);
		return &(sidFrequencies[sidNote+1]);
	}
	
	return &(sidFrequencies[0]);
}

const sid_frequency_t *FrequencyToSidFrequency(float freq)
{
	// TODO: optimize this
	
	// select nearest frequency
	float minFreqDist = FLT_MAX;
	int minIndex = -1;
	for (int i = 0; i < NUM_SID_FREQUENCIES; i++)
	{
		float freqDist = fabs(sidFrequencies[i].frequency - freq);
		
		if (freqDist < minFreqDist)
		{
			minIndex = i;
			minFreqDist = freqDist;
		}
	}
	
//	if (minIndex < 0)
//		return NULL;
	
	return &(sidFrequencies[minIndex]);
}
