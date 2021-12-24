#ifndef _VICEDITORLAYERC64CANVAS_H_
#define _VICEDITORLAYERC64CANVAS_H_

#include "SYS_Defs.h"
#include "CVicEditorLayer.h"

class CVicEditorLayerC64Canvas : public CVicEditorLayer
{
	public:
	CVicEditorLayerC64Canvas(CViewVicEditor *vicEditor);
	~CVicEditorLayerC64Canvas();
	
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
	
	virtual void Serialise(CByteBuffer *byteBuffer);
	virtual void Deserialise(CByteBuffer *byteBuffer, int version);
};

#endif
