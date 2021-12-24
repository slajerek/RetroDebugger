#include "CViewWaveform.h"

// TODO: add synchronization like in SidWiz2: https://github.com/Zeinok/SidWiz2F/blob/master/SidWiz/Form1.cs

// waveform views
CViewWaveform::CViewWaveform(float posX, float posY, float posZ, float sizeX, float sizeY, int waveformDataLength)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->waveformDataLength = waveformDataLength;
	this->waveformData = new signed short[waveformDataLength];
	
	this->lineStrip = new CGLLineStrip();
	this->lineStrip->Clear();
	
	isMuted = false;
}

CViewWaveform::~CViewWaveform()
{
	delete [] waveformData;
}

void CViewWaveform::CalculateWaveform()
{
	GenerateLineStrip(this->lineStrip,
					  waveformData, 0, waveformDataLength, this->posX, this->posY, this->posZ, this->sizeX, this->sizeY);
}

void CViewWaveform::Render()
{
	if (!isMuted)
	{
		BlitRectangle(posX, posY, posZ, sizeX, sizeY, 0.5f, 0.0f, 0.0f, 1.0f);
		BlitLineStrip(lineStrip, 0.9f, 0.9f, 0.9f, 1.0f);
	}
	else
	{
		BlitRectangle(posX, posY, posZ, sizeX, sizeY, 0.3f, 0.3f, 0.3f, 1.0f);
		BlitLineStrip(lineStrip, 0.3f, 0.3f, 0.3f, 1.0f);
	}
}

