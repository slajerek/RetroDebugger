#ifndef _CViewWaveform_h_
#define _CViewWaveform_h_

#include "C64D_Version.h"
#include "SYS_Defs.h"
#include "CGuiView.h"
#include <vector>
#include <list>

class CViewWaveform : public CGuiView
{
public:
	CViewWaveform(float posX, float posY, float posZ, float sizeX, float sizeY, int waveformDataLength);
	~CViewWaveform();

	signed short *waveformData;
	int waveformDataLength;

	CGLLineStrip *lineStrip;
	
	bool isMuted;
		
	void CalculateWaveform();
	void Render();
};


#endif
