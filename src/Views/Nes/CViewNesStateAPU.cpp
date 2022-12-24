#include "EmulatorsConfig.h"
#if defined(RUN_NES)

#include "NstApiMachine.hpp"
#include "NstMachine.hpp"
#include "NstApiEmulator.hpp"
#include "NstCpu.hpp"

#include "CViewNesStateAPU.h"
#include "SYS_Main.h"
#include "RES_ResourceManager.h"
#include "CGuiMain.h"
#include "CDataAdapter.h"
#include "CSlrString.h"
#include "SYS_KeyCodes.h"
#include "CViewC64.h"
#include "CViewMemoryMap.h"
#include "C64Tools.h"
#include "CViewC64Screen.h"
#include "SYS_Threading.h"
#include "CGuiEditHex.h"
#include "VID_ImageBinding.h"
#include "C64SIDFrequencies.h"
#include "CViewWaveform.h"
#include "CWaveformData.h"
#include "CLayoutParameter.h"

CViewNesStateAPU::CViewNesStateAPU(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceNes *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	fontBytes = viewC64->fontDisassembly;
	
	fontSize = 7.0f;
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	showRegistersOnly = false;
	editHex = new CGuiEditHex(this);
	editHex->isCapitalLetters = false;
	editingRegisterValueIndex = -1;

	for (int apuNum = 0; apuNum < MAX_NUM_NES_APUS; apuNum++)
	{
		for (int chanNum = 0; chanNum < 6; chanNum++)
		{
			viewChannelWaveform[apuNum][chanNum] = new CViewWaveform("CViewNesStateAPU::CViewWaveform", 0, 0, 0, 0, 0, debugInterface->nesChannelWaveform[apuNum][chanNum]);
		}
		viewMixWaveform[apuNum] = new CViewWaveform("CViewNesStateAPU::CViewWaveform", 0, 0, 0, 0, 0, debugInterface->nesMixWaveform[apuNum]);
	}
	
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);	
}

void CViewNesStateAPU::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
	UpdateFontSize();
}

void CViewNesStateAPU::UpdateFontSize()
{
	// UX workaround for now...
//	if (viewC64->currentScreenLayoutId != SCREEN_LAYOUT_NES_MEMORY_MAP)
	{
		// APU
		
		float dx = 1.0f;
		float dy = 0.0f;
		
		float wgx = 4.0f;
		float wgy = fontSize*18.9f;
		float wsx = fontSize*16.2f;
		float wsy = fontSize*5.5f;

		float sx = wgx;
		float sy = wgy;
		float px = posX + wgx;
		float py = posY + wgy;
		
		for (int chanNum = 0; chanNum < 6; chanNum++)
		{
			for (int apuNum = 0; apuNum < MAX_NUM_NES_APUS; apuNum++)
			{
				viewChannelWaveform[apuNum][chanNum]->SetPosition(px, py, posZ, wsx, wsy);
			}
			px += wsx + dx;
			sx += wsx + dx;
		}
		
		px += fontSize*1;
		sx += fontSize*1;

		for (int apuNum = 0; apuNum < MAX_NUM_NES_APUS; apuNum++)
		{
			viewMixWaveform[apuNum]->SetPosition(px, py, posZ, wsx, wsy);
		}
		
		sy += wsy;
		
//		CGuiView::SetSize(sx, sy);
	}
	/*
	else
	{
		//
		//
		// memory map
				
		float wsx = fontSize*11.5f;
		float wsy = fontSize*3.7f;

		float dx = 1.0f;

		float px = posX;
		float py = posY + fontSize*7.0f;
		
		for (int chanNum = 0; chanNum < 4; chanNum++)
		{
			for (int apuNum = 0; apuNum < MAX_NUM_NES_APUS; apuNum++)
			{
				nesChannelWaveform[apuNum][chanNum]->SetPosition(px, py, posZ, wsx, wsy);
			}
			px += wsx + dx;
			if (chanNum == 1)
			{
				px = posX;
				py += wsy;
			}
		}
		
		py += wsy + 1.0f;
		px = posX;
		
		for (int apuNum = 0; apuNum < MAX_NUM_NES_APUS; apuNum++)
		{
			nesMixWaveform[apuNum]->SetPosition(px, py, posZ, wsx*2 + dx, wsy);
		}

	}*/

}

void CViewNesStateAPU::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	UpdateFontSize();
}

