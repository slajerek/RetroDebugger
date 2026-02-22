#include "C64VicDisplayCanvas.h"
#include "CViewC64VicDisplay.h"
#include "C64SettingsStorage.h"
#include "CViewC64.h"
#include "C64Tools.h"
#include "CViewDisassembly.h"
#include "CViewC64VicEditor.h"
#include "CVicEditorLayerIoAccess.h"
#include "CVicEditorLayerMemoryAccess.h"
#include "CDebugInterfaceC64.h"
#include "DebuggerDefs.h"
#include <algorithm>
#include <cstring>

C64VicDisplayCanvas::C64VicDisplayCanvas(CViewC64VicDisplay *vicDisplay, u8 canvasType, bool isMultiColor, bool isExtendedColor)
{
	this->vicDisplay = vicDisplay;
	this->debugInterface = viewC64->debugInterfaceC64;
	
	this->canvasType = canvasType;
	this->isMultiColor = isMultiColor;
	this->isExtendedColor = isExtendedColor;

	// -1 means no dither mask
	ditherMaskPosX = -1;
	ditherMaskPosY = -1;
}

void C64VicDisplayCanvas::SetViciiState(vicii_cycle_state_t *viciiState)
{
	this->viciiState = viciiState;
}

void C64VicDisplayCanvas::RefreshScreen(vicii_cycle_state_t *viciiState, CImageData *imageDataScreen,
										u8 backgroundColorAlpha, u8 foregroundColorAlpha)
{
	SYS_FatalExit("C64VicDisplayCanvas::RefreshScreen");
}

void C64VicDisplayCanvas::ClearScreen()
{
	LOGError("C64VicDisplayCanvas::ClearScreen: not implemented");
}

void C64VicDisplayCanvas::ClearScreen(u8 charValue, u8 colorValue)
{
	LOGError("C64VicDisplayCanvas::ClearScreen: not implemented");
}

void C64VicDisplayCanvas::RenderGridLines()
{
	RenderCanvasSpecificGridLines();
	RenderVicWriteGridMarks();

	//LOGD("scale=%f", vicDisplay->scale);
	if (vicDisplay->scale > vicDisplay->gridLinesShowValuesZoomLevel)
	{
		RenderCanvasSpecificGridValues();
		RenderCycleInfo();
	}
}

void C64VicDisplayCanvas::RenderCanvasSpecificGridLines()
{
	LOGError("C64VicDisplayCanvas::RenderCanvasSpecificGridLines: not implemented");
}

void C64VicDisplayCanvas::RenderCanvasSpecificGridValues()
{
	LOGError("C64VicDisplayCanvas::RenderCanvasSpecificGridValues: not implemented");
}

// Helper: check if any I/O chip access happened in this cycle
static bool HasIoAccessInCycle(CVicEditorLayerIoAccess *layer, vicii_cycle_state_t *state)
{
	if (layer->showWrites)
	{
		// VIC
		if (state->registerWritten != -1)
		{
			int reg = state->registerWritten;
			IOAccessChipConfig *chip = &layer->chips[IOACCESS_CHIP_VIC];
			if (!(reg >= 0 && reg < chip->numRegisters && !chip->registerEnabled[reg]))
				return true;
		}
		// CIA1
		if (state->cia1RegisterWritten != -1)
		{
			int reg = state->cia1RegisterWritten;
			IOAccessChipConfig *chip = &layer->chips[IOACCESS_CHIP_CIA1];
			if (!(reg >= 0 && reg < chip->numRegisters && !chip->registerEnabled[reg]))
				return true;
		}
		// CIA2
		if (state->cia2RegisterWritten != -1)
		{
			int reg = state->cia2RegisterWritten;
			IOAccessChipConfig *chip = &layer->chips[IOACCESS_CHIP_CIA2];
			if (!(reg >= 0 && reg < chip->numRegisters && !chip->registerEnabled[reg]))
				return true;
		}
		// SID
		if (state->sidRegisterWritten != -1)
		{
			int reg = state->sidRegisterWritten;
			IOAccessChipConfig *chip = &layer->chips[IOACCESS_CHIP_SID];
			if (!(reg >= 0 && reg < chip->numRegisters && !chip->registerEnabled[reg]))
				return true;
		}
	}

	if (layer->showReads)
	{
		if (state->registerRead != -1)
		{
			int reg = state->registerRead;
			IOAccessChipConfig *chip = &layer->chips[IOACCESS_CHIP_VIC];
			if (!(reg >= 0 && reg < chip->numRegisters && !chip->registerEnabled[reg]))
				return true;
		}
		if (state->cia1RegisterRead != -1)
		{
			int reg = state->cia1RegisterRead;
			IOAccessChipConfig *chip = &layer->chips[IOACCESS_CHIP_CIA1];
			if (!(reg >= 0 && reg < chip->numRegisters && !chip->registerEnabled[reg]))
				return true;
		}
		if (state->cia2RegisterRead != -1)
		{
			int reg = state->cia2RegisterRead;
			IOAccessChipConfig *chip = &layer->chips[IOACCESS_CHIP_CIA2];
			if (!(reg >= 0 && reg < chip->numRegisters && !chip->registerEnabled[reg]))
				return true;
		}
		if (state->sidRegisterRead != -1)
		{
			int reg = state->sidRegisterRead;
			IOAccessChipConfig *chip = &layer->chips[IOACCESS_CHIP_SID];
			if (!(reg >= 0 && reg < chip->numRegisters && !chip->registerEnabled[reg]))
				return true;
		}
	}

	return false;
}

