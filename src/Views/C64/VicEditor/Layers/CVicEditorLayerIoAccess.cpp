#include "CVicEditorLayerIoAccess.h"
#include "CViewC64VicEditor.h"
#include "CViewC64.h"
#include "CViewC64VicDisplay.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceVice.h"
#include "VID_ImageBinding.h"
#include "CConfigStorageHjson.h"
#include "C64SettingsStorage.h"
#include <cstring>

#define VICII_NORMAL_BORDERS 0
#define VICII_FULL_BORDERS   1
#define VICII_DEBUG_BORDERS  2
#define VICII_NO_BORDERS     3

CVicEditorLayerIoAccess::CVicEditorLayerIoAccess(CViewC64VicEditor *vicEditor)
: CVicEditorLayer(vicEditor, "I/O Access")
{
	this->isVisible = false;

	// Setup chip configs
	chips[IOACCESS_CHIP_VIC].chipName = "VIC";
	chips[IOACCESS_CHIP_VIC].addrPrefix = "D0";
	chips[IOACCESS_CHIP_VIC].numRegisters = VIC_NUM_REGISTERS;
	chips[IOACCESS_CHIP_VIC].sectionOpen = true;

	chips[IOACCESS_CHIP_CIA1].chipName = "CIA1";
	chips[IOACCESS_CHIP_CIA1].addrPrefix = "DC";
	chips[IOACCESS_CHIP_CIA1].numRegisters = CIA_NUM_REGISTERS;
	chips[IOACCESS_CHIP_CIA1].sectionOpen = false;

	chips[IOACCESS_CHIP_CIA2].chipName = "CIA2";
	chips[IOACCESS_CHIP_CIA2].addrPrefix = "DD";
	chips[IOACCESS_CHIP_CIA2].numRegisters = CIA_NUM_REGISTERS;
	chips[IOACCESS_CHIP_CIA2].sectionOpen = false;

	chips[IOACCESS_CHIP_SID].chipName = "SID";
	chips[IOACCESS_CHIP_SID].addrPrefix = "D4";
	chips[IOACCESS_CHIP_SID].numRegisters = SID_NUM_REGISTERS;
	chips[IOACCESS_CHIP_SID].sectionOpen = false;

	InitDefaultColors();

	imageData = new CImageData(512, 512, IMG_TYPE_RGBA);
	imageData->AllocImage(false, true);

	image = new CSlrImage(true, false);
	image->LoadImageForRebinding(imageData, RESOURCE_PRIORITY_STATIC);
	VID_PostImageBinding(image, NULL, BINDING_MODE_DONT_FREE_IMAGEDATA);
}

void CVicEditorLayerIoAccess::InitChipDefaultColors(int chipIndex, float hueOffset)
{
	IOAccessChipConfig *chip = &chips[chipIndex];
	for (int i = 0; i < chip->numRegisters; i++)
	{
		chip->registerEnabled[i] = true;
		float hue = fmodf(i * 137.508f + hueOffset, 360.0f);
		float sat = 0.50f;
		float val = 0.85f;

		float c = val * sat;
		float h = hue / 60.0f;
		float x = c * (1.0f - fabsf(fmodf(h, 2.0f) - 1.0f));
		float m = val - c;

		float r, g, b;
		if      (h < 1.0f) { r = c; g = x; b = 0; }
		else if (h < 2.0f) { r = x; g = c; b = 0; }
		else if (h < 3.0f) { r = 0; g = c; b = x; }
		else if (h < 4.0f) { r = 0; g = x; b = c; }
		else if (h < 5.0f) { r = x; g = 0; b = c; }
		else               { r = c; g = 0; b = x; }

		chip->registerColors[i][0] = r + m;
		chip->registerColors[i][1] = g + m;
		chip->registerColors[i][2] = b + m;
	}
	// Clear unused slots
	for (int i = chip->numRegisters; i < IOACCESS_MAX_REGISTERS; i++)
	{
		chip->registerEnabled[i] = false;
		chip->registerColors[i][0] = 0.5f;
		chip->registerColors[i][1] = 0.5f;
		chip->registerColors[i][2] = 0.5f;
	}
}