void CViewNesStateAPU::SetVisible(bool isVisible)
{
	LOGG("CViewNesStateAPU::SetVisible: isVisible=%s", STRBOOL(isVisible));
	CGuiElement::SetVisible(isVisible);
	
	int selectedApuNumber = 0;
	viewC64->debugInterfaceNes->SetApuReceiveChannelsData(selectedApuNumber, isVisible);
}


void CViewNesStateAPU::DoLogic()
{
}

void CViewNesStateAPU::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewNesStateAPU::Render()
{
//	if (debugInterface->GetSettingIsWarpSpeed() == true)
//		return;

	int selectedApuNumber = 0;

	// calc trigger pos, render
	for (int chanNum = 0; chanNum < 6; chanNum++)
	{
		viewChannelWaveform[selectedApuNumber][chanNum]->Render();
	}
	viewMixWaveform[selectedApuNumber]->Render();

	this->RenderState(posX, posY, posZ, fontBytes, fontSize, 1);
}


void CViewNesStateAPU::RenderState(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize, int apuId)
{
	float startX = px;
	float startY = py;
	
	float psx = startX;
	float psy = startY;
	float sx = fontSize*16.2f + 1.0f;
	
	char buf[256] = {0};
	sprintf(buf, "APU");

	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	py += fontSize*0.5f;

	u8 regs[0x20];
	for (int i = 0; i < 0x20; i++)
	{
		regs[i] = viewC64->debugInterfaceNes->GetApuRegister(i);
	}
	
	psy = py;
	
//	if (viewC64->currentScreenLayoutId != SCREEN_LAYOUT_NES_MEMORY_MAP)
	{
		py = psy;
		
		double cpuClockFreq = debugInterfaceNes->GetCpuClockFrequency();

		char *space = "    ";
		
		u16 regOffset = 0;
		
		// PULSE 1
		{
			u16 envelopeDivider = regs[0x00 + regOffset] & 0x0F;
			u16 constVolumeOrEnvelopeFlag = (regs[0x00 + regOffset] >> 4) & 0x01;
			u16 lengthCounterHalt = (regs[0x00 + regOffset] >> 5) & 0x01;
			u16 dutyCycle = (regs[0x00 + regOffset] >> 6) & 0x03;
		
			u16 sweepEnabled = (regs[0x01 + regOffset] >> 7) & 0x01;
			u16 sweepDividerPeriod = (regs[0x01 + regOffset] >> 4) & 0x07;
			u16 sweepNegateFlag = (regs[0x01 + regOffset] >> 3) & 0x01;
			u16 sweepShiftCount = regs[0x01 + regOffset] & 0x07;
			
			u16 pulseTimer = ((regs[0x03 + regOffset] & 0x07) << 8) | regs[0x02 + regOffset];
			
			double freqPulse = 0.0;
			
			if (pulseTimer != 0)
			{
				freqPulse = cpuClockFreq / 16.0 / (((double)pulseTimer) + 1.0);
			}
					
			u16 pulseLen = (regs[0x03 + regOffset] >> 3) & 0x1F;
			
			const sid_frequency_t *freq = FrequencyToSidFrequency(freqPulse);

			sprintf(buf, "PULSE 1    %s", freq->name);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "FREQ  %8.2f", freqPulse);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "TIMER     %4x", pulseTimer);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "DUTY      %4x", dutyCycle);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			
			sprintf(buf, "LCH       %4x", lengthCounterHalt);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "CV/EN     %4x", constVolumeOrEnvelopeFlag);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "ENVDIV    %4x", envelopeDivider);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "LEN       %4x", pulseLen);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "SWEEP      %s", (sweepEnabled == 0x01 ? " ON" : "OFF"));
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "PERIOD    %4x", sweepDividerPeriod);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "NEGATE    %4x", sweepNegateFlag);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "SHIFT     %4x", sweepShiftCount);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		}
	
		// TODO: generalize this
		
		px += sx;
		py = psy;
		
		// PULSE 2
		regOffset = 0x04;
		{
			u16 envelopeDivider = regs[0x00 + regOffset] & 0x0F;
			u16 constVolumeOrEnvelopeFlag = (regs[0x00 + regOffset] >> 4) & 0x01;
			u16 lengthCounterHalt = (regs[0x00 + regOffset] >> 5) & 0x01;
			u16 dutyCycle = (regs[0x00 + regOffset] >> 6) & 0x03;
		
			u16 sweepEnabled = (regs[0x01 + regOffset] >> 7) & 0x01;
			u16 sweepDividerPeriod = (regs[0x01 + regOffset] >> 4) & 0x07;
			u16 sweepNegateFlag = (regs[0x01 + regOffset] >> 3) & 0x01;
			u16 sweepShiftCount = regs[0x01 + regOffset] & 0x07;
			
			u16 pulseTimer = ((regs[0x03 + regOffset] & 0x07) << 8) | regs[0x02 + regOffset];
			
			double freqPulse = 0.0;
			
			if (pulseTimer != 0)
			{
				freqPulse = cpuClockFreq / 16.0 / (((double)pulseTimer) + 1.0);
			}
					
			u16 pulseLen = (regs[0x03 + regOffset] >> 3) & 0x1F;
			
			const sid_frequency_t *freq = FrequencyToSidFrequency(freqPulse);

			sprintf(buf, "PULSE 2    %s", freq->name);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "FREQ  %8.2f", freqPulse);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "TIMER     %4x", pulseTimer);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "DUTY      %4x", dutyCycle);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "LCH       %4x", lengthCounterHalt);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "CV/EN     %4x", constVolumeOrEnvelopeFlag);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "ENVDIV    %4x", envelopeDivider);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "LEN       %4x", pulseLen);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "SWEEP      %s", (sweepEnabled == 0x01 ? " ON" : "OFF"));
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "PERIOD    %4x", sweepDividerPeriod);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "NEGATE    %4x", sweepNegateFlag);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "SHIFT     %4x", sweepShiftCount);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		}

		px += sx;
		py = psy;

		//https://www.mattmontag.com/uncategorized/nes-apu-note-table
		// TRIANGLE
		{
			u16 controlFlag = (regs[0x08] >> 7) & 0x01;
			u16 counterReload = regs[0x08] & 0x7F;
	
			u16 pulseTimer = ((regs[0x0B] & 0x07) << 8) | regs[0x0A];
			u16 pulseLen = (regs[0x0B] >> 3) & 0x1F;

			double freqPulse = 0.0;
			
			if (pulseTimer != 0)
			{
				freqPulse = cpuClockFreq / 32.0 / (((double)pulseTimer) + 1.0);
			}
					
			const sid_frequency_t *freq = FrequencyToSidFrequency(freqPulse);

			sprintf(buf, "TRIANGLE   %s", freq->name);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "FREQ  %8.2f", freqPulse);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "TIMER     %4x", pulseTimer);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "CTRL      %4x", controlFlag);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "RELOAD    %4x", counterReload);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "LEN       %4x", pulseLen);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		}
		
		px += sx;
		py = psy;

		// NOISE
		// https://wiki.nesdev.com/w/index.php/APU_Noise
		{
			u16 lengthCounterHalt = (regs[0x0C] >> 5) & 0x01;
			u16 constVolumeOrEnvelopeFlag = (regs[0x0C] >> 4) & 0x01;
			u16 envelopeDivider = regs[0x0C] & 0x0F;

			u16 period = regs[0x0E] & 0x0F;

			u16 lengthCounterLoad = regs[0x0F] & 0x0F;
			u16 pulseLen = (regs[0x0B] >> 3) & 0x1F;

			const float apuNoiseFreqs[0x10] = {
				4811.2, 2405.6, 1202.8, 601.4, 300.7, 200.5, 150.4, 120.3,
				95.3, 75.8, 50.6, 37.9, 25.3, 18.9, 9.5, 4.7
			};
			
			float freqNoise = apuNoiseFreqs[period];
			const sid_frequency_t *freq = FrequencyToSidFrequency(freqNoise);

			sprintf(buf, "NOISE      %s", freq->name);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "FREQ  %8.2f", freqNoise);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "PERIOD    %4x", period);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "LCH       %4x", lengthCounterHalt);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "CV/EN     %4x", constVolumeOrEnvelopeFlag);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "ENVDIV    %4x", envelopeDivider);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "LEN       %4x", lengthCounterLoad);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		}

		px += sx;
		py = psy;

		// DMC
		// https://wiki.nesdev.com/w/index.php/APU_DMC
		{
			u16 irqFlag = (regs[0x10] >> 7) & 0x01;
			u16 loopFlag = (regs[0x10] >> 6) & 0x01;
			u16 rate = regs[0x10] & 0x0F;

			u16 directLoad = regs[0x11] & 0x7F;
			
			u16 sampleAddr = 0xC000 + regs[0x12] * 0x40;
			
			u16 sampleLen = regs[0x13] * 0x10 + 1;
			
			const int rateNTSC[0x10] = { 428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84,  72,  54 };
			const int ratePAL[0x10]  = { 398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118,  98,  78,  66,  50 };
			
			int rateVal = debugInterfaceNes->IsPal() ? ratePAL[rate] : rateNTSC[rate];
			
			double cpuClockFreq = debugInterfaceNes->GetCpuClockFrequency();
			double freqDMC = cpuClockFreq / (double)rateVal;
			const sid_frequency_t *freq = FrequencyToSidFrequency(freqDMC);
		
			sprintf(buf, "DMC        %s", freq->name);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "FREQ  %8.2f", freqDMC);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "RATE      %4x", rate);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "IRQ FLAG   %s", irqFlag ? "YES" : " NO");
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "LOOP FLAG  %s", loopFlag ? "YES" : " NO");
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "DIRECT      %02x", directLoad);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "SAMPLE    %04x", sampleAddr);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "LEN       %04x", sampleLen);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		}

		px += 2.0f * sx;
		py = psy;

		// STATUS
		// https://wiki.nesdev.com/w/index.php/APU
		{
			// regs are regs written
			u16 dmcActiveFlag = (regs[0x15] >> 4) 	& 0x01;
			u16 noiseFlag = 	(regs[0x15] >> 3) 	& 0x01;
			u16 triangleFlag = 	(regs[0x15] >> 2) 	& 0x01;
			u16 pulse1Flag =	(regs[0x15] >> 1) 	& 0x01;
			u16 pulse2Flag = 	(regs[0x15]) 		& 0x01;


			// TODO: regs read
//			u16 irqFlag = (regs[0x15] >> 7) & 0x01;
//			u16 frameInterruptFlag = (regs[0x15] >> 6) & 0x01;
//			u16 dmcActiveFlag = (regs[0x15] >> 4) 	& 0x01;
//			u16 noiseFlag = 	(regs[0x15] >> 3) 	& 0x01;
//			u16 triangleFlag = 	(regs[0x15] >> 2) 	& 0x01;
//			u16 pulse1Flag =	(regs[0x15] >> 1) 	& 0x01;
//			u16 pulse2Flag = 	(regs[0x15]) 		& 0x01;

			sprintf(buf, "PULSE 1   %s", pulse1Flag ? " ON" : "OFF");
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "PULSE 2   %s", pulse2Flag ? " ON" : "OFF");
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "NOISE     %s", noiseFlag ? " ON" : "OFF");
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "TRIANGLE  %s", triangleFlag ? " ON" : "OFF");
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

			sprintf(buf, "DMC       %s", dmcActiveFlag ? " ON" : "OFF");
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