void C64VicDisplayCanvas::RenderVicWriteGridMarks()
{
	if (c64SettingsVicStateRecordingMode != C64D_VICII_RECORD_MODE_EVERY_CYCLE)
		return;

	CVicEditorLayerIoAccess *layerIoAccess = (viewC64->viewVicEditor) ? viewC64->viewVicEditor->layerIoAccess : NULL;
	CVicEditorLayerMemoryAccess *layerMemAccess = (viewC64->viewVicEditor) ? viewC64->viewVicEditor->layerMemoryAccess : NULL;

	bool hasIoLayer = layerIoAccess && layerIoAccess->isVisible;
	bool hasMemLayer = layerMemAccess && layerMemAccess->isVisible;
	if (!hasIoLayer && !hasMemLayer)
		return;

	bool fullInstructionMode = (c64SettingsAccessMarkMode == 1);

	// Mark array for full-instruction mode: adjacent-extension instead of global PC matching
	static const int TOTAL_CYCLES = 312 * 63;
	bool marks[TOTAL_CYCLES];

	if (fullInstructionMode)
	{
		memset(marks, 0, sizeof(marks));

		// Phase 1: mark direct access cycles
		for (int i = 0; i < TOTAL_CYCLES; i++)
		{
			int line = i / 63;
			int cycle = i % 63;
			vicii_cycle_state_t *state = c64d_get_vicii_state_for_raster_cycle(line, cycle);
			if (!state)
				continue;

			if (hasIoLayer && HasIoAccessInCycle(layerIoAccess, state))
			{
				marks[i] = true;
				continue;
			}
			if (hasMemLayer && state->memAccessAddr != -1)
			{
				bool matchRW = (state->memAccessIsWrite && layerMemAccess->showWrites) ||
							   (!state->memAccessIsWrite && layerMemAccess->showReads);
				if (matchRW && layerMemAccess->FindEntryForAddress((uint16_t)state->memAccessAddr) >= 0)
					marks[i] = true;
			}
		}

		// Phase 2: extend forward — propagate to next cycle if same PC
		for (int i = 0; i < TOTAL_CYCLES - 1; i++)
		{
			if (!marks[i])
				continue;
			int nextI = i + 1;
			if (marks[nextI])
				continue;
			vicii_cycle_state_t *cur = c64d_get_vicii_state_for_raster_cycle(i / 63, i % 63);
			vicii_cycle_state_t *nxt = c64d_get_vicii_state_for_raster_cycle(nextI / 63, nextI % 63);
			if (nxt->pc == cur->pc)
				marks[nextI] = true;
		}

		// Phase 3: extend backward — propagate to prev cycle if same PC
		for (int i = TOTAL_CYCLES - 1; i > 0; i--)
		{
			if (!marks[i])
				continue;
			int prevI = i - 1;
			if (marks[prevI])
				continue;
			vicii_cycle_state_t *cur = c64d_get_vicii_state_for_raster_cycle(i / 63, i % 63);
			vicii_cycle_state_t *prv = c64d_get_vicii_state_for_raster_cycle(prevI / 63, prevI % 63);
			if (prv->pc == cur->pc)
				marks[prevI] = true;
		}
	}

	float sfX = vicDisplay->rasterScaleFactorX;
	float sfY = vicDisplay->rasterScaleFactorY;
	float baseX = vicDisplay->displayPosX + vicDisplay->rasterCrossOffsetX;
	float baseY = vicDisplay->displayPosY + vicDisplay->rasterCrossOffsetY;
	float cellW = sfX * 8.0f;
	float cellH = sfY;

	for (int vicLine = 0; vicLine < 312; vicLine++)
	{
		float cy = baseY + (float)(vicLine - 0x33) * sfY;
		if (cy + cellH < 0 || cy > vicDisplay->sizeY)
			continue;

		for (int vicCycle = 0; vicCycle < 63; vicCycle++)
		{
			float cx = baseX + (float)(vicCycle * 8 - 0x88) * sfX;
			if (cx + cellW < 0 || cx > vicDisplay->sizeX)
				continue;

			bool hasAccess = false;

			if (fullInstructionMode)
			{
				int idx = vicLine * 63 + vicCycle;
				hasAccess = marks[idx];
			}
			else
			{
				vicii_cycle_state_t *state = c64d_get_vicii_state_for_raster_cycle(vicLine, vicCycle);
				if (!state)
					continue;

				if (hasIoLayer && HasIoAccessInCycle(layerIoAccess, state))
					hasAccess = true;
				if (!hasAccess && hasMemLayer && state->memAccessAddr != -1)
				{
					bool matchRW = (state->memAccessIsWrite && layerMemAccess->showWrites) ||
								   (!state->memAccessIsWrite && layerMemAccess->showReads);
					if (matchRW && layerMemAccess->FindEntryForAddress((uint16_t)state->memAccessAddr) >= 0)
						hasAccess = true;
				}
			}

			if (hasAccess)
			{
				BlitRectangle(cx, cy, vicDisplay->posZ, cellW, cellH,
							  0.0f, 0.0f, 0.0f, 0.35f);
			}
		}
	}
}

