#ifndef _CDebuggerApiVice_H_
#define _CDebuggerApiVice_H_

#include "SYS_Defs.h"
#include "DebuggerDefs.h"
#include "SYS_KeyCodes.h"
#include "CDebugInterface.h"
#include "SYS_Threading.h"
#include "CImageData.h"
#include "CDebuggerApi.h"

enum {
	ASSEMBLE_TARGET_NONE,
	ASSEMBLE_TARGET_MAIN_CPU,
	ASSEMBLE_TARGET_DISK_DRIVE1
};

class CGuiView;
class CDebugInterface;
class CDebugInterfaceVice;

class CDebuggerApiVice : public CDebuggerApi
{
public:
	CDebuggerApiVice(CDebugInterface *debugInterface);
	CDebugInterfaceVice *debugInterfaceVice;
	
	void ResetMachine();
	
	void SwitchToVicEditor();
	void CreateNewPicture(u8 mode, u8 backgroundColor);

	// rgb reference image in vic editor
	void ClearReferenceImage();
	void LoadReferenceImage(char *filePath);
	void LoadReferenceImage(CImageData *imageData);
	void SetReferenceImageLayerVisible(bool isVisible);
	CImageData *GetReferenceImage();

	// vic editor dialogs -> to be moved
	void SetTopBarVisible(bool isVisible);
	void SetViewPaletteVisible(bool isVisible);
	void SetViewCharsetVisible(bool isVisible);
	void SetViewSpriteVisible(bool isVisible);
	void SetViewLayersVisible(bool isVisible);
	void SetViewPreviewVisible(bool isVisible);
	void SetSpritesFramesVisible(bool isVisible);
	void ZoomDisplay(float newScale);
	
	// this shows in VicEditor only screen without any other views
	void SetupVicEditorForScreenOnly();
	
	// emulated computer screen
	void ClearScreen();
	
	// load from png file
	void ConvertImageToScreen(char *filePath);

	void ConvertImageToScreen(CImageData *imageData);
	
	CImageData *GetScreenImage(int *width, int *height);
	
	// always returns 320x200 for C64:
	CImageData *GetScreenImageWithoutBorders();

	//
	u8 FindC64Color(u8 r, u8 g, u8 b);
	u8 PaintPixel(int x, int y, u8 color);
	u8 PaintReferenceImagePixel(int x, int y, u8 color);
	u8 PaintReferenceImagePixel(int x, int y, u8 r, u8 g, u8 b, u8 a);
	
	//
	void GetCBMColor(u8 colorNum, u8 *r, u8 *g, u8 *b);
	
	//
	void SetByte(int addr, u8 v);	/// NOTE: this needs change
	void SetByteWithIo(int addr, u8 v);
	void SetByteToRam(int addr, u8 v);
	void SetByteToRamC64(int addr, u8 v);
	u8 GetByteFromRamC64(int addr);
	
	virtual u8 GetByteWithIo(int addr);
	virtual u8 GetByteFromRam(int addr);

	void MakeJmp(int addr);

	//
	void SetCiaRegister(u8 ciaId, u8 registerNum, u8 value);
	void SetVicRegister(u16 registerNum, u8 value);
	
	//
	void DetachEverything();
	void ClearRam(int startAddr, int endAddr, u8 value);
	
	//
	u8 assembleTarget;
	void SetAssembleTarget(u8 target);
	int Assemble(int addr, char *assembleText);
		
	//
	void AddWatch(CSlrString *segmentName, int address, CSlrString *watchName, uint8 representation, int numberOfValues);
	void AddWatch(int address, char *watchName, uint8 representation, int numberOfValues);
	void AddWatch(int address, char *watchName);
	
	//
	void BasicUpStart(u16 jmpAddr);
	bool LoadPRG(const char *filePath, u16 *fromAddr, u16 *toAddr);
	bool LoadPRG(CByteBuffer *byteBuffer, u16 *fromAddr, u16 *toAddr);
	bool LoadPRG(CByteBuffer *byteBuffer, bool autoStart, bool forceFastReset);
	bool LoadCRT(CByteBuffer *byteBuffer);
	bool LoadCRT(const char *filePath);
	bool LoadSID(const char *filePath, u16 *fromAddr, u16 *toAddr, u16 *initAddr, u16 *playAddr);
	bool LoadSID(const char *filePath, u16 *fromAddr, u16 *toAddr, u16 *initAddr, u16 *playAddr, u8 **buffer);
//	bool LoadAndRelocateSID(char *filePath, u16 fromAddr, u16 *toAddr, u16 *initAddr, u16 *playAddr);
	bool LoadKLA(const char *filePath);
	bool LoadKLA(const char *filePath, u16 bitmapAddress, u16 screenAddress, u16 colorRamAddress, u8 *colorD020, u8 *colorD021);
	void SaveExomizerPRG(u16 fromAddr, u16 toAddr, u16 jmpAddr, char *fileName);
	void SavePRG(u16 fromAddr, u16 toAddr, char *fileName);
	void AddCrtEntryPoint(u8 *cartImage, u16 coldStartAddr, u16 warmStartAddr);
	CByteBuffer *MakeCrt(const char *cartName, int cartSize, int bankSize, u8 *cartImage);
	void SaveBinary(u16 fromAddr, u16 toAddr, char *fileName);
	int LoadBinary(u16 fromAddr, char *filePath);

	u8 *ExomizerMemoryRaw(u16 fromAddr, u16 toAddr, int *compressedSize);
	
	//
	void SetScreenAndCharsetAddress(u16 screenAddr, u16 charsetAddr);
	
	//
	void ShowMessage(const char *text);
	void BlitText(const char *text, float posX, float posY, float fontSize);
	void Sleep(long milliseconds);
	long GetCurrentTimeInMilliseconds();
	
	//
	void AddView(CGuiView *view);
};

#endif
