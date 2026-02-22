#ifndef _VICEDITORLAYERMEMORYACCESS_H_
#define _VICEDITORLAYERMEMORYACCESS_H_

#include "SYS_Defs.h"
#include "CVicEditorLayer.h"
#include "CSlrImage.h"
#include <vector>

#define MEM_ACCESS_MAX_ENTRIES 64

struct MemAccessWatchEntry
{
	uint16_t addrStart;
	uint16_t addrEnd;
	float color[3];
	bool enabled;
	char label[32];
};

class CVicEditorLayerMemoryAccess : public CVicEditorLayer
{
public:
	CVicEditorLayerMemoryAccess(CViewC64VicEditor *vicEditor);
	~CVicEditorLayerMemoryAccess();

	virtual void RenderMain(vicii_cycle_state_t *viciiState);
	virtual void RenderPreview(vicii_cycle_state_t *viciiState);

	CImageData *imageData;
	CSlrImage *image;

	std::vector<MemAccessWatchEntry> watchEntries;
	float layerAlpha;
	bool showWrites;
	bool showReads;

	void RebuildWatchTable();
	int FindEntryForAddress(uint16_t addr);
	void UpdateBitmapFromCycles(int regionStartX, int regionStartY,
								int regionWidth, int regionHeight);
	void RefreshImage();

	void LoadFromConfig();
	void SaveToConfig();

	static bool IsRomAtAddress(vicii_cycle_state_t *state, uint16_t addr);

	// Generate a color using golden angle for the Nth entry
	static void GenerateGoldenAngleColor(int index, float *outR, float *outG, float *outB);
};

#endif
