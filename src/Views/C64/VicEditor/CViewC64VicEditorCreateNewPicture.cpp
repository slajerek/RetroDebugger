#include "CViewC64VicEditorCreateNewPicture.h"
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
#include "CDebugInterfaceVice.h"
#include "CViewC64VicEditor.h"
#include "CViewC64VicDisplay.h"
#include "CVicEditorLayer.h"
#include "CVicEditorLayerC64Canvas.h"
#include "CVicEditorLayerImage.h"
#include "C64SettingsStorage.h"
#include "CViewC64VicControl.h"
#include "CViewC64Charset.h"
#include "CViewC64Palette.h"
#include "CSlrFileFromOS.h"
#include "CSlrFileFromDocuments.h"
#include <list>

CViewC64VicEditorCreateNewPicture::CViewC64VicEditorCreateNewPicture(const char *name, float posX, float posY, float posZ, CViewC64VicEditor *vicEditor)
: CGuiView(name, posX, posY, posZ, 233, 250)			// 233 x 250 WTF?????
{
	this->vicEditor = vicEditor;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	float px = 41.5f;
	float py = 4.0f;
	
	float sx = sizeX - 5.0f;
	float sy = sizeY; //57;
	
	font = viewC64->fontDefaultCBMShifted;
	fontScale = 3.0f;
	float fontHeight = this->vicEditor->fontHeight;

	
	float buttonSizeX = 150.0f;
	float buttonSizeY = 21.0f;

	float gpx = fontScale*(1.25f/1.9f) * 1.9f;
	float elgpx = fontHeight * 0.5f;
	
	
//	barFont->BlitTextColor(this->barTitle, view->posX, view->posY-barHeight+frameWidth*0.75f, view->posZ, fontSize,
//						   barTextColorR, barTextColorG, barTextColorB, barTextColorA);

	
//	lblImageMode = new CGuiLabel("Image mode:", false, px, py, posZ, listSizeX, lstFontSize, LABEL_ALIGNED_LEFT, lstFontSize, lstFontSize, NULL);
//	this->AddGuiElement(lblImageMode);

	
	float ly = 4.5f;
	btnNewPictureModeTextHires = new CGuiButton(NULL, NULL,
											  px, py, posZ, buttonSizeX, buttonSizeY,
											  new CSlrString("Text Hires"),
											  FONT_ALIGN_CENTER, buttonSizeX/2, ly,
											  font, fontScale,
											  1.0, 1.0, 1.0, 1.0,
											  0.3, 0.3, 0.3, 1.0,
											  this);
	this->AddGuiElement(btnNewPictureModeTextHires);
	
	py += buttonSizeY + gpx;

	btnNewPictureModeTextMulti = new CGuiButton(NULL, NULL,
												px, py, posZ, buttonSizeX, buttonSizeY,
												new CSlrString("Text Multi"),
												FONT_ALIGN_CENTER, buttonSizeX/2, ly,
												font, fontScale,
												1.0, 1.0, 1.0, 1.0,
												0.3, 0.3, 0.3, 1.0,
												this);
	this->AddGuiElement(btnNewPictureModeTextMulti);
	
	py += buttonSizeY + gpx;

	
	btnNewPictureModeBitmapHires = new CGuiButton(NULL, NULL,
											  px, py, posZ, buttonSizeX, buttonSizeY,
											  new CSlrString("Bitmap Hires"),
											  FONT_ALIGN_CENTER, buttonSizeX/2, ly,
											  font, fontScale,
											  1.0, 1.0, 1.0, 1.0,
											  0.3, 0.3, 0.3, 1.0,
											  this);
	this->AddGuiElement(btnNewPictureModeBitmapHires);
	
	py += buttonSizeY + gpx;
	
	btnNewPictureModeBitmapMulti = new CGuiButton(NULL, NULL,
											px, py, posZ, buttonSizeX, buttonSizeY,
											new CSlrString("Bitmap Multi"),
											FONT_ALIGN_CENTER, buttonSizeX/2, ly,
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
	float sp = 1.9f;
	viewPalette = new CViewC64Palette("Palette", px, py, posZ, 150*sp, 30*sp, this);
	viewPalette->visible = false; // we will render this by ourselves
	this->AddGuiElement(viewPalette);
}

bool CViewC64VicEditorCreateNewPicture::ButtonPressed(CGuiButton *button)
{
	if (button == btnNewPictureModeTextHires)
	{
		CreateNewPicture(C64_PICTURE_MODE_TEXT_HIRES, viewPalette->colorLMB, true);
		guiMain->RemoveViewSkippingLayout(this);
		guiMain->RemoveViewFromLayout(this);
		viewC64->viewVicEditor->SetVisible(true);
		return true;
	}
	else if (button == btnNewPictureModeTextMulti)
	{
		CreateNewPicture(C64_PICTURE_MODE_TEXT_MULTI, viewPalette->colorLMB, true);
		guiMain->RemoveViewFromLayout(this);
		guiMain->RemoveViewSkippingLayout(this);
		viewC64->viewVicEditor->SetVisible(true);
		return true;
	}
	else if (button == btnNewPictureModeBitmapHires)
	{
		CreateNewPicture(C64_PICTURE_MODE_BITMAP_HIRES, viewPalette->colorLMB, true);
		guiMain->RemoveViewFromLayout(this);
		guiMain->RemoveViewSkippingLayout(this);
		viewC64->viewVicEditor->SetVisible(true);
		return true;
	}
	else if (button == btnNewPictureModeBitmapMulti)
	{
		CreateNewPicture(C64_PICTURE_MODE_BITMAP_MULTI, viewPalette->colorLMB, true);
		guiMain->RemoveViewFromLayout(this);
		guiMain->RemoveViewSkippingLayout(this);
		viewC64->viewVicEditor->SetVisible(true);
		return true;
	}

	return false;
}

void CViewC64VicEditorCreateNewPicture::CreateNewPicture(u8 mode, u8 backgroundColor, bool storeUndo)
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
//	viewC64->debugInterfaceC64->ResetHard();
	
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
	if (mode == C64_PICTURE_MODE_TEXT_HIRES_EXTENDED)
	{
		vicEditor->SetVicMode(false, false, true);
		vicEditor->SetVicAddresses(0, 0x0C00, 0x1000, 0x0000);
		
		vicEditor->viewCharset->SetVisible(true);
	}
	else if (mode == C64_PICTURE_MODE_TEXT_MULTI_EXTENDED)
	{
		vicEditor->SetVicMode(false, true, true);
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

	vicEditor->viewVicDisplay->UpdateViciiState();
//	vicEditor->viewVicDisplaySmall->UpdateViciiState();

	vicEditor->RunC64EndlessLoop();
	
	viewC64->debugInterfaceC64->UnlockRenderScreenMutex();
	guiMain->UnlockMutex();
	
	debugInterfaceVice->SetPatchKernalFastBoot(c64SettingsFastBootKernalPatch);
		
	c64SettingsVicEditorDefaultBackgroundColor = backgroundColor;

	C64DebuggerStoreSettings();
}


void CViewC64VicEditorCreateNewPicture::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	LOGD("CViewVicEditorCreateNewPicture::SetPosition: %f %f", posX, posY);
	
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
	

}

