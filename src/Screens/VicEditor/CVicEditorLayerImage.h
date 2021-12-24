#ifndef _VICEDITORLAYERIMAGE_H_
#define _VICEDITORLAYERIMAGE_H_

#include "SYS_Defs.h"
#include "CVicEditorLayer.h"
#include "CSlrImage.h"

#define C64_SCREEN_OFFSET_X 32
#define C64_SCREEN_OFFSET_Y 35

class CVicEditorLayerImage : public CVicEditorLayer
{
	public:
	CVicEditorLayerImage(CViewVicEditor *vicEditor, char *layerName);
	~CVicEditorLayerImage();
	
	virtual void RenderMain(vicii_cycle_state_t *viciiState);
	virtual void RenderPreview(vicii_cycle_state_t *viciiState);

	void RefreshImage();
	CImageData *imageData;
	CSlrImage *image;
	
	float screenTexEndX, screenTexEndY;
	
	u8 PutPixelImage(int x, int y, u8 paintColor);
	u8 PutPixelImage(int x, int y, u8 r, u8 g, u8 b, u8 a);
	u8 PutColorAtPixel(int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource);
	u8 PaintDither(int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource);
	
	virtual u8 Paint(bool forceColorReplace, bool isDither, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue);
	virtual bool GetColorAtPixel(int x, int y, u8 *color);

	virtual void ClearScreen();
	virtual void ClearScreen(u8 charValue, u8 colorValue);

	virtual void ClearDitherMask();
	int ditherMaskPosX;
	int ditherMaskPosY;

	bool LoadFrom(CImageData *imageData);
	CImageData *GetFullDisplayImage();
	CImageData *GetScreenImage();
	
	int NumVisiblePixels();
	
	virtual void LayerSelected(bool isSelected);

	virtual void Serialise(CByteBuffer *byteBuffer);
	virtual void Deserialise(CByteBuffer *byteBuffer, int version);
	

};

#endif