void CVicEditorLayerIoAccess::InitDefaultColors()
{
	layerAlpha = 200.0f / 255.0f;
	showWrites = true;
	showReads = false;

	// VIC: original hue distribution
	InitChipDefaultColors(IOACCESS_CHIP_VIC, 0.0f);

	// Override with distinct vivid colors for key VIC registers
	IOAccessChipConfig *vic = &chips[IOACCESS_CHIP_VIC];
	vic->registerColors[0x11][0] = 1.0f; vic->registerColors[0x11][1] = 1.0f; vic->registerColors[0x11][2] = 0.0f;
	vic->registerColors[0x12][0] = 0.0f; vic->registerColors[0x12][1] = 1.0f; vic->registerColors[0x12][2] = 1.0f;
	vic->registerColors[0x16][0] = 1.0f; vic->registerColors[0x16][1] = 0.0f; vic->registerColors[0x16][2] = 1.0f;
	vic->registerColors[0x18][0] = 0.0f; vic->registerColors[0x18][1] = 1.0f; vic->registerColors[0x18][2] = 0.0f;
	vic->registerColors[0x19][0] = 1.0f; vic->registerColors[0x19][1] = 0.5f; vic->registerColors[0x19][2] = 0.0f;
	vic->registerColors[0x20][0] = 0.4f; vic->registerColors[0x20][1] = 0.6f; vic->registerColors[0x20][2] = 1.0f;
	vic->registerColors[0x21][0] = 0.5f; vic->registerColors[0x21][1] = 1.0f; vic->registerColors[0x21][2] = 0.5f;

	// CIA1: warm hues (offset 90)
	InitChipDefaultColors(IOACCESS_CHIP_CIA1, 90.0f);

	// CIA2: cool hues (offset 180)
	InitChipDefaultColors(IOACCESS_CHIP_CIA2, 180.0f);

	// SID: purple hues (offset 270)
	InitChipDefaultColors(IOACCESS_CHIP_SID, 270.0f);
}

void CVicEditorLayerIoAccess::LoadColorsFromConfig()
{
	char key[128];
	viewC64->config->GetFloat("IOAccessLayerAlpha", &layerAlpha, layerAlpha);
	viewC64->config->GetBool("IOAccessShowWrites", &showWrites, showWrites);
	viewC64->config->GetBool("IOAccessShowReads", &showReads, showReads);

	// Backward compatibility: try old VicAccess keys for VIC chip
	viewC64->config->GetFloat("VicAccessLayerAlpha", &layerAlpha, layerAlpha);
	viewC64->config->GetBool("VicAccessShowWrites", &showWrites, showWrites);
	viewC64->config->GetBool("VicAccessShowReads", &showReads, showReads);

	for (int c = 0; c < IOACCESS_NUM_CHIPS; c++)
	{
		IOAccessChipConfig *chip = &chips[c];
		snprintf(key, sizeof(key), "IOAccess.%s.SectionOpen", chip->chipName);
		viewC64->config->GetBool(key, &chip->sectionOpen, chip->sectionOpen);

		for (int i = 0; i < chip->numRegisters; i++)
		{
			if (c == IOACCESS_CHIP_VIC)
			{
				// Try old VicAccess keys first for backward compat
				snprintf(key, sizeof(key), "VicAccessRegColor.%02X.R", i);
				viewC64->config->GetFloat(key, &chip->registerColors[i][0], chip->registerColors[i][0]);
				snprintf(key, sizeof(key), "VicAccessRegColor.%02X.G", i);
				viewC64->config->GetFloat(key, &chip->registerColors[i][1], chip->registerColors[i][1]);
				snprintf(key, sizeof(key), "VicAccessRegColor.%02X.B", i);
				viewC64->config->GetFloat(key, &chip->registerColors[i][2], chip->registerColors[i][2]);
				snprintf(key, sizeof(key), "VicAccessRegEnabled.%02X", i);
				viewC64->config->GetBool(key, &chip->registerEnabled[i], chip->registerEnabled[i]);
			}

			// New keys (override old if present)
			snprintf(key, sizeof(key), "IOAccess.%s.RegColor.%02X.R", chip->chipName, i);
			viewC64->config->GetFloat(key, &chip->registerColors[i][0], chip->registerColors[i][0]);
			snprintf(key, sizeof(key), "IOAccess.%s.RegColor.%02X.G", chip->chipName, i);
			viewC64->config->GetFloat(key, &chip->registerColors[i][1], chip->registerColors[i][1]);
			snprintf(key, sizeof(key), "IOAccess.%s.RegColor.%02X.B", chip->chipName, i);
			viewC64->config->GetFloat(key, &chip->registerColors[i][2], chip->registerColors[i][2]);
			snprintf(key, sizeof(key), "IOAccess.%s.RegEnabled.%02X", chip->chipName, i);
			viewC64->config->GetBool(key, &chip->registerEnabled[i], chip->registerEnabled[i]);
		}
	}
}

