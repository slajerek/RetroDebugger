#ifndef _CViewC64Charset_H_
#define _CViewC64Charset_H_

#include "SYS_Defs.h"
#include "CGuiWindow.h"
#include "CGuiEditHex.h"
#include "CGuiViewFrame.h"
#include "CGuiViewToolBox.h"
#include "SYS_FileSystem.h"
#include <vector>
#include <list>

class CSlrFont;
class CDataAdapter;
class CViewMemoryMap;
class CSlrMutex;
class CDebugInterfaceC64;
class CViewVicEditor;

class CViewC64Charset : public CGuiWindow, CGuiEditHexCallback, public CGuiViewToolBoxCallback, public CGuiWindowCallback, CSystemFileDialogCallback
{
public:
	CViewC64Charset(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewVicEditor *vicEditor);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	
	virtual bool DoRightClick(float x, float y);
//	virtual bool DoFinishRightClick(float x, float y);

	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);

	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	
	virtual void Render();
	virtual void DoLogic();
	
	//
	CViewVicEditor *vicEditor;
	
	CImageData *imageDataCharset;
	CSlrImage *imageCharset;
	
	CSlrImage *imgIconExport;
	CSlrImage *imgIconImport;
	
	float selX, selY;
	float selSizeX, selSizeY;
	
	int selectedChar;
	
	int GetSelectedChar();
	void SelectChar(int chr);
	
	virtual void ToolBoxIconPressed(CSlrImage *imgIcon);

	std::list<CSlrString *> charsetFileExtensions;

	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileOpenCancelled();
	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();

	// returns charset addr
	int ImportCharset(CSlrString *path);
	void ExportCharset(CSlrString *path);
};


#endif

