#ifndef _CVicEditorBrush_h_
#define _CVicEditorBrush_h_

#include "CSlrImage.h"

class CVicEditorBrush
{
public:
	CVicEditorBrush();
	~CVicEditorBrush();

	int brushSize;
	CImageData *brushImage;
	
	//
	int GetWidth();
	int GetHeight();
	u8 GetPixel(int x, int y);
	void CreateBrushCircle(int newSize);
	void CreateBrushRectangle(int newSize);
};

#endif