void C64VicDisplayCanvas::RenderCycleInfo()
{
	if (c64SettingsVicStateRecordingMode != C64D_VICII_RECORD_MODE_EVERY_CYCLE)
	{
		float hintFs = 8.0f;
		float hintX = vicDisplay->displayPosWithScrollX + vicDisplay->rasterCrossOffsetX;
		float hintY = vicDisplay->displayPosWithScrollY + vicDisplay->rasterCrossOffsetY;
		viewC64->fontDisassembly->BlitTextColor("Set VIC recording to every cycle",
												hintX, hintY, vicDisplay->posZ, hintFs,
												1.0f, 1.0f, 0.5f, 1.0f);
		return;
	}

	float fs = 8.0f;
	float fs2 = fs * 4.0f;
	char pcBuf[8];
	char mnBuf[32];
	char combinedBuf[48];
	char writeBuf[16];
	char readBuf[16];

	// Iterate all VIC raster lines (0..311) and cycles (0..62)
	// Convert VIC coords to screen coords relative to inner display origin:
	//   inner display starts at VIC line 0x33, pixel 0x88 (cycle 0x11)
	float sfX = vicDisplay->rasterScaleFactorX;
	float sfY = vicDisplay->rasterScaleFactorY;
	float baseX = vicDisplay->displayPosX + vicDisplay->rasterCrossOffsetX;
	float baseY = vicDisplay->displayPosY + vicDisplay->rasterCrossOffsetY;

	CVicEditorLayerIoAccess *layerIoAccess = (viewC64->viewVicEditor) ? viewC64->viewVicEditor->layerIoAccess : NULL;
	CVicEditorLayerMemoryAccess *layerMemAccess = (viewC64->viewVicEditor) ? viewC64->viewVicEditor->layerMemoryAccess : NULL;

	for (int vicLine = 0; vicLine < 312; vicLine++)
	{
		float cy = baseY + (float)(vicLine - 0x33) * sfY;

		if (cy + fs * 4.0f < 0 || cy > vicDisplay->sizeY)
			continue;

		for (int vicCycle = 0; vicCycle < 63; vicCycle++)
		{
			float cx = baseX + (float)(vicCycle * 8 - 0x88) * sfX;

			if (cx >= -fs2 && cx < vicDisplay->sizeX)
			{
				vicii_cycle_state_t *state = c64d_get_vicii_state_for_raster_cycle(vicLine, vicCycle);

				if (state)
				{
					// ASM mnemonic with PC address
					u16 pc = state->pc;
					u8 op = debugInterface->GetByteC64ForCycleState(pc, state->memory0001, state->exrom, state->game);
					u8 lo = debugInterface->GetByteC64ForCycleState(pc + 1, state->memory0001, state->exrom, state->game);
					u8 hi = debugInterface->GetByteC64ForCycleState(pc + 2, state->memory0001, state->exrom, state->game);

					sprintfHexCode16WithoutZeroEnding(pcBuf, pc);
					pcBuf[4] = ' ';
					pcBuf[5] = 0;

					viewC64->viewC64Disassembly->MnemonicWithArgumentToStr(pc, op, lo, hi, mnBuf);

					strcpy(combinedBuf, pcBuf);
					strcat(combinedBuf, mnBuf);

					viewC64->fontDisassembly->BlitText(combinedBuf, cx, cy + fs, vicDisplay->posZ, fs);

					float nextInfoY = cy + fs * 2.0f;

					// Helper: render an I/O write/read indicator for any chip
					struct IoAccess {
						int chipIdx;
						int reg;
						u8 value;
						bool isWrite;
					};

					IoAccess accesses[8];
					int numAccesses = 0;

					// Collect write accesses
					if (layerIoAccess && layerIoAccess->showWrites)
					{
						if (state->registerWritten != -1)
							accesses[numAccesses++] = { IOACCESS_CHIP_VIC, state->registerWritten, state->regs[state->registerWritten & 0x3F], true };
						if (state->cia1RegisterWritten != -1)
							accesses[numAccesses++] = { IOACCESS_CHIP_CIA1, state->cia1RegisterWritten, state->cia1WriteValue, true };
						if (state->cia2RegisterWritten != -1)
							accesses[numAccesses++] = { IOACCESS_CHIP_CIA2, state->cia2RegisterWritten, state->cia2WriteValue, true };
						if (state->sidRegisterWritten != -1)
							accesses[numAccesses++] = { IOACCESS_CHIP_SID, state->sidRegisterWritten, state->sidWriteValue, true };
					}

					// Collect read accesses
					if (layerIoAccess && layerIoAccess->showReads)
					{
						if (state->registerRead != -1)
							accesses[numAccesses++] = { IOACCESS_CHIP_VIC, state->registerRead, state->regs[state->registerRead & 0x3F], false };
						if (state->cia1RegisterRead != -1)
							accesses[numAccesses++] = { IOACCESS_CHIP_CIA1, state->cia1RegisterRead, state->cia1ReadValue, false };
						if (state->cia2RegisterRead != -1)
							accesses[numAccesses++] = { IOACCESS_CHIP_CIA2, state->cia2RegisterRead, state->cia2ReadValue, false };
						if (state->sidRegisterRead != -1)
							accesses[numAccesses++] = { IOACCESS_CHIP_SID, state->sidRegisterRead, state->sidReadValue, false };
					}

					for (int a = 0; a < numAccesses; a++)
					{
						IOAccessChipConfig *chip = &layerIoAccess->chips[accesses[a].chipIdx];
						int reg = accesses[a].reg;
						char ioBuf[16];

						if (accesses[a].isWrite)
							sprintf(ioBuf, "%s%02x=%02x", chip->addrPrefix, reg, accesses[a].value);
						else
							sprintf(ioBuf, "%s%02x %02x", chip->addrPrefix, reg, accesses[a].value);

						// Convert addr prefix to lowercase for display
						for (char *p = ioBuf; *p && *p != '='; p++) { if (*p >= 'A' && *p <= 'F') *p += 32; }

						bool barDrawn = layerIoAccess->isVisible
										&& reg >= 0 && reg < chip->numRegisters
										&& chip->registerEnabled[reg];

						if (barDrawn)
						{
							viewC64->fontDisassembly->BlitText(ioBuf, cx, nextInfoY, vicDisplay->posZ, fs);
						}
						else
						{
							float colorR = 1.0f, colorG = 0.0f, colorB = 0.0f;
							if (reg >= 0 && reg < chip->numRegisters)
							{
								colorR = chip->registerColors[reg][0];
								colorG = chip->registerColors[reg][1];
								colorB = chip->registerColors[reg][2];
							}
							viewC64->fontDisassembly->BlitTextColor(ioBuf, cx, nextInfoY, vicDisplay->posZ, fs,
																	colorR, colorG, colorB, 1.0f);
						}
						nextInfoY += fs;
					}

					// Memory access info
					if (layerMemAccess && state->memAccessAddr != -1)
					{
						bool matchRW = (state->memAccessIsWrite && layerMemAccess->showWrites) ||
									   (!state->memAccessIsWrite && layerMemAccess->showReads);
						if (matchRW)
						{
							int entryIdx = layerMemAccess->FindEntryForAddress((uint16_t)state->memAccessAddr);
							if (entryIdx >= 0)
							{
								MemAccessWatchEntry *entry = &layerMemAccess->watchEntries[entryIdx];
								char memBuf[24];
								if (state->memAccessIsWrite)
									sprintf(memBuf, "%04x=%02x", state->memAccessAddr, state->memAccessValue);
								else
								{
									bool isRom = CVicEditorLayerMemoryAccess::IsRomAtAddress(state, (uint16_t)state->memAccessAddr);
									if (isRom)
										sprintf(memBuf, "%04x %02x ROM", state->memAccessAddr, state->memAccessValue);
									else
										sprintf(memBuf, "%04x %02x", state->memAccessAddr, state->memAccessValue);
								}

								bool barDrawn = layerMemAccess->isVisible;
								if (barDrawn)
								{
									viewC64->fontDisassembly->BlitText(memBuf, cx, nextInfoY, vicDisplay->posZ, fs);
								}
								else
								{
									viewC64->fontDisassembly->BlitTextColor(memBuf, cx, nextInfoY, vicDisplay->posZ, fs,
																			entry->color[0], entry->color[1], entry->color[2], 1.0f);
								}
								nextInfoY += fs;
							}
						}
					}
				}
			}
		}
	}
}

