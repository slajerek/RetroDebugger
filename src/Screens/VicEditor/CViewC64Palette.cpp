#include "CViewC64Palette.h"
#include "SYS_Main.h"
#include "RES_ResourceManager.h"
#include "CGuiMain.h"
#include "CDataAdapter.h"
#include "CSlrString.h"
#include "SYS_KeyCodes.h"
#include "CViewC64.h"
#include "CViewMemoryMap.h"
#include "C64Tools.h"
#include "CViewC64Screen.h"
#include "CDebugInterfaceC64.h"
#include "SYS_Threading.h"
#include "CGuiEditHex.h"
#include "VID_ImageBinding.h"
#include "CViewC64.h"
#include "CViewVicEditor.h"
#include "CDebugInterfaceVice.h"
#include "C64VicDisplayCanvas.h"

CViewC64Palette::CViewC64Palette(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewC64PaletteCallback *callback)
: CGuiWindow(name, posX, posY, posZ, sizeX, sizeY, new CSlrString("Palette"))
{
	this->callback = callback;
	
	this->isVertical = false;
	this->gap1 = 0.0f;
	this->gap2 = 0.0f;
	this->rectSize = 0.0f;
	this->rectSize4 = 0.0f;
	this->rectSizeBig = 0.0f;

	this->SetPosition(posX, posY, sizeX, false);
	
	colorD020 = 14;
	colorD021 = 6;
	colorLMB = 15;
	colorRMB = 14;
	
	LOGD("1 CViewC64Palette this->sizeX=%f this->rectSize=%f", this->sizeX, this->rectSize);
	this->UpdatePosition();
	LOGD("2 CViewC64Palette this->sizeX=%f this->rectSize=%f", this->sizeX, this->rectSize);

	float scale = (this->sizeX - 8.0f*this->gap1 - this->gap2)/10.0f;
	SetPaletteRectScale(scale);

}

void CViewC64Palette::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	LOGD("CViewC64Palette::SetPosition: %f %f", posX, posY);
	CGuiWindow::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64Palette::SetPosition(float posX, float posY, float sizeX, bool isVertical)
{
	this->isVertical = isVertical;
	
	if (!isVertical)
	{
		// horizontal size
		//float size1 = 10*scale + 8*gap1 + gap2;
		float newScale = (sizeX - 8.0f*gap1 - gap2)/10.0f;
		
		this->SetPosition(this->posX, this->posY, this->posZ, newScale);
		
	}
	else
	{
		SYS_FatalExit("TODO");
	}
}

void CViewC64Palette::SetPaletteRectScale(float scale)
{
	// so we have
	this->gap1 = scale / 8.0f;
	this->gap2 = scale / 4.0f;
	this->rectSize = scale;
	this->rectSize4 = scale/4.0f;
	
	this->rectSizeBig = scale * 2.15f;
}

// scale is scale of a colour rect
void CViewC64Palette::SetPosition(float posX, float posY, float posZ, float scale)
{
	SetPaletteRectScale(scale);
	
	float size1 = 8.0f*scale + 7.0f*gap1 + gap2 + scale*2.0f + gap1;
	float size2 = scale * 2.0f + gap1;

	if (!isVertical)
	{
		this->SetPosition(posX, posY, posZ, size1, size2);
	}
	else
	{
		this->SetPosition(posX, posY, posZ, size2, size1);
	}
}


void CViewC64Palette::DoLogic()
{
	CGuiWindow::DoLogic();
}


void CViewC64Palette::Render()
{
	//	LOGD("CViewC64Palette::Render: pos=%f %f", posX, posY);
	
	this->RenderWindowBackground();
	this->RenderPalette(true);
	
	CGuiWindow::Render();

	BlitRectangle(posX, posY, posZ, sizeX, sizeY, 0, 0, 0, 1, 1);
}

