#include "CViewC64Palette.h"
#include "SYS_Main.h"
#include "RES_ResourceManager.h"
#include "CGuiMain.h"
#include "CDataAdapter.h"
#include "CSlrString.h"
#include "SYS_KeyCodes.h"
#include "CViewC64.h"
#include "CViewDataMap.h"
#include "C64Tools.h"
#include "CViewC64Screen.h"
#include "CDebugInterfaceC64.h"
#include "SYS_Threading.h"
#include "CGuiEditHex.h"
#include "VID_ImageBinding.h"
#include "CViewC64.h"
#include "CLayoutParameter.h"
#include "CViewC64VicEditor.h"
#include "CDebugInterfaceVice.h"
#include "C64VicDisplayCanvas.h"
#include "C64SettingsStorage.h"
#include "C64Palette.h"

void CViewC64PaletteCallback::PaletteColorChanged(u8 colorSource, u8 newColorValue)
{
}

CViewC64Palette::CViewC64Palette(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewC64PaletteCallback *callback)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->callback = callback;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	renderColorHexNumber = false;
	AddLayoutParameter(new CLayoutParameterBool("Hex numbers", &renderColorHexNumber));

	this->isVertical = false;
//	AddLayoutParameter(new CLayoutParameterBool("Vertical layout", &isVertical));

	this->font = viewC64->fontDefaultCBMShifted;
	this->fontScale = 1;
	this->fontCharacterWidth = 1;
	
	this->gap1 = 1.0f;
	this->gap2 = 0.0f;
	this->rectSize = 0.0f;
	this->rectSize4 = 0.0f;
	this->rectSizeBig = 0.0f;

	colorD020 = 14;
	colorD021 = 6;
	colorLMB = 15;
	colorRMB = 14;
	
	LOGD("1 CViewC64Palette this->sizeX=%f this->rectSize=%f", this->sizeX, this->rectSize);
	this->UpdatePosition();
	LOGD("2 CViewC64Palette this->sizeX=%f this->rectSize=%f", this->sizeX, this->rectSize);

	float scale = (this->sizeX - 8.0f*this->gap1 - this->gap2)/10.0f;
	SetPaletteRectScale(scale);

	float size1 = 8.0f*scale + 7.0f*gap1 + gap2 + scale*2.0f + gap1;
	float size2 = scale * 2.0f + gap1;

	imGuiWindowKeepAspectRatio = true;
	if (!isVertical)
	{
		imGuiWindowAspectRatio = size1/size2;
	}
	else
	{
		imGuiWindowAspectRatio = size2/size1;
		SYS_FatalExit("TODO: CViewC64Palette isVertical");
	}
}

void CViewC64Palette::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	LOGD("CViewC64Palette::SetPosition: %f %f %f %f", posX, posY, sizeX, sizeY);
	float newScale = (sizeX - 8.0f*gap1 - gap2)/10.0f;
	LOGD("...newScale=%f sizeX=%f gap1=%f gap2=%f", newScale, sizeX, gap1, gap2);
	SetPaletteRectScale(newScale);
	newScale = (sizeX - 8.0f*gap1 - gap2)/10.0f;
	SetPaletteRectScale(newScale);

	fontScale = 0.075f * newScale;
	fontCharacterWidth = font->GetCharWidth('@', fontScale);
	fontCharacterHeight = font->GetCharHeight('@', fontScale);

	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64Palette::SetIsVertical(bool isVertical)
{
	this->isVertical = isVertical;
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64Palette::SetPaletteRectScale(float scale)
{
	LOGD("CViewC64Palette::SetPaletteRectScale scale=%f", scale);
	
	// so we have
	this->gap1 = scale / 8.0f;
	this->gap2 = scale / 4.0f;
	this->rectSize = scale;
	this->rectSize4 = scale/4.0f;
	
	this->rectSizeBig = scale * 2.15f;
}

void CViewC64Palette::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewC64Palette::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewC64Palette::Render()
{
//	LOGD("CViewC64Palette::Render: pos=%f %f %f %f", posX, posY, sizeX, sizeY);
	
	this->RenderPalette(true);
	
	BlitRectangle(posX, posY, posZ, sizeX, sizeY, 0, 0, 0, 1, 1);
}

void CViewC64Palette::RenderPalette(bool renderBackgroundInformation)
{
	float py = posY;
	float px = posX;

	char *buf = SYS_GetCharBuf();

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
					float oy = (p == 0 ? 1:0);
					float osy = (p == 0 ? 0:-1);
					BlitRectangle(px, py + oy, posZ, this->rectSize, this->rectSize + osy, 1.0f, 0.2f, 0.2f, 1.0f, 1.0f);
				}
				
				if (renderColorHexNumber)
				{
					sprintf(buf, "%1X", colNum);
					float phx = px + this->rectSize/2.0f - fontCharacterWidth/2.0f;
					float phy = py + this->rectSize/2.0f - fontCharacterHeight/2.0f;
					if (colNum != 1)
					{
						font->BlitTextColor(buf, phx, phy, -1, fontScale, 1, 1, 1, 1);
					}
					else
					{
						font->BlitTextColor(buf, phx, phy, -1, fontScale, 0, 0, 0, 1);
					}
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
	
	SYS_ReleaseCharBuf(buf);
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
	
	return CGuiView::DoTap(x, y);
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
	return CGuiView::DoRightClick(x, y);
}

bool CViewC64Palette::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	if (this->IsInsideView(x, y))
		return true;
	
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
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

void CViewC64Palette::SwitchSelectedColors()
{
	u8 t = colorLMB;
	SetColorLMB(colorRMB);
	SetColorRMB(t);
}

bool CViewC64Palette::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI("CViewC64Palette::KeyDown: %d", keyCode);
	
	if (keyCode == 'x')
	{
		SwitchSelectedColors();
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


bool CViewC64Palette::HasContextMenuItems()
{
	return true;
}

void CViewC64Palette::RenderContextMenuItems()
{
	RenderContextMenuLayoutParameters(true);
	if (ImGui::MenuItem("Switch selected colors", "x"))
	{
		SwitchSelectedColors();
	}
	ImGui::Separator();
	RenderMenuItemsAvailablePalettes();
}

void CViewC64Palette::RenderMenuItemsAvailablePalettes()
{
	std::vector<const char *> *options = new std::vector<const char *>();
	C64GetAvailablePalettes(options);
	
	int i = 0;
	for (std::vector<const char *>::iterator it = options->begin(); it != options->end(); it++)
	{
		const char *paletteName = *it;
		if (ImGui::MenuItem(paletteName, NULL, i == c64SettingsVicPalette))
		{
			C64DebuggerSetSetting("VicPalette", &i);
			C64DebuggerStoreSettings();
			break;
		}
		i++;
	}
}
