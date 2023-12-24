#include "CViewC64Charset.h"
#include "CViewC64VicEditor.h"
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
#include "CViewC64VicDisplay.h"
#include "CViewDataDump.h"
#include "C64SettingsStorage.h"
#include "CSlrFileFromOS.h"
#include "CMainMenuBar.h"

CViewC64Charset::CViewC64Charset(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewC64VicEditor *vicEditor)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->vicEditor = vicEditor;
	this->debugInterface = vicEditor->debugInterface;
	this->vicDisplay = vicEditor->viewVicDisplay;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	int w = 256;
	int h = 64;
	this->imGuiWindowAspectRatio = (float)w/(float)h;
	this->imGuiWindowKeepAspectRatio = true;

	imageDataCharset = new CImageData(w, h, IMG_TYPE_RGBA);
	imageDataCharset->AllocImage(false, true);
		
	imageCharset = new CSlrImage(true, false);
	imageCharset->LoadImage(imageDataCharset, RESOURCE_PRIORITY_STATIC, false);
	imageCharset->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
	imageCharset->resourcePriority = RESOURCE_PRIORITY_STATIC;
	VID_PostImageBinding(imageCharset, NULL);

	selX = -1;
	selY = -1;
	selSizeX = this->sizeX / 32.0f;
	selSizeY = this->sizeY / 8.0f;

	charsetFileExtensions.push_back(new CSlrString("charset"));
	charsetFileExtensions.push_back(new CSlrString("bin"));
	charsetFileExtensions.push_back(new CSlrString("data"));
	recentlyOpened = new CRecentlyOpenedFiles(new CSlrString("recents-charsets"), this);

	/*  TODO TOOLBOX
	// setup icons in frame
	this->AddBarToolBox(this);
	
	imgIconImport = RES_GetImage("/gfx/icon_raw_import", true);
	this->AddBarIcon(imgIconImport);
	
	imgIconExport = RES_GetImage("/gfx/icon_raw_export", true);
	this->AddBarIcon(imgIconExport);
	 */
		
	SelectChar(17 + 0x40);
	
	this->UpdatePosition();
}

void CViewC64Charset::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
//	LOGD("CViewC64Charset::SetPosition: %f %f", posX, posY);
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
	
	selSizeX = this->sizeX / 32.0f;
	selSizeY = this->sizeY / 8.0f;
	
	if (selectedChar >= 0)
	{
		SelectChar(selectedChar);
	}
}

void CViewC64Charset::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewC64Charset::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewC64Charset::Render()
{
//	LOGD("CViewC64Charset::Render: pos=%f %f", posX, posY);
//	BlitFilledRectangle(posX, posY, posZ, sizeX, sizeY, viewFrame->barColorR, viewFrame->barColorG, viewFrame->barColorB, 1);
	float py = posY;
	float px;
	
	vicii_cycle_state_t *viciiState = &(viewC64->viciiStateToShow);

	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 colors[0x0F];
	
	vicEditor->viewVicDisplay->GetViciiPointers(viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);

	if (viewC64->viewC64MemoryDataDump->renderDataWithColors)
	{
		CopyMultiCharsetToImage(chargen_ptr, imageDataCharset, 32, colors[1], colors[2], colors[3],
								viewC64->colorToShowD800, viewC64->debugInterfaceC64);
	}
	else
	{
		CopyHiresCharsetToImage(chargen_ptr, imageDataCharset, 32, 0, 1, viewC64->debugInterfaceC64);
	}

	imageCharset->ReBindImageData(imageDataCharset);

	
	//viewC64->debugInterface
	
	// nearest neighbour
//	LOGTODO("is it required?");
//	this->imageCharset->SetLinearScaling(false);
		
	Blit(this->imageCharset, posX, posY+1.0f, posZ, sizeX, sizeY);

	if (selectedChar >= 0)
	{
		BlitRectangle(this->posX + selX, this->posY + selY, posZ, selSizeX, selSizeY, 1.0f, 0.0f, 0.0f, 1.0f, 1.5f);
	}
	
	
//	CGuiWindow::Render();

//	BlitRectangle(posX, posY, posZ, sizeX, sizeY, 0, 0, 0, 1, 1);
}

