#include "CVicEditorLayerMemoryAccess.h"
#include "CViewC64VicEditor.h"
#include "CViewC64.h"
#include "CViewC64VicDisplay.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceVice.h"
#include "VID_ImageBinding.h"
#include "CConfigStorageHjson.h"
#include "C64SettingsStorage.h"
#include <cstring>
#include <cmath>
#include <set>

extern "C"
{
#include "ViceWrapper.h"
};

#define VICII_NORMAL_BORDERS 0
#define VICII_FULL_BORDERS   1
#define VICII_DEBUG_BORDERS  2
#define VICII_NO_BORDERS     3

// 32-mode lookup table: mode = (exrom << 4) | (game << 3) | (port & 7)
// Each entry: { $8000, $A000, $C000, $D000, $E000 }
// 0=RAM, 1=BASIC_ROM, 2=KERNAL_ROM, 3=CHAR_ROM, 4=IO, 5=CART_LO, 6=CART_HI, 7=OPEN
static const uint8_t bankingModes[32][5] = {
	// EXROM=0, GAME=0
	{ 0, 0, 0, 0, 0 },  //  0
	{ 0, 0, 0, 0, 0 },  //  1
	{ 0, 0, 0, 3, 2 },  //  2
	{ 5, 6, 0, 3, 2 },  //  3
	{ 0, 0, 0, 0, 0 },  //  4
	{ 0, 0, 0, 4, 0 },  //  5
	{ 0, 0, 0, 4, 2 },  //  6
	{ 5, 6, 0, 4, 2 },  //  7
	// EXROM=0, GAME=1
	{ 0, 0, 0, 0, 0 },  //  8
	{ 0, 0, 0, 0, 0 },  //  9
	{ 0, 0, 0, 3, 2 },  // 10
	{ 5, 1, 0, 3, 2 },  // 11
	{ 0, 0, 0, 0, 0 },  // 12
	{ 0, 0, 0, 4, 0 },  // 13
	{ 0, 0, 0, 4, 2 },  // 14
	{ 5, 1, 0, 4, 2 },  // 15
	// EXROM=1, GAME=0 (Ultimax)
	{ 7, 7, 7, 7, 6 },  // 16
	{ 7, 7, 7, 7, 6 },  // 17
	{ 5, 7, 7, 3, 6 },  // 18
	{ 5, 7, 7, 3, 6 },  // 19
	{ 7, 7, 7, 7, 6 },  // 20
	{ 5, 7, 7, 4, 6 },  // 21
	{ 5, 7, 7, 4, 6 },  // 22
	{ 5, 7, 7, 4, 6 },  // 23
	// EXROM=1, GAME=1 (no cartridge)
	{ 0, 0, 0, 0, 0 },  // 24
	{ 0, 0, 0, 0, 0 },  // 25
	{ 0, 0, 0, 3, 2 },  // 26
	{ 0, 1, 0, 3, 2 },  // 27
	{ 0, 0, 0, 0, 0 },  // 28
	{ 0, 0, 0, 4, 0 },  // 29
	{ 0, 0, 0, 4, 2 },  // 30
	{ 0, 1, 0, 4, 2 },  // 31
};

CVicEditorLayerMemoryAccess::CVicEditorLayerMemoryAccess(CViewC64VicEditor *vicEditor)
: CVicEditorLayer(vicEditor, "Memory Access")
{
	this->isVisible = false;
	this->layerAlpha = 200.0f / 255.0f;
	this->showWrites = true;
	this->showReads = false;

	imageData = new CImageData(512, 512, IMG_TYPE_RGBA);
	imageData->AllocImage(false, true);

	image = new CSlrImage(true, false);
	image->LoadImageForRebinding(imageData, RESOURCE_PRIORITY_STATIC);
	VID_PostImageBinding(image, NULL, BINDING_MODE_DONT_FREE_IMAGEDATA);
}

CVicEditorLayerMemoryAccess::~CVicEditorLayerMemoryAccess()
{
}