//			sprintf(buf, "LEN       %04x", sampleLen);
//			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		}

	}
	
//	else
//	{
//		// memory map
//		char *space = "  ";
//
//		sprintf(buf, "AUDF1  %02X%sAUDC1  %02X",
//				POKEY_AUDF[POKEY_CHAN1], space, POKEY_AUDC[POKEY_CHAN1]);
//		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
//		sprintf(buf, "AUDF2  %02X%sAUDC2  %02X",
//				POKEY_AUDF[POKEY_CHAN2], space, POKEY_AUDC[POKEY_CHAN2]);
//		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
//		sprintf(buf, "AUDF3  %02X%sAUDC3  %02X",
//				POKEY_AUDF[POKEY_CHAN3], space, POKEY_AUDC[POKEY_CHAN3]);
//		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
//		sprintf(buf, "AUDF4  %02X%sAUDC4  %02X",
//				POKEY_AUDF[POKEY_CHAN4], space, POKEY_AUDC[POKEY_CHAN4]);
//		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
//		sprintf(buf, "AUDCTL %02X", POKEY_AUDCTL[0]);
//		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
//	}
	
//	psy = py + 0.5f * fontSize;

	py = psy + fontSize * 12.5f;
	
	px = psx;
	
	//	if (showRegistersOnly)
		{
			float fs2 = fontSize;
			
			float plx = px;
			float ply = py;
			for (int i = 0; i < 0x20; i++)
			{
				if (editingRegisterValueIndex == i)
				{
					sprintf(buf, "40%02x", i);
					fontBytes->BlitText(buf, plx, ply, posZ, fs2);
					fontBytes->BlitTextColor(editHex->textWithCursor,
											 plx + fontSize*5.0f, ply, posZ, fontSize, 1.0f, 1.0f, 1.0f, 1.0f);
				}
				else
				{
					u8 v = regs[i];
					sprintf(buf, "40%02x %02x", i, v);
					fontBytes->BlitText(buf, plx, ply, posZ, fs2);
				}
			
				ply += fs2;

				if (i % 0x04 == 0x03)
				{
					ply = py;
					plx += sx;
					
					if (i == 0x13)
					{
						sprintf(buf, "EXT IN");
						fontBytes->BlitText(buf, plx, ply + fontSize * 3.0f, posZ, fs2);

						plx += sx + fontSize;
						ply += fs2 * 3;
						i++;
					}
				}
				
				if (i == 0x015)
					break;
			}
	//		return;
			
			py = ply;
		 }
	//
	py += fontSize;

	
}

