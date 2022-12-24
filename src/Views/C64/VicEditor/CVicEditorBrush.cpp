#include "CVicEditorBrush.h"

CVicEditorBrush::CVicEditorBrush()
{
	brushSize = 0;
	brushImage = NULL;
}

CVicEditorBrush::~CVicEditorBrush()
{
	if (brushImage)
		delete brushImage;
}

int CVicEditorBrush::GetWidth()
{
	return brushImage->width;
}

int CVicEditorBrush::GetHeight()
{
	return brushImage->height;
}

u8 CVicEditorBrush::GetPixel(int x, int y)
{
	return brushImage->GetPixelResultByte(x, y);
}

void CVicEditorBrush::CreateBrushCircle(int newSize)
{
	brushImage = new CImageData(newSize, newSize, IMG_TYPE_GRAYSCALE);
	brushImage->AllocImage(false, true);
	
	float r = (float)newSize / 2.0f;
	int ir = (int)r;

	float r2 = r*r;
	
	for (int y = -r; y < r; y++)
	{
		for (int x = -r; x < r; x++)
		{
			if ( (x*x + y*y) < r2 )
			{
				brushImage->SetPixelResultByte(x + ir, y + ir, 1);
			}
		}
	}
	
	//brushImage->debugPrint();
}

void CVicEditorBrush::CreateBrushRectangle(int newSize)
{
	brushImage = new CImageData(newSize, newSize, IMG_TYPE_GRAYSCALE);
	brushImage->AllocImage(false, true);
	
	for (int y = 0; y < newSize; y++)
	{
		for (int x = 0; x < newSize; x++)
		{
			brushImage->SetPixelResultByte(x, y, 1);
		}
	}

	//brushImage->debugPrint();
}