void CVicEditorLayerMemoryAccess::GenerateGoldenAngleColor(int index, float *outR, float *outG, float *outB)
{
	float hue = fmodf(index * 137.508f, 360.0f);
	float sat = 0.65f;
	float val = 0.90f;

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

	*outR = r + m;
	*outG = g + m;
	*outB = b + m;
}

void CVicEditorLayerMemoryAccess::RebuildWatchTable()
{
	memset(c64d_mem_access_watch, 0, sizeof(c64d_mem_access_watch));
	for (size_t i = 0; i < watchEntries.size(); i++)
	{
		MemAccessWatchEntry *entry = &watchEntries[i];
		if (!entry->enabled)
			continue;
		for (int addr = entry->addrStart; addr <= entry->addrEnd; addr++)
		{
			c64d_mem_access_watch[addr] = 1;
		}
	}
}

int CVicEditorLayerMemoryAccess::FindEntryForAddress(uint16_t addr)
{
	for (size_t i = 0; i < watchEntries.size(); i++)
	{
		MemAccessWatchEntry *entry = &watchEntries[i];
		if (entry->enabled && addr >= entry->addrStart && addr <= entry->addrEnd)
			return (int)i;
	}
	return -1;
}

bool CVicEditorLayerMemoryAccess::IsRomAtAddress(vicii_cycle_state_t *state, uint16_t addr)
{
	if (addr < 0x8000)
		return false;

	int mode = (state->exrom << 4) | (state->game << 3) | (state->memory0001 & 0x07);

	int region;
	if (addr < 0xA000)
		region = 0;      // $8000-$9FFF
	else if (addr < 0xC000)
		region = 1;      // $A000-$BFFF
	else if (addr < 0xD000)
		region = 2;      // $C000-$CFFF
	else if (addr < 0xE000)
		region = 3;      // $D000-$DFFF
	else
		region = 4;      // $E000-$FFFF

	uint8_t what = bankingModes[mode][region];
	// 0=RAM, 1=BASIC_ROM, 2=KERNAL_ROM, 3=CHAR_ROM, 4=IO, 5=CART_LO, 6=CART_HI, 7=OPEN
	return (what == 1 || what == 2 || what == 3 || what == 5 || what == 6);
}

void CVicEditorLayerMemoryAccess::RefreshImage()
{
	image->ReBindImage();
}