bool CViewC64Charset::DoTap(float x, float y)
{
	LOGG("CViewC64Charset::DoTap: %f %f", x, y);

	if (this->IsInsideView(x, y))
	{
		int xp = (int)floor((x - this->posX) / selSizeX);
		int yp = (int)floor((y - this->posY) / selSizeY);
		
		int chr = yp * 32 + xp;
		
		this->SelectChar(chr);
		return true;
	}
	
	return CGuiView::DoTap(x, y);
}

bool CViewC64Charset::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	if (this->IsInsideView(x, y))
		return true;
	
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}


int CViewC64Charset::GetSelectedChar()
{
	return selectedChar;
}

void CViewC64Charset::SelectChar(int chr)
{
	if (chr < 0 || chr > 255)
	{
		this->selectedChar = -1;
		return;
	}
	
	this->selectedChar = chr;
	
	selSizeX = this->sizeX / 32.0f;
	selSizeY = this->sizeY / 8.0f;

	int row = floor((float)chr / 32.0f);
	int col = chr % 32;
	
	selX = selSizeX * col;
	selY = selSizeY * row;
}


bool CViewC64Charset::DoRightClick(float x, float y)
{
	LOGI("CViewC64Charset::DoRightClick: %f %f", x, y);
	
	if (this->IsInsideView(x, y))
		return true;
	
	return CGuiView::DoRightClick(x, y);
}


bool CViewC64Charset::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI("CViewC64Charset::KeyDown: %d", keyCode);
	
	return false;
}

bool CViewC64Charset::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

void CViewC64Charset::ToolBoxIconPressed(CSlrImage *imgIcon)
{
	if (imgIcon == this->imgIconExport)
	{
		CSlrString *defaultFileName = new CSlrString("charset");

		CSlrString *windowTitle = new CSlrString("Export Charset");
		viewC64->ShowDialogSaveFile(this, &charsetFileExtensions, defaultFileName, c64SettingsDefaultSnapshotsFolder, windowTitle);
		delete windowTitle;
		delete defaultFileName;
	}
	else if (imgIcon == this->imgIconImport)
	{
		CSlrString *windowTitle = new CSlrString("Import Charset");
		windowTitle->DebugPrint("windowTitle=");
		viewC64->ShowDialogOpenFile(this, &charsetFileExtensions, NULL, windowTitle);
		delete windowTitle;
	}
}

//
bool CViewC64Charset::HasContextMenuItems()
{
	return true;
}

void CViewC64Charset::RenderContextMenuItems()
{
	bool multiColor = viewC64->viewC64MemoryDataDump->renderDataWithColors;
	if (ImGui::MenuItem("Multicolor", viewC64->mainMenuBar->kbsToggleMulticolorImageDump->cstr, &multiColor))
	{
		viewC64->SetIsMulticolorDataDump(multiColor);
	}
	ImGui::Separator();
	if (ImGui::MenuItem("Export Charset"))
	{
		CSlrString *defaultFileName = new CSlrString("charset");

		CSlrString *windowTitle = new CSlrString("Export Charset");
		viewC64->ShowDialogSaveFile(this, &charsetFileExtensions, defaultFileName, c64SettingsDefaultSnapshotsFolder, windowTitle);
		delete windowTitle;
		delete defaultFileName;
	}
	if (ImGui::MenuItem("Import Charset"))
	{
		CSlrString *windowTitle = new CSlrString("Import Charset");
		windowTitle->DebugPrint("windowTitle=");
		viewC64->ShowDialogOpenFile(this, &charsetFileExtensions, NULL, windowTitle);
		delete windowTitle;
	}
	
	recentlyOpened->RenderImGuiMenu("Recent##CViewC64Charset");
}
//
void CViewC64Charset::ImportCharsetAndShowMessage(CSlrString *path)
{
	int importCharsetAddr = this->ImportCharset(path);

	CSlrString *str = path->GetFileNameComponentFromPath();
	
	char *buf = str->GetStdASCII();
	char *buf2 = SYS_GetCharBuf();
	sprintf(buf2, "%s imported at $%04x", buf, importCharsetAddr);
	viewC64->ShowMessageSuccess(buf2);
	SYS_ReleaseCharBuf(buf2);
	delete [] buf;
	delete str;
}

