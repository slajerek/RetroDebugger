#ifndef _VICEDITORLAYERC64UNRESTRICTEDBITMAP_H_
#define _VICEDITORLAYERC64UNRESTRICTEDBITMAP_H_

#include "SYS_Defs.h"
#include "CVicEditorLayer.h"
#include "CSlrImage.h"

class CVicEditorLayerUnrestrictedBitmap : public CVicEditorLayer
{
	public:
	CVicEditorLayerUnrestrictedBitmap(CViewVicEditor *vicEditor, char *layerName);
	~CVicEditorLayerUnrestrictedBitmap();
	
	virtual void RenderMain(vicii_cycle_state_t *viciiState);
	virtual void RenderPreview(vicii_cycle_state_t *viciiState);

	virtual void Serialise(CByteBuffer *byteBuffer);
	virtual void Deserialise(CByteBuffer *byteBuffer, int version);
	
	void RefreshImage();
	CImageData *imageDataUnrestricted;
	CSlrImage *imageUnrestricted;
	
	float screenTexEndX, screenTexEndY;
	
	u8 map[384*272];
	
	u8 PutPixelUnrestrictedBitmap(int x, int y, u8 paintColor);
	
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
	void UpdateBitmapFromMap();
	
	int NumVisiblePixels();
};

#endif
