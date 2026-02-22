#ifndef _VICEDITORLAYERIOACCESS_H_
#define _VICEDITORLAYERIOACCESS_H_

#include "SYS_Defs.h"
#include "CVicEditorLayer.h"
#include "CSlrImage.h"

#define IOACCESS_CHIP_VIC   0
#define IOACCESS_CHIP_CIA1  1
#define IOACCESS_CHIP_CIA2  2
#define IOACCESS_CHIP_SID   3
#define IOACCESS_NUM_CHIPS  4

#define VIC_NUM_REGISTERS   0x2F
#define CIA_NUM_REGISTERS   0x10
#define SID_NUM_REGISTERS   0x1D
#define IOACCESS_MAX_REGISTERS  0x2F   // max across all chips (VIC has the most)

struct IOAccessChipConfig
{
	const char *chipName;       // "VIC", "CIA1", "CIA2", "SID"
	const char *addrPrefix;     // "D0", "DC", "DD", "D4"
	int numRegisters;
	float registerColors[IOACCESS_MAX_REGISTERS][3];
	bool registerEnabled[IOACCESS_MAX_REGISTERS];
	bool sectionOpen;
};

class CVicEditorLayerIoAccess : public CVicEditorLayer
{
public:
	CVicEditorLayerIoAccess(CViewC64VicEditor *vicEditor);
	~CVicEditorLayerIoAccess();

	virtual void RenderMain(vicii_cycle_state_t *viciiState);
	virtual void RenderPreview(vicii_cycle_state_t *viciiState);

	CImageData *imageData;    // 512x512 RGBA
	CSlrImage *image;         // GPU texture

	void UpdateBitmapFromCycles(int regionStartX, int regionStartY,
							   int regionWidth, int regionHeight);
	void RefreshImage();

	IOAccessChipConfig chips[IOACCESS_NUM_CHIPS];
	float layerAlpha;

	bool showWrites;
	bool showReads;

	void InitDefaultColors();
	void InitChipDefaultColors(int chipIndex, float hueOffset);
	void LoadColorsFromConfig();
	void SaveColorsToConfig();
};

#endif
