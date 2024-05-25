#ifndef _CViewC64CrtMaker_h_
#define _CViewC64CrtMaker_h_

#include "CGuiView.h"
#include "CGuiButton.h"
#include "CGuiViewMenu.h"
#include "SYS_FileSystem.h"
#include <list>

class CSlrKeyboardShortcut;
class CViewC64MenuItem;
class CGuiEvent;
class CDebuggerApiVice;
class C64DebuggerPluginCrtMaker;
class CCrtMakerFile;

extern char *crtMakerConfigFilePath;

class CViewC64CrtMaker : public CGuiView, CGuiButtonCallback, CGuiViewMenuCallback, CSystemFileDialogCallback
{
public:
	CViewC64CrtMaker(float posX, float posY, float posZ, float sizeX, float sizeY, C64DebuggerPluginCrtMaker *plugin);
	virtual ~CViewC64CrtMaker();
	
	virtual void Render();
	virtual void Render(float posX, float posY);
	virtual void RenderImGui();
	virtual void DoLogic();
	
	virtual bool DoTap(float x, float y);
	virtual bool DoFinishTap(float x, float y);
	
	virtual bool DoDoubleTap(float x, float y);
	virtual bool DoFinishDoubleTap(float posX, float posY);
	
	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);
	
	virtual bool InitZoom();
	virtual bool DoZoomBy(float x, float y, float zoomValue, float difference);
	
	// multi touch
	virtual bool DoMultiTap(COneTouchData *touch, float x, float y);
	virtual bool DoMultiMove(COneTouchData *touch, float x, float y);
	virtual bool DoMultiFinishTap(COneTouchData *touch, float x, float y);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);	// repeats
	
	virtual void ActivateView();
	virtual void DeactivateView();
	
	virtual void Run();

	C64DebuggerPluginCrtMaker *plugin;
	CDebuggerApiVice *api;
	
	CSlrFont *font;
	float fontScale;
	float fontHeight;

	//
	bool ReadConfigFromFile(char *hjsonFilePath);
	bool ReadBuildFromFile(char *hjsonFilePath);
	bool ProcessFiles();
	bool MakeCartridge();
	
	bool AssembleFile(CCrtMakerFile *file);
	bool ExomizeFile(CCrtMakerFile *file);
	
	// config
	char *cartName;
	char *cartOutPath;
	char *exomizerPath;
	char *exomizerParams;
	char *javaPath;
	char *kickAssJarPath;
	char *kickAssParams;
	char *buildFilePath;
	char *rootFolderPath;
	char *binFilesPath;
	char *exoFilesPath;
	char *decrunchBinPath;
	u16   decrunchBinStartAddr;

	//
	int cartSize;
	int bankSize;
	int numBanks;
	
	u8 *cartImage;
	u16 relocCodeStart;
	
	CByteBuffer *crtBuffer;
	
	// files to include
	std::list<CCrtMakerFile *> files;
	void AddFile(CCrtMakerFile *file);
	
	u16 codeEntryPoint;
	u16 cartImageOffset;

	// file tables
	u16 addrFileTables;
	u16 addrFileTableBank;
	u16 addrFileTableAddrL;
	u16 addrFileTableAddrH;
	u16 addrCodeStart;
	
	//
	int decrunchCodeLen;
	
	//
	void AddInitCode(int *codeStartAddr, int *codeSize);
	
	//
	u16 depackFrameworkAddrLoadFile;
	int depackFrameworkLen;
	void AddDepackerFramework();
	
	// forward
	u8 *Assemble64Tass(int *codeStartAddr, int *codeSize);
	void CopyToRam(int codeStartAddr, int codeSize, u8 *buf);
	
	CByteBuffer *byteBufferMapText;
	void PrintMap(const char *format, ...);
	
	// logs
	ImGuiTextBuffer     Buf;
	ImGuiTextFilter     Filter;
	ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
	bool                AutoScroll;  // Keep scrolling if already at the bottom.
	
	void Clear();
	void Print(const char* fmt, ...) IM_FMTARGS(2);
	void PrintError(const char* fmt, ...) IM_FMTARGS(2);

	CSlrMutex *mutex;
};

#endif //_VIEW_C64GOATTRACKER_
