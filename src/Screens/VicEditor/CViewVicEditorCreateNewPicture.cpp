#include "CViewVicEditorCreateNewPicture.h"
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
#include "CDebugInterfaceVice.h"
#include "CViewVicEditor.h"
#include "CViewC64VicDisplay.h"
#include "CVicEditorLayer.h"
#include "CVicEditorLayerC64Canvas.h"
#include "CVicEditorLayerImage.h"
#include "C64SettingsStorage.h"
#include "CViewC64VicControl.h"
#include "CViewC64Palette.h"
#include "CSlrFileFromOS.h"
#include "CSlrFileFromDocuments.h"
#include <list>


CViewVicEditorCreateNewPicture::CViewVicEditorCreateNewPicture(const char *name, float posX, float posY, float posZ, CViewVicEditor *vicEditor)
: CGuiWindow(name, posX, posY, posZ, 123, 98, new CSlrString("New picture"))
{
	this->vicEditor = vicEditor;
	
	CSlrFont *font = this->vicEditor->font;
	float fontScale = this->vicEditor->fontScale;
	float fontHeight = this->vicEditor->fontHeight;
	
	float px = 21.5f;
	float py = 4.0f;
	
	float sx = sizeX - 5.0f;
	float sy = sizeY; //57;
	
	font = viewC64->fontCBMShifted;
	fontScale = 1.25f;
	
	
	float buttonSizeX = 80.0f;
	float buttonSizeY = 11.0f;

	float gpx = 1.9f;
	float elgpx = fontHeight * 0.5f;
	
	
//	barFont->BlitTextColor(this->barTitle, view->posX, view->posY-barHeight+frameWidth*0.75f, view->posZ, fontSize,
//						   barTextColorR, barTextColorG, barTextColorB, barTextColorA);

	
//	lblImageMode = new CGuiLabel("Image mode:", false, px, py, posZ, listSizeX, lstFontSize, LABEL_ALIGNED_LEFT, lstFontSize, lstFontSize, NULL);
//	this->AddGuiElement(lblImageMode);

	
	btnNewPictureModeTextHires = new CGuiButton(NULL, NULL,
											  px, py, posZ, buttonSizeX, buttonSizeY,
											  new CSlrString("Text Hires"),
											  FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
											  font, fontScale,
											  1.0, 1.0, 1.0, 1.0,
											  0.3, 0.3, 0.3, 1.0,
											  this);
	this->AddGuiElement(btnNewPictureModeTextHires);
	
	py += buttonSizeY + gpx;

	btnNewPictureModeTextMulti = new CGuiButton(NULL, NULL,
												px, py, posZ, buttonSizeX, buttonSizeY,
												new CSlrString("Text Multi"),
												FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
												font, fontScale,
												1.0, 1.0, 1.0, 1.0,
												0.3, 0.3, 0.3, 1.0,
												this);
	this->AddGuiElement(btnNewPictureModeTextMulti);
	
	py += buttonSizeY + gpx;

	
	btnNewPictureModeBitmapHires = new CGuiButton(NULL, NULL,
											  px, py, posZ, buttonSizeX, buttonSizeY,
											  new CSlrString("Bitmap Hires"),
											  FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
											  font, fontScale,
											  1.0, 1.0, 1.0, 1.0,
											  0.3, 0.3, 0.3, 1.0,
											  this);
	this->AddGuiElement(btnNewPictureModeBitmapHires);
	
	py += buttonSizeY + gpx;
	
	btnNewPictureModeBitmapMulti = new CGuiButton(NULL, NULL,
											px, py, posZ, buttonSizeX, buttonSizeY,
											new CSlrString("Bitmap Multi"),
											FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
											font, fontScale,
											1.0, 1.0, 1.0, 1.0,
											0.3, 0.3, 0.3, 1.0,
											this);
	this->AddGuiElement(btnNewPictureModeBitmapMulti);

	
	px = 5.0f;
	
	py += buttonSizeY + gpx;
	py += elgpx;
	
	lblBackgroundColor = new CGuiLabel(new CSlrString("Background color:"), px, py, posZ, buttonSizeX, fontHeight, LABEL_ALIGNED_LEFT, font, fontScale,
										  1, 1, 1, 1, NULL);
	this->AddGuiElement(lblBackgroundColor);

	py += fontHeight;

	//
	float sp = 0.85f;
	viewPalette = new CViewC64Palette("Palette", px, py, posZ, 150*sp, 30*sp, this);
	viewPalette->visible = false; // we will render this by ourselves
	this->AddGuiElement(viewPalette);
}

bool CViewVicEditorCreateNewPicture::ButtonPressed(CGuiButton *button)
{
	if (button == btnNewPictureModeTextHires)
	{
		CreateNewPicture(C64_PICTURE_MODE_TEXT_HIRES, viewPalette->colorLMB, true);
		return true;
	}
	else if (button == btnNewPictureModeTextMulti)
	{
		CreateNewPicture(C64_PICTURE_MODE_TEXT_MULTI, viewPalette->colorLMB, true);
		return true;
	}
	else if (button == btnNewPictureModeBitmapHires)
	{
		CreateNewPicture(C64_PICTURE_MODE_BITMAP_HIRES, viewPalette->colorLMB, true);
		return true;
	}
	else if (button == btnNewPictureModeBitmapMulti)
	{
		CreateNewPicture(C64_PICTURE_MODE_BITMAP_MULTI, viewPalette->colorLMB, true);
		return true;
	}

	return false;
}