bool CViewNesStateAPU::DoTap(float x, float y)
{
	guiMain->LockMutex();
	
	if (editingRegisterValueIndex != -1)
	{
		editHex->FinalizeEntering(MTKEY_ENTER, true);
	}

//	for (int apuNum = 0; apuNum < debugInterface->GetNumSids(); apuNum++)
	int apuNum = 0;
	{
		for (int i = 0; i < 6; i++)
		{
			if (viewChannelWaveform[apuNum][i]->IsInside(x, y))
			{
				viewChannelWaveform[apuNum][i]->waveform->isMuted = !viewChannelWaveform[apuNum][i]->waveform->isMuted;
				debugInterface->UpdateWaveformsMuteStatus();
				guiMain->UnlockMutex();
				return true;
			}
		}

		if (viewMixWaveform[apuNum]->IsInside(x,y))
		{
			viewMixWaveform[apuNum]->waveform->isMuted = !viewMixWaveform[apuNum]->waveform->isMuted;
			debugInterface->UpdateWaveformsMuteStatus();
			guiMain->UnlockMutex();
			return true;
		}
	}
	
//	if (CGuiView::DoTap(x, y))
//	{
//		guiMain->UnlockMutex();
//		return true;
//	}
//
	
	
	// check if tap register
	if (showRegistersOnly)
	{
		/*
		float px = posX;
		
		if (x >= posX && x < posX+190)
		{
			float fs2 = fontSize;
			float sx = fs2 * 9;
			
			float plx = posX;	//+ fontSize * 5.0f
			float plex = posX + fontSize * 7.0f;
			float ply = posY + fontSize;
			for (int i = 0; i < 0x10; i++)
			{
				if (x >= plx && x <= plex
					&& y >= ply && y <= ply+fontSize)
				{
					LOGD("CViewNesStateAPU::DoTap: tapped register %02x", i);
					
					editingRegisterValueIndex = i;

					u8 v = debugInterface->GetCiaRegister(editingCIAIndex, editingRegisterValueIndex);
					editHex->SetValue(v, 2);
					
					guiMain->UnlockMutex();
					return true;
				}
				
				ply += fs2;
				
				if (i % 0x08 == 0x07)
				{
					ply = posY + fontSize;
					plx += sx;
					plex += sx;
				}
			}
		}
		 */
	}
	
	showRegistersOnly = !showRegistersOnly;

	guiMain->UnlockMutex();
	return false;
}

