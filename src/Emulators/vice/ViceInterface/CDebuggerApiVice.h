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
class CSidData;

class CDebuggerApiVice : public CDebuggerApi
{
public:
	CDebuggerApiVice(CDebugInterface *debugInterface);
	CDebugInterfaceVice *debugInterfaceVice;
	
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
	virtual bool ConvertImageToScreen(char *filePath);
	
	// load from CImageData
	virtual bool ConvertImageToScreen(CImageData *imageData);
	
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
	CDataAdapter *GetDataAdapterDrive1541MemoryWithIO();
	CDataAdapter *GetDataAdapterDrive1541MemoryDirectRAM();
	
	//
	void SetByte(int addr, u8 v);	/// NOTE: this needs change
	void SetByteWithIo(int addr, u8 v);
	void SetByteToRam(int addr, u8 v);
	void SetByteToRamC64(int addr, u8 v);
	u8 GetByteFromRamC64(int addr);
	
	virtual u8 GetByteWithIo(int addr);
	virtual u8 GetByteFromRam(int addr);
	
	void MakeJmp(int addr);
	
	// CIA
	void SetCiaRegister(u8 ciaId, u8 registerNum, u8 value);
	u8 GetCiaRegister(u8 ciaId, u8 registerNum);

	// SIDs
	// write multiple registers to SIDs at once. Note, the CSidData is _not_deleted_ after use
	void SetSid(CSidData *sidData);

	// note SetSidRegister writes only _one_ register in next emulation cycle via trap, previous not-executed writes will be skipped (overwritten).
	// if you need to write to multiple registers then use CSidData instead.
	// TODO: we can add a mechanism to collect SidRegister writes here and use CSidData instead, TBD
	void SetSidRegister(uint8 sidId, uint8 registerNum, uint8 value);
	u8 GetSidRegister(uint8 sidId, uint8 registerNum);

	// VIC
	void SetVicRegister(u16 registerNum, u8 value);
	u8 GetVicRegister(u16 registerNum);
	
	// Drive1541 VIA
	void SetDrive1541ViaRegister(u8 driveId, u8 viaId, u8 registerNum, u8 value);
	u8 GetDrive1541ViaRegister(u8 driveId, u8 viaId, u8 registerNum);

	//
	void DetachEverything();
	void ClearRam(int startAddr, int endAddr, u8 value);
	
	//
	u8 assembleTarget;
	void SetAssembleTarget(u8 target);
	int Assemble(int addr, char *assembleText);
	
	//
	void BasicUpStart(u16 jmpAddr);
	bool LoadPRG(const char *filePath);
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
	bool SaveExomizerPRG(u16 fromAddr, u16 toAddr, u16 jmpAddr, const char *fileName);
	bool SavePRG(u16 fromAddr, u16 toAddr, const char *fileName);
	void AddCrtEntryPoint(u8 *cartImage, u16 coldStartAddr, u16 warmStartAddr);
	CByteBuffer *MakeCrt(const char *cartName, int cartSize, int bankSize, u8 *cartImage);
	
	u8 *ExomizerMemoryRaw(u16 fromAddr, u16 toAddr, int *compressedSize);
	
	//
	void SetScreenAndCharsetAddress(u16 screenAddr, u16 charsetAddr);
	
	// breakpoints
	u64 AddBreakpointRasterLine(int rasterLine);
	u64 RemoveBreakpointRasterLine(int rasterLine);

	//
	void ShowMessage(const char *text);
	void BlitText(const char *text, float posX, float posY, float fontSize);
	void Sleep(long milliseconds);
	long GetCurrentTimeInMilliseconds();
	
	//
	void AddView(CGuiView *view);

	//
	virtual nlohmann::json GetCpuStatusJson();

};

#endif
