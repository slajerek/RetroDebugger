#ifndef _VICEDITORLAYERC64SCREEN_H_
#define _VICEDITORLAYERC64SCREEN_H_

#include "SYS_Defs.h"
#include "CVicEditorLayer.h"

class CImageData;

class CVicEditorLayerC64Screen : public CVicEditorLayer
{
public:
	CVicEditorLayerC64Screen(CViewVicEditor *vicEditor);
	~CVicEditorLayerC64Screen();
	
	virtual void RenderMain(vicii_cycle_state_t *viciiState);
	virtual void RenderPreview(vicii_cycle_state_t *viciiState);

	virtual u8 Paint(bool forceColorReplace, bool isDither, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue);
	virtual bool GetColorAtPixel(int x, int y, u8 *color);
	
	virtual CImageData *GetScreenImage(int *width, int *height);
	virtual CImageData *GetInteriorScreenImage();

	virtual void Serialise(CByteBuffer *byteBuffer);
	virtual void Deserialise(CByteBuffer *byteBuffer, int version);
};

#endif