void CViewNesStateAPU::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled)
{
	if (isCancelled)
		return;
	
	/*
	if (editingRegisterValueIndex != -1)
	{
		byte v = editHex->value;
		debugInterface->SetCiaRegister(editingCIAIndex, editingRegisterValueIndex, v);
		
		editHex->SetCursorPos(0);
	}
	 */

}

bool CViewNesStateAPU::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	/*
	if (editingRegisterValueIndex != -1)
	{
		if (keyCode == MTKEY_ARROW_UP)
		{
			if (editingRegisterValueIndex > 0)
			{
				editingRegisterValueIndex--;
				u8 v = debugInterface->GetCiaRegister(editingCIAIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_DOWN)
		{
			if (editingRegisterValueIndex < 0x0F)
			{
				editingRegisterValueIndex++;
				u8 v = debugInterface->GetCiaRegister(editingCIAIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_LEFT)
		{
			if (editHex->cursorPos == 0 && editingRegisterValueIndex > 0x08)
			{
				editingRegisterValueIndex -= 0x08;
				u8 v = debugInterface->GetCiaRegister(editingCIAIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_RIGHT)
		{
			if (editHex->cursorPos == 1 && editingRegisterValueIndex < 0x10-0x08)
			{
				editingRegisterValueIndex += 0x08;
				u8 v = debugInterface->GetCiaRegister(editingCIAIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		editHex->KeyDown(keyCode);
		return true;
	}
	 */
	return false;
}

bool CViewNesStateAPU::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