void CVicEditorLayerIoAccess::SaveColorsToConfig()
{
	char key[128];
	CConfigStorageHjson *config = viewC64->config;

	config->hjsonRoot["IOAccessLayerAlpha"] = layerAlpha;
	config->hjsonRoot["IOAccessShowWrites"] = showWrites;
	config->hjsonRoot["IOAccessShowReads"] = showReads;

	for (int c = 0; c < IOACCESS_NUM_CHIPS; c++)
	{
		IOAccessChipConfig *chip = &chips[c];
		snprintf(key, sizeof(key), "IOAccess.%s.SectionOpen", chip->chipName);
		config->hjsonRoot[key] = chip->sectionOpen;

		for (int i = 0; i < chip->numRegisters; i++)
		{
			snprintf(key, sizeof(key), "IOAccess.%s.RegColor.%02X.R", chip->chipName, i);
			config->hjsonRoot[key] = chip->registerColors[i][0];
			snprintf(key, sizeof(key), "IOAccess.%s.RegColor.%02X.G", chip->chipName, i);
			config->hjsonRoot[key] = chip->registerColors[i][1];
			snprintf(key, sizeof(key), "IOAccess.%s.RegColor.%02X.B", chip->chipName, i);
			config->hjsonRoot[key] = chip->registerColors[i][2];
			snprintf(key, sizeof(key), "IOAccess.%s.RegEnabled.%02X", chip->chipName, i);
			config->hjsonRoot[key] = chip->registerEnabled[i];
		}
	}

	config->SaveConfig();
}

CVicEditorLayerIoAccess::~CVicEditorLayerIoAccess()
{
}

void CVicEditorLayerIoAccess::RefreshImage()
{
	image->ReBindImage();
}

// Helper: check if a cycle has an I/O access and return chip/reg/color
static bool GetIoAccessColor(vicii_cycle_state_t *state, IOAccessChipConfig *chips,
							 bool showWrites, bool showReads, u8 *outR, u8 *outG, u8 *outB)
{
	int chipIdx = -1;
	int reg = -1;

	if (showWrites)
	{
		if (state->registerWritten != -1)
		{
			chipIdx = IOACCESS_CHIP_VIC; reg = state->registerWritten;
		}
		else if (state->cia1RegisterWritten != -1)
		{
			chipIdx = IOACCESS_CHIP_CIA1; reg = state->cia1RegisterWritten;
		}
		else if (state->cia2RegisterWritten != -1)
		{
			chipIdx = IOACCESS_CHIP_CIA2; reg = state->cia2RegisterWritten;
		}
		else if (state->sidRegisterWritten != -1)
		{
			chipIdx = IOACCESS_CHIP_SID; reg = state->sidRegisterWritten;
		}
	}

	if (chipIdx == -1 && showReads)
	{
		if (state->registerRead != -1)
		{
			chipIdx = IOACCESS_CHIP_VIC; reg = state->registerRead;
		}
		else if (state->cia1RegisterRead != -1)
		{
			chipIdx = IOACCESS_CHIP_CIA1; reg = state->cia1RegisterRead;
		}
		else if (state->cia2RegisterRead != -1)
		{
			chipIdx = IOACCESS_CHIP_CIA2; reg = state->cia2RegisterRead;
		}
		else if (state->sidRegisterRead != -1)
		{
			chipIdx = IOACCESS_CHIP_SID; reg = state->sidRegisterRead;
		}
	}

	if (chipIdx == -1 || reg == -1)
		return false;

	IOAccessChipConfig *chip = &chips[chipIdx];
	if (reg >= 0 && reg < chip->numRegisters && !chip->registerEnabled[reg])
		return false;

	if (reg >= 0 && reg < chip->numRegisters)
	{
		*outR = (u8)(chip->registerColors[reg][0] * 255.0f);
		*outG = (u8)(chip->registerColors[reg][1] * 255.0f);
		*outB = (u8)(chip->registerColors[reg][2] * 255.0f);
	}
	else
	{
		*outR = 255; *outG = 0; *outB = 0;
	}
	return true;
}

