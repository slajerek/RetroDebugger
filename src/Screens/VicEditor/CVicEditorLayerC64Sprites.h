#ifndef _VICEDITORLAYERC64SPRITES_H_
#define _VICEDITORLAYERC64SPRITES_H_

#include "SYS_Defs.h"
#include "CVicEditorLayer.h"

class CVicEditorLayerC64Sprites : public CVicEditorLayer
{
	public:
	CVicEditorLayerC64Sprites(CViewVicEditor *vicEditor);
	~CVicEditorLayerC64Sprites();
	
	virtual void RenderMain(vicii_cycle_state_t *viciiState);
	virtual void RenderGridMain(vicii_cycle_state_t *viciiState);
	virtual void RenderPreview(vicii_cycle_state_t *viciiState);
	virtual void RenderGridPreview(vicii_cycle_state_t *viciiState);

	virtual u8 Paint(bool forceColorReplace, bool isDither, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue);
	virtual bool GetColorAtPixel(int x, int y, u8 *color);

	virtual void Serialise(CByteBuffer *byteBuffer);
	virtual void Deserialise(CByteBuffer *byteBuffer, int version);
};

#endif