void CVicEditorLayerMemoryAccess::UpdateBitmapFromCycles(int regionStartX, int regionStartY,
														  int regionWidth, int regionHeight)
{
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
			int line = i / 63;
			int cycle = i % 63;
			vicii_cycle_state_t *state = c64d_get_vicii_state_for_raster_cycle(line, cycle);

			if (state->memAccessAddr == -1)
				continue;
			if (state->memAccessIsWrite && !showWrites)
				continue;
			if (!state->memAccessIsWrite && !showReads)
				continue;

			int entryIdx = FindEntryForAddress((uint16_t)state->memAccessAddr);
			if (entryIdx < 0)
				continue;

			MemAccessWatchEntry *entry = &watchEntries[entryIdx];
			marks[i].r = (u8)(entry->color[0] * 255.0f);
			marks[i].g = (u8)(entry->color[1] * 255.0f);
			marks[i].b = (u8)(entry->color[2] * 255.0f);
			marks[i].marked = true;
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
				if (state->memAccessAddr != -1)
				{
					bool passFilter = true;
					if (state->memAccessIsWrite && !showWrites)
						passFilter = false;
					if (!state->memAccessIsWrite && !showReads)
						passFilter = false;

					if (passFilter)
					{
						int entryIdx = FindEntryForAddress((uint16_t)state->memAccessAddr);
						if (entryIdx >= 0)
						{
							MemAccessWatchEntry *entry = &watchEntries[entryIdx];
							r = (u8)(entry->color[0] * 255.0f);
							g = (u8)(entry->color[1] * 255.0f);
							b = (u8)(entry->color[2] * 255.0f);
							shouldPaint = true;
						}
					}
				}
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

void CVicEditorLayerMemoryAccess::RenderMain(vicii_cycle_state_t *viciiState)
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

void CVicEditorLayerMemoryAccess::RenderPreview(vicii_cycle_state_t *viciiState)
{
	RenderMain(viciiState);
}

void CVicEditorLayerMemoryAccess::LoadFromConfig()
{
	char key[128];
	viewC64->config->GetFloat("MemAccess.LayerAlpha", &layerAlpha, layerAlpha);
	viewC64->config->GetBool("MemAccess.ShowWrites", &showWrites, showWrites);
	viewC64->config->GetBool("MemAccess.ShowReads", &showReads, showReads);

	int numEntries = 0;
	viewC64->config->GetInt("MemAccess.NumEntries", &numEntries, 0);

	watchEntries.clear();
	for (int i = 0; i < numEntries && i < MEM_ACCESS_MAX_ENTRIES; i++)
	{
		MemAccessWatchEntry entry;
		int addrStart = 0, addrEnd = 0;

		snprintf(key, sizeof(key), "MemAccess.Entry.%d.AddrStart", i);
		viewC64->config->GetInt(key, &addrStart, 0);
		snprintf(key, sizeof(key), "MemAccess.Entry.%d.AddrEnd", i);
		viewC64->config->GetInt(key, &addrEnd, addrStart);

		entry.addrStart = (uint16_t)addrStart;
		entry.addrEnd = (uint16_t)addrEnd;

		entry.color[0] = 1.0f; entry.color[1] = 0.0f; entry.color[2] = 0.0f;
		snprintf(key, sizeof(key), "MemAccess.Entry.%d.Color.R", i);
		viewC64->config->GetFloat(key, &entry.color[0], entry.color[0]);
		snprintf(key, sizeof(key), "MemAccess.Entry.%d.Color.G", i);
		viewC64->config->GetFloat(key, &entry.color[1], entry.color[1]);
		snprintf(key, sizeof(key), "MemAccess.Entry.%d.Color.B", i);
		viewC64->config->GetFloat(key, &entry.color[2], entry.color[2]);

		entry.enabled = true;
		snprintf(key, sizeof(key), "MemAccess.Entry.%d.Enabled", i);
		viewC64->config->GetBool(key, &entry.enabled, entry.enabled);

		entry.label[0] = '\0';
		snprintf(key, sizeof(key), "MemAccess.Entry.%d.Label", i);
		viewC64->config->GetString(key, entry.label, sizeof(entry.label), "");

		watchEntries.push_back(entry);
	}

	RebuildWatchTable();
}

void CVicEditorLayerMemoryAccess::SaveToConfig()
{
	char key[128];
	CConfigStorageHjson *config = viewC64->config;

	config->hjsonRoot["MemAccess.LayerAlpha"] = layerAlpha;
	config->hjsonRoot["MemAccess.ShowWrites"] = showWrites;
	config->hjsonRoot["MemAccess.ShowReads"] = showReads;
	config->hjsonRoot["MemAccess.NumEntries"] = (int)watchEntries.size();

	for (int i = 0; i < (int)watchEntries.size(); i++)
	{
		MemAccessWatchEntry *entry = &watchEntries[i];

		snprintf(key, sizeof(key), "MemAccess.Entry.%d.AddrStart", i);
		config->hjsonRoot[key] = (int)entry->addrStart;
		snprintf(key, sizeof(key), "MemAccess.Entry.%d.AddrEnd", i);
		config->hjsonRoot[key] = (int)entry->addrEnd;
		snprintf(key, sizeof(key), "MemAccess.Entry.%d.Color.R", i);
		config->hjsonRoot[key] = entry->color[0];
		snprintf(key, sizeof(key), "MemAccess.Entry.%d.Color.G", i);
		config->hjsonRoot[key] = entry->color[1];
		snprintf(key, sizeof(key), "MemAccess.Entry.%d.Color.B", i);
		config->hjsonRoot[key] = entry->color[2];
		snprintf(key, sizeof(key), "MemAccess.Entry.%d.Enabled", i);
		config->hjsonRoot[key] = entry->enabled;
		snprintf(key, sizeof(key), "MemAccess.Entry.%d.Label", i);
		config->hjsonRoot[key] = std::string(entry->label);
	}

	config->SaveConfig();
}