void CVicEditorLayerIoAccess::UpdateBitmapFromCycles(int regionStartX, int regionStartY,
												 int regionWidth, int regionHeight)
{
	// Clear the visible region to transparent
	for (int y = 0; y < regionHeight; y++)
	{
		for (int x = 0; x < regionWidth; x++)
		{
			imageData->SetPixelResultRGBA(x, y, 0, 0, 0, 0);
		}
	}

	u8 alphaVal = (u8)(layerAlpha * 255.0f);
	bool fullInstructionMode = (c64SettingsAccessMarkMode == 1);

	// Mark array for full-instruction mode: stores color per cycle
	// Only cycles with direct accesses or adjacent same-PC neighbors get painted
	static const int TOTAL_CYCLES = 312 * 63;
	struct CycleMark { u8 r, g, b; bool marked; };
	CycleMark marks[TOTAL_CYCLES];

	if (fullInstructionMode)
	{
		memset(marks, 0, sizeof(marks));

		// Phase 1: mark direct access cycles
		for (int i = 0; i < TOTAL_CYCLES; i++)
		{
			vicii_cycle_state_t *state = c64d_get_vicii_state_for_raster_cycle(i / 63, i % 63);
			u8 cr, cg, cb;
			if (GetIoAccessColor(state, chips, showWrites, showReads, &cr, &cg, &cb))
			{
				marks[i].r = cr;
				marks[i].g = cg;
				marks[i].b = cb;
				marks[i].marked = true;
			}
		}

		// Phase 2: extend forward — propagate to next cycle if same PC
		for (int i = 0; i < TOTAL_CYCLES - 1; i++)
		{
			if (!marks[i].marked)
				continue;
			int nextI = i + 1;
			if (marks[nextI].marked)
				continue;
			vicii_cycle_state_t *cur = c64d_get_vicii_state_for_raster_cycle(i / 63, i % 63);
			vicii_cycle_state_t *nxt = c64d_get_vicii_state_for_raster_cycle(nextI / 63, nextI % 63);
			if (nxt->pc == cur->pc)
			{
				marks[nextI] = marks[i];
			}
		}

		// Phase 3: extend backward — propagate to prev cycle if same PC
		for (int i = TOTAL_CYCLES - 1; i > 0; i--)
		{
			if (!marks[i].marked)
				continue;
			int prevI = i - 1;
			if (marks[prevI].marked)
				continue;
			vicii_cycle_state_t *cur = c64d_get_vicii_state_for_raster_cycle(i / 63, i % 63);
			vicii_cycle_state_t *prv = c64d_get_vicii_state_for_raster_cycle(prevI / 63, prevI % 63);
			if (prv->pc == cur->pc)
			{
				marks[prevI] = marks[i];
			}
		}
	}

	for (int rasterLine = 0; rasterLine < 312; rasterLine++)
	{
		int imgY = rasterLine - regionStartY;
		if (imgY < 0 || imgY >= regionHeight)
			continue;

		for (int rasterCycle = 0; rasterCycle < 63; rasterCycle++)
		{
			u8 r = 0, g = 0, b = 0;
			bool shouldPaint = false;

			if (fullInstructionMode)
			{
				int idx = rasterLine * 63 + rasterCycle;
				if (marks[idx].marked)
				{
					r = marks[idx].r;
					g = marks[idx].g;
					b = marks[idx].b;
					shouldPaint = true;
				}
			}
			else
			{
				vicii_cycle_state_t *state = c64d_get_vicii_state_for_raster_cycle(rasterLine, rasterCycle);
				shouldPaint = GetIoAccessColor(state, chips, showWrites, showReads, &r, &g, &b);
			}

			if (!shouldPaint)
				continue;

			int fullX = rasterCycle * 8;
			for (int dx = 0; dx < 8; dx++)
			{
				int imgX = fullX + dx - regionStartX;
				if (imgX >= 0 && imgX < regionWidth)
				{
					imageData->SetPixelResultRGBA(imgX, imgY, r, g, b, alphaVal);
				}
			}
		}
	}
}