void CViewNesStateAPU::RenderFocusBorder()
{
//	CGuiView::RenderFocusBorder();
	//
}

float CViewNesStateAPU::GetFrequencyForChannel(int chanNum)
{
	// https://www.mattmontag.com/uncategorized/nes-apu-note-table
	
	u8 regs[0x20];
	for (int i = 0; i < 0x20; i++)
	{
		regs[i] = viewC64->debugInterfaceNes->GetApuRegister(i);
	}
	
	double cpuClockFreq = debugInterfaceNes->GetCpuClockFrequency();

	u16 regOffset = 0;

	// PULSE 1
	if (chanNum == 0)
	{
		u16 pulseTimer = ((regs[0x03 + regOffset] & 0x07) << 8) | regs[0x02 + regOffset];
		
		double freqPulse = 0.0;
		
		if (pulseTimer != 0)
		{
			freqPulse = cpuClockFreq / 16.0 / (((double)pulseTimer) + 1.0);
		}
		
		return freqPulse;
	}
	
	// PULSE 2
	if (chanNum == 1)
	{
		regOffset = 0x04;

		u16 pulseTimer = ((regs[0x03 + regOffset] & 0x07) << 8) | regs[0x02 + regOffset];
		double freqPulse = 0.0;
		
		if (pulseTimer != 0)
		{
			freqPulse = cpuClockFreq / 16.0 / (((double)pulseTimer) + 1.0);
		}

		return freqPulse;
	}
	
	// TRIANGLE
	if (chanNum == 2)
	{
		u16 pulseTimer = ((regs[0x0B] & 0x07) << 8) | regs[0x0A];

		double freqPulse = 0.0;
		
		if (pulseTimer != 0)
		{
			freqPulse = cpuClockFreq / 32.0 / (((double)pulseTimer) + 1.0);
		}
		
		return freqPulse;
	}
	
	if (chanNum == 3)
	{
		// NOISE

		u16 period = regs[0x0E] & 0x0F;

		const float apuNoiseFreqs[0x10] = {
			4811.2, 2405.6, 1202.8, 601.4, 300.7, 200.5, 150.4, 120.3,
			95.3, 75.8, 50.6, 37.9, 25.3, 18.9, 9.5, 4.7
		};
		
		float freqNoise = apuNoiseFreqs[period];
		return freqNoise;
	}

	if (chanNum == 4)
	{
		// DMC
		u16 rate = regs[0x10] & 0x0F;

		const int rateNTSC[0x10] = { 428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84,  72,  54 };
		const int ratePAL[0x10]  = { 398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118,  98,  78,  66,  50 };

		int rateVal = debugInterfaceNes->IsPal() ? ratePAL[rate] : rateNTSC[rate];
		double cpuClockFreq = debugInterfaceNes->GetCpuClockFrequency();
		double freqDMC = cpuClockFreq / (double)rateVal;
		
		if (freqDMC > 4100.0)
			return 0.0f;
		
		return freqDMC;
	}
	
	return 0.0f;
}

extern Nes::Api::Emulator nesEmulator;

bool CViewNesStateAPU::IsChannelActive(int chanNum)
{
	Nes::Core::Machine& machine = nesEmulator;
	
	switch(chanNum)
	{
		case 0:
			return machine.cpu.apu.square[0].CanOutput();
		case 1:
			return machine.cpu.apu.square[1].CanOutput();
		case 2:
			return machine.cpu.apu.triangle.CanOutput();
		case 3:
			return machine.cpu.apu.noise.CanOutput();
			
			// TODO: check if DMC active
//		case 4:
//			return machine.cpu.apu.dmc.
			
	}
	
	return false;
}


#else
// dummy

/*
CViewNesStateAPU::CViewNesStateAPU(float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceNes *debugInterface)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
}

void CViewNesStateAPU::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY) {}
void CViewNesStateAPU::DoLogic() {}
void CViewNesStateAPU::Render() {}
void CViewNesStateAPU::RenderState(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize, int ciaId) {}
bool CViewNesStateAPU::DoTap(float x, float y) { return false; }
void CViewNesStateAPU::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled) {}
bool CViewNesStateAPU::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper) { return false; }
bool CViewNesStateAPU::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper) { return false; }
void CViewNesStateAPU::RenderFocusBorder() {}
void CViewNesStateAPU::SetVisible(bool isVisible) {}
void CViewNesStateAPU::AddWaveformData(int pokeyNumber, int v1, int v2, int v3, int v4, short mix) {}
*/

#endif
