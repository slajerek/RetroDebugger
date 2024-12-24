#ifndef _CDebuggerApiNestopia_H_
#define _CDebuggerApiNestopia_H_

#include "SYS_Defs.h"
#include "DebuggerDefs.h"
#include "SYS_KeyCodes.h"
#include "CDebugInterface.h"
#include "SYS_Threading.h"
#include "CImageData.h"
#include "CDebuggerApi.h"
#include "CDebugInterfaceNes.h"

class CGuiView;

class CDebuggerApiNestopia : public CDebuggerApi
{
public:
	CDebuggerApiNestopia(CDebugInterface *debugInterface);
	CDebugInterfaceNes *debugInterfaceNes;
	
	virtual void CreateNewPicture(u8 mode, u8 backgroundColor);
	virtual void StartThread(CSlrThread *run);

	// rgb reference image in vic editor
	virtual void ClearReferenceImage();
	virtual void LoadReferenceImage(char *filePath);
	virtual void LoadReferenceImage(CImageData *imageData);
	virtual void SetReferenceImageLayerVisible(bool isVisible);
	CImageData *GetReferenceImage();

	// emulated computer screen
	virtual void ClearScreen();
	virtual void ZoomDisplay(float newScale);

	// load from png file
	virtual bool ConvertImageToScreen(char *filePath);
	virtual bool ConvertImageToScreen(CImageData *imageData);
	virtual CImageData *GetScreenImage(int *width, int *height);
	
	// always returns 320x200 for C64:
	virtual CImageData *GetScreenImageWithoutBorders();

	//
	virtual u8 PaintPixel(int x, int y, u8 color);
	virtual u8 PaintReferenceImagePixel(int x, int y, u8 color);
	virtual u8 PaintReferenceImagePixel(int x, int y, u8 r, u8 g, u8 b, u8 a);
	
	//
	virtual void SetByteWithIo(int addr, u8 v);
	virtual void SetByteToRam(int addr, u8 v);
	virtual void SetWord(int addr, u16 v);
	virtual void MakeJmp(int addr);

	//
	virtual void DetachEverything();
	
	//
	virtual int Assemble(int addr, char *assembleText);
	
	//
	virtual nlohmann::json GetCpuStatusJson();
};

#endif
