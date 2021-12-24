#include "CViewC64Charset.h"
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
#include "CViewDataDump.h"
#include "C64SettingsStorage.h"
#include "CSlrFileFromOS.h"

CViewC64Charset::CViewC64Charset(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewVicEditor *vicEditor)
: CGuiWindow(name, posX, posY, posZ, sizeX, sizeY, new CSlrString("Charset"))
{
	this->vicEditor = vicEditor;
	
	int w = 256;
	int h = 64;
	imageDataCharset = new CImageData(w, h, IMG_TYPE_RGBA);
	imageDataCharset->AllocImage(false, true);
		
	imageCharset = new CSlrImage(true, true);
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

	// setup icons in frame
	viewFrame->AddBarToolBox(this);
	
	imgIconImport = RES_GetImage("/gfx/icon_raw_import", true);
	viewFrame->AddBarIcon(imgIconImport);
	
	imgIconExport = RES_GetImage("/gfx/icon_raw_export", true);
	viewFrame->AddBarIcon(imgIconExport);
	
	SelectChar(17 + 0x40);
	
	this->UpdatePosition();
}

void CViewC64Charset::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	LOGD("CViewC64Charset::SetPosition: %f %f", posX, posY);
	CGuiWindow::SetPosition(posX, posY, posZ, sizeX, sizeY);
	
	selSizeX = this->sizeX / 32.0f;
	selSizeY = this->sizeY / 8.0f;
}

void CViewC64Charset::DoLogic()
{
	CGuiWindow::DoLogic();
}


void CViewC64Charset::Render()
{
//	LOGD("CViewC64Charset::Render: pos=%f %f", posX, posY);
	BlitFilledRectangle(posX, posY, posZ, sizeX, sizeY, viewFrame->barColorR, viewFrame->barColorG, viewFrame->barColorB, 1);
	float py = posY;
	float px;
	
	vicii_cycle_state_t *viciiState = &(viewC64->viciiStateToShow);

	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 colors[0x0F];
	
	vicEditor->viewVicDisplayMain->GetViciiPointers(viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);

	if (viewC64->viewC64VicDisplay->backupRenderDataWithColors)
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
	
	// TODO: create generic engine function for this
	
	// nearest neighbour
	{
		glBindTexture(GL_TEXTURE_2D, this->imageCharset->textureId);
		
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	}
	
	
	Blit(this->imageCharset, posX, posY, posZ, sizeX, sizeY);
	
	
//	// back to linear scale
//	{
//		glBindTexture(GL_TEXTURE_2D, this->imageCharset->texture[0]);
//		
//		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
//	}

	if (selectedChar >= 0)
	{
		BlitRectangle(this->posX + selX, this->posY + selY, posZ, selSizeX, selSizeY, 1.0f, 0.0f, 0.0f, 1.0f, 1.5f);
	}
	
	
	CGuiWindow::Render();

	BlitRectangle(posX, posY, posZ, sizeX, sizeY, 0, 0, 0, 1, 1);
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
	
	return CGuiWindow::DoTap(x, y);
}

bool CViewC64Charset::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	if (this->IsInsideView(x, y))
		return true;
	
	return CGuiWindow::DoMove(x, y, distX, distY, diffX, diffY);
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
	
	return CGuiWindow::DoRightClick(x, y);
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

void CViewC64Charset::SystemDialogFileOpenSelected(CSlrString *path)
{
	int importCharsetAddr = this->ImportCharset(path);
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	
	char *buf = str->GetStdASCII();
	char *buf2 = SYS_GetCharBuf();
	sprintf(buf2, "%s imported at $%04x", buf, importCharsetAddr);
	guiMain->ShowMessage(buf2);
	SYS_ReleaseCharBuf(buf2);
	delete [] buf;
	delete str;
}

int CViewC64Charset::ImportCharset(CSlrString *path)
{
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 colors[0x0F];
	
	vicEditor->viewVicDisplaySmall->GetViciiPointers(&(viewC64->viciiStateToShow), &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);
	
	int importCharsetAddr = vicEditor->viewVicDisplaySmall->charsetAddress;
	
	int charsetAddr = importCharsetAddr;
	
	CSlrFile *file = new CSlrFileFromOS(path);
	if (!file->Exists())
	{
		guiMain->ShowMessage("Can't open file");
		return -1;
	}
	
	CByteBuffer *byteBuffer = new CByteBuffer(file, false);
	for (int i = 0; i < 0x800; i++)
	{
		if (byteBuffer->IsEof())
		{
			guiMain->ShowMessage("End of file reached");
			return -1;
		}
		
		u8 v = byteBuffer->GetU8();
		
		viewC64->debugInterfaceC64->SetByteC64(charsetAddr, v);
		
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
	guiMain->ShowMessage(str);
	delete str;
}

void CViewC64Charset::ExportCharset(CSlrString *path)
{
	vicEditor->ExportCharset(path);
}

void CViewC64Charset::SystemDialogFileSaveCancelled()
{
}