void CVicEditorLayerIoAccess::RenderMain(vicii_cycle_state_t *viciiState)
{
	if (vicEditor->viewVicDisplay->showDisplayBorderType != VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA
		&& vicEditor->viewVicDisplay->showDisplayBorderType != VIC_DISPLAY_SHOW_BORDER_NONE)
	{
		return;
	}

	CDebugInterfaceVice *debugInterfaceVice = (CDebugInterfaceVice*)vicEditor->debugInterface;
	int borderMode = debugInterfaceVice->GetViciiBorderMode();

	int regionStartX, regionStartY, regionWidth, regionHeight;
	if (borderMode == VICII_NORMAL_BORDERS)
	{
		regionStartX = 104; regionStartY = 16;
		regionWidth = 384; regionHeight = 272;
	}
	else if (borderMode == VICII_FULL_BORDERS)
	{
		regionStartX = 88; regionStartY = 8;
		regionWidth = 408; regionHeight = 293;
	}
	else if (borderMode == VICII_DEBUG_BORDERS)
	{
		regionStartX = 0; regionStartY = 0;
		regionWidth = 504; regionHeight = 312;
	}
	else
	{
		regionStartX = 136; regionStartY = 51;
		regionWidth = 320; regionHeight = 200;
	}

	UpdateBitmapFromCycles(regionStartX, regionStartY, regionWidth, regionHeight);
	RefreshImage();

	float px, py, sx, sy;
	if (borderMode == VICII_NORMAL_BORDERS)
	{
		px = vicEditor->viewVicDisplay->visibleScreenPosX;
		py = vicEditor->viewVicDisplay->visibleScreenPosY;
		sx = vicEditor->viewVicDisplay->visibleScreenSizeX;
		sy = vicEditor->viewVicDisplay->visibleScreenSizeY;
	}
	else if (borderMode == VICII_FULL_BORDERS)
	{
		float ox = -16;
		float oy = 8-16;
		float osx = 408.0f/384.f;
		float osy = 293.0f/272.f;
		px = vicEditor->viewVicDisplay->visibleScreenPosX + ox*vicEditor->viewVicDisplay->rasterScaleFactorX;
		py = vicEditor->viewVicDisplay->visibleScreenPosY + oy*vicEditor->viewVicDisplay->rasterScaleFactorY;
		sx = vicEditor->viewVicDisplay->visibleScreenSizeX * osx;
		sy = vicEditor->viewVicDisplay->visibleScreenSizeY * osy;
	}
	else if (borderMode == VICII_DEBUG_BORDERS)
	{
		float ox = -104;
		float oy = -16;
		float osx = 504.0f/384.f;
		float osy = 312.0f/272.f;
		px = vicEditor->viewVicDisplay->visibleScreenPosX + ox*vicEditor->viewVicDisplay->rasterScaleFactorX;
		py = vicEditor->viewVicDisplay->visibleScreenPosY + oy*vicEditor->viewVicDisplay->rasterScaleFactorY;
		sx = vicEditor->viewVicDisplay->visibleScreenSizeX * osx;
		sy = vicEditor->viewVicDisplay->visibleScreenSizeY * osy;
	}
	else
	{
		float ox = 32;
		float oy = 51-16;
		float osx = 320.0f/384.f;
		float osy = 200.0f/272.f;
		px = vicEditor->viewVicDisplay->visibleScreenPosX + ox*vicEditor->viewVicDisplay->rasterScaleFactorX;
		py = vicEditor->viewVicDisplay->visibleScreenPosY + oy*vicEditor->viewVicDisplay->rasterScaleFactorY;
		sx = vicEditor->viewVicDisplay->visibleScreenSizeX * osx;
		sy = vicEditor->viewVicDisplay->visibleScreenSizeY * osy;
	}

	float texEndX = (float)regionWidth / 512.0f;
	float texEndY = (float)regionHeight / 512.0f;

	Blit(image, px, py, -1, sx, sy,
		 0.0f, 0.0f, texEndX, texEndY);
}

void CVicEditorLayerIoAccess::RenderPreview(vicii_cycle_state_t *viciiState)
{
	RenderMain(viciiState);
}