void CViewC64Palette::RenderPalette(bool renderBackgroundInformation)
{
	float py = posY;
	float px;
	
	if (!isVertical)
	{
		int colNum = 0;
		for (int p = 0; p < 2; p++)
		{
			px = posX;
			for (int i = 0; i < 8; i++)
			{
				float fr,fg,fb;
				viewC64->debugInterfaceC64->GetFloatCBMColor(colNum, &fr, &fg, &fb);
				BlitFilledRectangle(px, py, posZ, this->rectSize, this->rectSize, fr, fg, fb, 1.0f);
				
				if (colNum == this->colorLMB)
				{
					BlitRectangle(px+1, py+1, posZ, this->rectSize-1, this->rectSize-1, 1.0f, 0.2f, 0.2f, 1.0f, 1.0f);
				}
				
				colNum++;
				
				px += this->rectSize + this->gap1;
			}
			
			py += this->rectSize + this->gap1;
		}
		
		if (renderBackgroundInformation)
		{
			//
			// background
			float sy = posY;
			float sx = px + gap1;
			py = sy;
			px = sx;
			
			float r,g,b;
			viewC64->debugInterfaceC64->GetFloatCBMColor(colorD021, &r, &g, &b);
			
			BlitFilledRectangle(px, py, posZ, this->rectSizeBig, this->rectSizeBig, r, g, b, 1);
			
			sx += gap2;
			sy += gap2;
			
			px = sx + rectSize*0.65f;
			py = sy + rectSize*0.65f;
			
			viewC64->debugInterfaceC64->GetFloatCBMColor(colorRMB, &r, &g, &b);
			BlitFilledRectangle(px, py, posZ, this->rectSize, this->rectSize, r, g, b, 1);
			BlitRectangle(px, py, posZ, this->rectSize, this->rectSize, 0, 0, 0, 1);
			
			px = sx;
			py = sy;
			viewC64->debugInterfaceC64->GetFloatCBMColor(colorLMB, &r, &g, &b);
			BlitFilledRectangle(px, py, posZ, this->rectSize, this->rectSize, r, g, b, 1);
			BlitRectangle(px, py, posZ, this->rectSize, this->rectSize, 0, 0, 0, 1);
		}
	}
}

bool CViewC64Palette::DoTap(float x, float y)
{
	LOGG("CViewC64Palette::DoTap: %f %f", x, y);
	
	if (IsInsideNonVisible(x, y))
	{
		int colorIndex = GetColorIndex(x, y);
		
		if (colorIndex != -1)
		{
			SetColorLMB(colorIndex);
			return true;
		}
		
		return true;
	}
	
	return CGuiWindow::DoTap(x, y);
}

bool CViewC64Palette::DoRightClick(float x, float y)
{
	LOGI("CViewC64Palette::DoRightClick: %f %f", x, y);

	if (IsInside(x, y))
	{
		LOGD(".......inside");
		int colorIndex = GetColorIndex(x, y);
		
		if (colorIndex != -1)
		{
			SetColorRMB(colorIndex);

			LOGD(" CViewC64Palette: ret true");
			return true;
		}
		
		return true;
	}
	
	LOGD(" CViewC64Palette: ret CGuiWindow::DoRightClick");
	return CGuiWindow::DoRightClick(x, y);
}

bool CViewC64Palette::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	if (this->IsInsideView(x, y))
		return true;
	
	return CGuiWindow::DoMove(x, y, distX, distY, diffX, diffY);
}

int CViewC64Palette::GetColorIndex(float x, float y)
{
	LOGD("CViewC64Palette::GetColorIndex: %f %f", x, y);
	
	float py = posY;
	
	if (!isVertical)
	{
		int colNum = 0;
		for (int p = 0; p < 2; p++)
		{
			float px = posX;
			for (int i = 0; i < 8; i++)
			{
				if (x >= px && x <= (px + this->rectSize)
					&& y >= py && y <= (py + this->rectSize))
				{
					return colNum;
				}
				
				colNum++;
				
				px += this->rectSize + this->gap1;
			}
			
			py += this->rectSize + this->gap1;
		}
	}
	
	return -1;
}


bool CViewC64Palette::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI("CViewC64Palette::KeyDown: %d", keyCode);
	
	if (keyCode == 'x')
	{
		u8 t = colorLMB;
		SetColorLMB(colorRMB);
		SetColorRMB(t);
		return true;
	}
	
	return false;
}

bool CViewC64Palette::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

void CViewC64Palette::SetColorLMB(u8 color)
{
	this->colorLMB = color;
	callback->PaletteColorChanged(VICEDITOR_COLOR_SOURCE_LMB, color);
}

void CViewC64Palette::SetColorRMB(u8 color)
{
	this->colorRMB = color;
	callback->PaletteColorChanged(VICEDITOR_COLOR_SOURCE_RMB, color);
}

void CViewC64PaletteCallback::PaletteColorChanged(u8 colorSource, u8 newColorValue)
{
}