u8 C64VicDisplayCanvas::GetColorAtPixel(int x, int y)
{
	LOGError("C64VicDisplayCanvas::GetColorAtPixel: not implemented");
	return 0;
}

u8 C64VicDisplayCanvas::PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	LOGError("C64VicDisplayCanvas::PutColorAtPixel: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 C64VicDisplayCanvas::PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	LOGError("C64VicDisplayCanvas::PaintDither: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 C64VicDisplayCanvas::Paint(bool forceColorReplace, bool isDither, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	if (isDither)
	{
		return this->PaintDither(forceColorReplace, x, y, colorLMB, colorRMB, colorSource, charValue);
	}
	else
	{
		this->ClearDitherMask();
		
		return this->PutColorAtPixel(forceColorReplace, x, y, colorLMB, colorRMB, colorSource, charValue);
	}

}

void C64VicDisplayCanvas::ClearDitherMask()
{
	ditherMaskPosX = -1;
	ditherMaskPosY = -1;
}

u8 C64VicDisplayCanvas::ConvertFrom(CImageData *imageData)
{
	LOGError("C64VicDisplayCanvas::ConvertFrom: not implemented");
	return PAINT_RESULT_ERROR;
}

struct colorPairLess_t {	bool operator()(const C64ColorsHistogramElement *a, const C64ColorsHistogramElement *b) const
	{
		return a->num > b->num;
	}
} colorPairLess;

// finds background color (the color that has highest number of appearances)
std::vector<C64ColorsHistogramElement *> *C64VicDisplayCanvas::GetSortedColorsHistogram(CImageData *imageData)
{
	int histogram[16] = { 0 };
	
	for (int x = 0; x < imageData->width; x++)
	{
		for (int y = 0; y < imageData->height; y++)
		{
			int color = imageData->GetPixelResultByte(x, y);
			histogram[color]++;
		}
	}
	
	std::vector<C64ColorsHistogramElement *> *colors = new std::vector<C64ColorsHistogramElement *>();
	for (int i = 0; i < 16; i++)
	{
		C64ColorsHistogramElement *pair = new C64ColorsHistogramElement(i, histogram[i]);
		colors->push_back(pair);
	}
	
	std::sort(colors->begin(), colors->end(), colorPairLess);
	
//	LOGD("... sorted:");
//	for (std::vector<C64ColorPair *>::iterator it = colors->begin(); it != colors->end(); it++)
//	{
//		C64ColorPair *pair = *it;
//		LOGD("  %d %d", pair->num, pair->color);
//	}
	
	return colors;
}

void C64VicDisplayCanvas::DeleteColorsHistogram(std::vector<C64ColorsHistogramElement *> *colors)
{
	while(!colors->empty())
	{
		C64ColorsHistogramElement *pair = colors->back();
		colors->pop_back();
		
		delete pair;
	}
}

// reduces color space to C64 colors only (nearest)
CImageData *C64VicDisplayCanvas::ReducePalette(CImageData *imageData, CViewC64VicDisplay *vicDisplay)
{
	CImageData *imageReducedPalette = new CImageData(imageData->width, imageData->height, IMG_TYPE_GRAYSCALE);
	imageReducedPalette->AllocImage(false, true);
	
	for (int x = 0; x < imageData->width; x++)
	{
		for (int y = 0; y < imageData->height; y++)
		{
			u8 r, g, b, a;
			
			imageData->GetPixelResultRGBA(x, y, &r, &g, &b, &a);
			
			u8 color = FindC64Color(r, g, b, vicDisplay->debugInterface);
			
			imageReducedPalette->SetPixelResultByte(x, y, color);
		}
	}
	
	return imageReducedPalette;
}