void CViewVicEditorCreateNewPicture::CreateNewPicture(u8 mode, u8 backgroundColor, bool storeUndo)
{
	LOGD("CViewVicEditorCreateNewPicture::CreateNewPicture: mode=%d", mode);
	
	if (storeUndo)
	{
		vicEditor->StoreUndo();
	}

	viewC64->debugInterfaceC64->SetDebugMode(DEBUGGER_MODE_RUNNING);

	guiMain->LockMutex();
	viewC64->debugInterfaceC64->LockRenderScreenMutex();
	
	viewC64->viewC64VicControl->UnlockAll();
	
	debugInterfaceVice->SetPatchKernalFastBoot(true);
	
	viewC64->debugInterfaceC64->DetachCartridge();
//	viewC64->debugInterfaceC64->HardReset();
	
	SYS_Sleep(350);
	
	if (mode == C64_PICTURE_MODE_TEXT_HIRES)
	{
		vicEditor->SetVicMode(false, false, false);
		vicEditor->SetVicAddresses(0, 0x0C00, 0x1000, 0x0000);
		
		vicEditor->viewCharset->SetVisible(true);
	}
	else if (mode == C64_PICTURE_MODE_TEXT_MULTI)
	{
		vicEditor->SetVicMode(false, true, false);
		vicEditor->SetVicAddresses(0, 0x0C00, 0x1000, 0x0000);

		vicEditor->viewCharset->SetVisible(true);
	}
	else if (mode == C64_PICTURE_MODE_BITMAP_HIRES)
	{
		vicEditor->SetVicMode(true, false, false);
		vicEditor->SetVicAddresses(0, 0x0C00, 0x0000, 0x2000);

		vicEditor->viewCharset->SetVisible(false);
	}
	else if (mode == C64_PICTURE_MODE_BITMAP_MULTI)
	{
		vicEditor->SetVicMode(true, true, false);
		vicEditor->SetVicAddresses(0, 0x0C00, 0x0000, 0x2000);

		vicEditor->viewCharset->SetVisible(false);
	}
	
	vicEditor->viewPalette->colorD020 = backgroundColor;
	vicEditor->viewPalette->colorD021 = backgroundColor;
	
	viewC64->debugInterfaceC64->SetVicRegister(0x20, vicEditor->viewPalette->colorD020);
	viewC64->debugInterfaceC64->SetVicRegister(0x21, vicEditor->viewPalette->colorD021);
	
	vicEditor->ClearScreen();

	vicEditor->viewVicDisplayMain->UpdateViciiState();
	vicEditor->viewVicDisplaySmall->UpdateViciiState();

	vicEditor->RunC64EndlessLoop();
	
	viewC64->debugInterfaceC64->UnlockRenderScreenMutex();
	guiMain->UnlockMutex();
	
	debugInterfaceVice->SetPatchKernalFastBoot(c64SettingsFastBootKernalPatch);
	
	this->SetVisible(false);
	
	c64SettingsVicEditorDefaultBackgroundColor = backgroundColor;

	C64DebuggerStoreSettings();
}


void CViewVicEditorCreateNewPicture::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	LOGD("CViewVicEditorCreateNewPicture::SetPosition: %f %f", posX, posY);
	
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
	

}

void CViewVicEditorCreateNewPicture::DoLogic()
{
	CGuiView::DoLogic();
}


void CViewVicEditorCreateNewPicture::Render()
{
	BlitFilledRectangle(posX, posY, posZ, sizeX, sizeY, viewFrame->barColorR, viewFrame->barColorG, viewFrame->barColorB, 1);

	CGuiView::Render();

	viewPalette->RenderPalette(false);
	
	BlitRectangle(posX, posY, posZ, sizeX, sizeY, 0, 0, 0, 1, 1);
}

bool CViewVicEditorCreateNewPicture::DoTap(float x, float y)
{
	LOGG("CViewVicEditorCreateNewPicture::DoTap: %f %f", x, y);
	
	if (viewPalette->DoTap(x, y))
		return true;

	if (CGuiView::DoTap(x, y) == false)
	{
		if (this->IsInsideView(x, y))
		{
			return true;
		}
	}
	
	return CGuiView::DoTap(x, y);;
}

bool CViewVicEditorCreateNewPicture::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	if (this->IsInsideView(x, y))
		return true;
	
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}


bool CViewVicEditorCreateNewPicture::DoRightClick(float x, float y)
{
	LOGI("CViewVicEditorCreateNewPicture::DoRightClick: %f %f", x, y);
	
	if (this->IsInsideView(x, y))
		return true;
	
	return CGuiView::DoRightClick(x, y);
}


bool CViewVicEditorCreateNewPicture::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("CViewVicEditorCreateNewPicture::KeyDown: %d", keyCode);
	
	return false;
}

bool CViewVicEditorCreateNewPicture::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

void CViewVicEditorCreateNewPicture::ActivateView()
{
	viewPalette->SetColorLMB(c64SettingsVicEditorDefaultBackgroundColor);
	viewPalette->SetColorRMB(c64SettingsVicEditorDefaultBackgroundColor);
}

