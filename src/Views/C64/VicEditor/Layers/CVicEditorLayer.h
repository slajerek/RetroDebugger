#ifndef _VICEDITORLAYER_H_
#define _VICEDITORLAYER_H_

#include "SYS_Defs.h"
#include "ViceWrapper.h"

class CViewC64VicEditor;
class CByteBuffer;
class CDebugInterfaceC64;

class CVicEditorLayer
{
public:
	CVicEditorLayer(CViewC64VicEditor *vicEditor, const char *layerName);
	virtual ~CVicEditorLayer();
	
	CViewC64VicEditor *vicEditor;
	CDebugInterfaceC64 *debugInterface;

	const char *layerName;

	bool isVisible;
	bool isPaintingLocked;
	
	virtual void RenderMain(vicii_cycle_state_t *viciiState);
	virtual void RenderGridMain(vicii_cycle_state_t *viciiState);
	virtual void RenderPreview(vicii_cycle_state_t *viciiState);
	virtual void RenderGridPreview(vicii_cycle_state_t *viciiState);
	
	// @returns if pixel belongs to this layer
	virtual bool PixelBelongsToLayer(int x, int y);

	virtual u8 Paint(bool forceColorReplace, bool isDither, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue);
	virtual bool GetColorAtPixel(int x, int y, u8 *color);
	
	virtual void ClearScreen();
	virtual void ClearScreen(u8 charValue, u8 colorValue);

	virtual void LayerSelected(bool isSelected);
	
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer, int version);
	
	//CGuiButtonSwitch *btnVisible;
	
};

#endif