void CViewC64Charset::SystemDialogFileOpenSelected(CSlrString *path)
{
	ImportCharsetAndShowMessage(path);
	recentlyOpened->Add(path);
}

void CViewC64Charset::RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *path)
{
	ImportCharsetAndShowMessage(path);
}

int CViewC64Charset::ImportCharset(CSlrString *path)
{
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 colors[0x0F];
	
	vicEditor->viewVicDisplay->GetViciiPointers(&(viewC64->viciiStateToShow), &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);
	
	int importCharsetAddr = vicEditor->viewVicDisplay->charsetAddress;
	
	int charsetAddr = importCharsetAddr;
	
	CSlrFile *file = new CSlrFileFromOS(path);
	if (!file->Exists())
	{
		guiMain->ShowMessageBox("Error", "Import charset failed. Can't open file.");
		return -1;
	}
	
	CByteBuffer *byteBuffer = new CByteBuffer(file, false);
	for (int i = 0; i < 0x800; i++)
	{
		if (byteBuffer->IsEof())
		{
			guiMain->ShowMessageBox("Error", "Importing charset failed. Unexpected end of file reached. Ensure the file is complete and not corrupted.");
			return -1;
		}
		
		u8 v = byteBuffer->GetU8();
		
		viewC64->debugInterfaceC64->SetByteToRamC64(charsetAddr, v);
		
		charsetAddr++;
		if (charsetAddr > 0xFFFF)
		{
			break;
		}
	}
	
	delete byteBuffer;
	delete file;
	
	return importCharsetAddr;
}

void CViewC64Charset::SystemDialogFileOpenCancelled()
{
}

void CViewC64Charset::SystemDialogFileSaveSelected(CSlrString *path)
{
	this->ExportCharset(path);
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" saved");
	viewC64->ShowMessageInfo(str);
	delete str;

	recentlyOpened->Add(path);
}

bool CViewC64Charset::ExportCharset(CSlrString *path)
{
	LOGM("CViewC64Charset::ExportCharset");
	
	path->DebugPrint("ExportCharset path=");

	guiMain->LockMutex();
	
	char *cPath;
	
	//
	CSlrString *pathNoExt = path->GetFilePathWithoutExtension();
	pathNoExt->DebugPrint("pathNoExt=");
	
	CSlrString *pathCharset = new CSlrString(pathNoExt);
	char buf[64];
	sprintf(buf, ".charset"); //-charset.data"); //.charset"); //-charset.data");
	
	pathCharset->Concatenate(buf);
	cPath = pathCharset->GetStdASCII();
	LOGD(" ..... cPath='%s'", cPath);
	
	
	CSlrFileFromOS *file = new CSlrFileFromOS(cPath, SLR_FILE_MODE_WRITE);
	delete [] cPath;
	delete pathCharset;
	delete pathNoExt;

	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	vicDisplay->GetViciiPointers(&(viewC64->viciiStateToShow),
										 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	//
	file->Write(chargen_ptr, 0x0800);
	
	file->Close();
	
	guiMain->UnlockMutex();
	
	LOGM("CViewC64Charset::ExportCharset: file saved");
	
	return true;
}

void CViewC64Charset::SystemDialogFileSaveCancelled()
{
}


