#include "EmulatorsConfig.h"
#if defined(RUN_NES)

extern "C" {
#include "sid.h"
}

#include "CViewNesPianoKeyboard.h"
#include "C64Tools.h"
#include "C64SIDFrequencies.h"
#include "CViewC64.h"
#include "CViewC64StateSID.h"
#include "CViewNesStateAPU.h"
//#include "CViewSIDTrackerHistory.h"
#include "CDebugInterface.h"
#include "SYS_KeyCodes.h"
#include "CViewWaveform.h"
#include "CWaveformData.h"

CViewNesPianoKeyboard::CViewNesPianoKeyboard(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CPianoKeyboardCallback *callback)
: CPianoKeyboard(name, posX, posY, posZ, sizeX, sizeY, callback)
{
	for (int chanNum = 0; chanNum < 5; chanNum++)
	{
		prevFreq[chanNum] = 0;
	}
}

void CViewNesPianoKeyboard::DoLogic()
{
//	CPianoKeyboard::DoLogic();
}

void CViewNesPianoKeyboard::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
	CPianoKeyboard::DoLogic();
}

void CViewNesPianoKeyboard::Render()
{
	if (this->doKeysFadeOut == false)
	{
		for (int i = 0; i < pianoKeys.size(); i++)
		{
			CPianoKey *key = pianoKeys[i];
			if (!key->isBlackKey)
			{
				key->cr = key->cg = key->cb = 1.0f;
			}
			else
			{
				key->cr = key->cg = key->cb = 0.0f;
			}
		}
	}
	
	for (int chanNum = 0; chanNum < 5; chanNum++)
	{
		// check if channel is not muted
		if (viewC64->viewNesStateAPU->viewChannelWaveform[0][chanNum]->waveform->isMuted)
			continue;
		
		float freq = viewC64->viewNesStateAPU->GetFrequencyForChannel(chanNum);
				
		bool noteOn = false;
		
		if (this->doKeysFadeOut == true)
		{
			// update also on note changes
			if (freq != prevFreq[chanNum])
			{
				noteOn = true;
				prevFreq[chanNum] = freq;
			}
			
			bool channelActive = viewC64->viewNesStateAPU->IsChannelActive(chanNum);
			if (channelActive)
				noteOn = true;
		}
		else
		{
			noteOn = true;
		}
		
		if (noteOn)
		{
			const sid_frequency_t *frequencyData = FrequencyToSidFrequency(freq);
			
			int note = frequencyData->note;

			if (note >= 0 && note < 96)
			{
				CPianoKey *key = pianoKeys[note];
				
				// TODO: how to do mixing?
				if (chanNum == 0)
				{
					key->cr = 1.0f;
					key->cg = 0.0;
					key->cb = 0.0;
				}
				else if (chanNum == 1)
				{
					key->cr = 0.0f;
					key->cg = 1.0;
					key->cb = 0.0;
				}
				else if (chanNum == 2)
				{
					if (key->isBlackKey)
					{
						key->cr = 0.2f;
						key->cg = 0.2;
						key->cb = 1.0;
					}
					else
					{
						key->cr = 0.0f;
						key->cg = 0.0;
						key->cb = 1.0;
					}
				}
				else if (chanNum == 3)
				{
					key->cr = 1.0f;
					key->cg = 1.0;
					key->cb = 0.0;
				}
				else if (chanNum == 4)
				{
					key->cr = 0.0f;
					key->cg = 1.0;
					key->cb = 1.0;
				}

			}
		}
	}
	CPianoKeyboard::Render();
	
	// fixme!
	CPianoKeyboard::DoLogic();
}

bool CViewNesPianoKeyboard::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("CViewNesPianoKeyboard::KeyDown: keyCode=%d", keyCode);
//	if (keyCode == MTKEY_SPACEBAR || keyCode == MTKEY_ARROW_UP || keyCode == MTKEY_ARROW_DOWN
//		|| keyCode == MTKEY_PAGE_UP || keyCode == MTKEY_PAGE_DOWN)
//
//	{
//		return viewC64->viewC64AllSids->viewTrackerHistory->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
//	}
//
//	if (this->HasFocus()
//		|| viewC64->viewC64AllSids->viewTrackerHistory->HasFocus())
//	{
//		return CPianoKeyboard::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
//	}
	
	return false;
}

bool CViewNesPianoKeyboard::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
//	if (this->HasFocus()
//		|| viewC64->viewC64AllSids->viewTrackerHistory->HasFocus())
//	{
//		return CPianoKeyboard::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
//	}
	
	return false;
}

#endif