void CViewC64VicEditorCreateNewPicture::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewC64VicEditorCreateNewPicture::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewC64VicEditorCreateNewPicture::Render()
{
	CGuiView::Render();

	viewPalette->RenderPalette(false);
	
	BlitRectangle(posX, posY, posZ, sizeX, sizeY, 0, 0, 0, 1, 1);
}

bool CViewC64VicEditorCreateNewPicture::DoTap(float x, float y)
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

bool CViewC64VicEditorCreateNewPicture::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	if (this->IsInsideView(x, y))
		return true;
	
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}


bool CViewC64VicEditorCreateNewPicture::DoRightClick(float x, float y)
{
	LOGI("CViewVicEditorCreateNewPicture::DoRightClick: %f %f", x, y);
	
	if (this->IsInsideView(x, y))
		return true;
	
	return CGuiView::DoRightClick(x, y);
}


bool CViewC64VicEditorCreateNewPicture::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("CViewVicEditorCreateNewPicture::KeyDown: %d", keyCode);
	
	return false;
}

bool CViewC64VicEditorCreateNewPicture::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

void CViewC64VicEditorCreateNewPicture::ActivateView()
{
	viewPalette->SetColorLMB(c64SettingsVicEditorDefaultBackgroundColor);
	viewPalette->SetColorRMB(c64SettingsVicEditorDefaultBackgroundColor);
}

